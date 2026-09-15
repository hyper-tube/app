#include "PlayerScript.h"

#include "core/Logging.h"
#include "core/Paths.h"
#include "net/HttpClient.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJSEngine>
#include <QRegularExpression>
#include <QSaveFile>

namespace {

constexpr int kIdleReleaseMs = 5 * 60 * 1000;
constexpr const char *kNoisyCategory = "qt.qml.usedbeforedeclared";

const QString kVersionUrl = QStringLiteral("https://www.youtube.com/iframe_api");
const QString kSourceUrl =
    QStringLiteral("https://www.youtube.com/s/player/%1/player_ias.vflset/en_US/base.js");

const char *kBrowserShim = R"JS(
var globalThis = this;
var window = this;
var self = this;
var location = { href: 'https://www.youtube.com/', protocol: 'https:',
    hostname: 'www.youtube.com', origin: 'https://www.youtube.com', search: '', hash: '' };
var navigator = { userAgent: 'Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/152.0.0.0 Safari/537.36',
    platform: 'Linux x86_64', languages: ['en-US'], language: 'en-US', mimeTypes: [], plugins: [],
    sendBeacon: function () { return true; }, connection: {} };
function element() {
    return { style: {}, setAttribute: function () {}, getAttribute: function () { return null; },
        appendChild: function () {}, removeChild: function () {}, addEventListener: function () {},
        removeEventListener: function () {},
        classList: { add: function () {}, remove: function () {}, contains: function () { return false; } },
        getElementsByTagName: function () { return []; }, querySelectorAll: function () { return []; },
        cloneNode: function () { return element(); } };
}
var document = { location: location, cookie: '', documentElement: element(), body: element(),
    head: element(), createElement: element, createTextNode: element,
    getElementById: function () { return null; }, getElementsByTagName: function () { return []; },
    querySelector: function () { return null; }, querySelectorAll: function () { return []; },
    addEventListener: function () {}, removeEventListener: function () {},
    createEvent: function () { return { initEvent: function () {} }; },
    visibilityState: 'visible', readyState: 'complete' };
function XMLHttpRequest() {
    this.open = function () {}; this.send = function () {}; this.setRequestHeader = function () {};
}
function setTimeout() { return 0; }
function clearTimeout() {}
function setInterval() { return 0; }
function clearInterval() {}
var performance = { now: function () { return Date.now(); }, timing: {} };
var screen = { width: 1920, height: 1080 };
function matchMedia() { return { matches: false, addListener: function () {}, removeListener: function () {} }; }
function fetch() { return Promise.reject(new Error('unavailable')); }
)JS";

const char *kTransform = R"JS(
(function () {
    var accessor = null;
    var throttle = /[?&]n=([^&]*)/;

    function rebuild(url, before) {
        var names = Object.keys(_yt_player);
        for (var i = 0; i < names.length; ++i) {
            var factory = _yt_player[names[i]];
            if (typeof factory !== 'function')
                continue;
            var box;
            try { box = new factory(url, true); } catch (error) { continue; }
            if (!box || typeof box !== 'object')
                continue;
            var shape = Object.getPrototypeOf(box);
            if (!shape)
                continue;
            var members = Object.getOwnPropertyNames(shape);
            for (var j = 0; j < members.length; ++j) {
                var member = members[j];
                var method;
                try { method = box[member]; } catch (error) { continue; }
                if (typeof method !== 'function' || method.length !== 0)
                    continue;
                var built = apply(factory, member, url);
                if (built && throttle.exec(built)[1] !== before)
                    return { factory: factory, member: member, url: built };
            }
        }
        return null;
    }

    function apply(factory, member, url) {
        try {
            var built = new factory(url, true)[member]();
            return typeof built === 'string' && built.indexOf('/videoplayback') >= 0
                && throttle.exec(built) ? built : '';
        } catch (error) {
            return '';
        }
    }

    return function (url) {
        var before = throttle.exec(url);
        if (!before)
            return url;
        if (accessor) {
            var quick = apply(accessor.factory, accessor.member, url);
            if (quick && throttle.exec(quick)[1] !== before[1])
                return quick;
            accessor = null;
        }
        var found = rebuild(url, before[1]);
        if (!found)
            return '';
        accessor = found;
        return found.url;
    };
})()
)JS";

class SilencedDeclarations
{
public:
    SilencedDeclarations() { s_previous = qInstallMessageHandler(&forward); }
    ~SilencedDeclarations() { qInstallMessageHandler(s_previous); }

private:
    static void forward(QtMsgType type, const QMessageLogContext &context, const QString &message)
    {
        if (context.category && qstrcmp(context.category, kNoisyCategory) == 0)
            return;
        if (s_previous)
            s_previous(type, context, message);
    }

    static inline QtMessageHandler s_previous = nullptr;
};

QString versionFrom(const QByteArray &body)
{
    static const QRegularExpression pattern(
        QStringLiteral("player\\\\?/([0-9a-zA-Z_-]{4,})\\\\?/"));
    const QRegularExpressionMatch match = pattern.match(QString::fromUtf8(body));
    return match.hasMatch() ? match.captured(1) : QString();
}

int signatureTimestampFrom(const QString &source)
{
    static const QRegularExpression pattern(QStringLiteral("signatureTimestamp:([0-9]+)"));
    const QRegularExpressionMatch match = pattern.match(source);
    return match.hasMatch() ? match.captured(1).toInt() : 0;
}

}

namespace player {

PlayerScript::PlayerScript(QObject *parent)
    : QObject(parent)
{
    m_idle.setSingleShot(true);
    m_idle.setInterval(kIdleReleaseMs);
    connect(&m_idle, &QTimer::timeout, this, [this] {
        m_transform = QJSValue();
        m_engine.reset();
        qCDebug(logStream) << "player script released";
    });
}

PlayerScript::~PlayerScript() = default;

PlayerScript &PlayerScript::instance()
{
    static auto *script = new PlayerScript(QCoreApplication::instance());
    return *script;
}

QString PlayerScript::cacheFile(const QString &version) const
{
    const QString path = core::paths::cacheDir() + QStringLiteral("/player");
    QDir().mkpath(path);
    return path + QLatin1Char('/') + version + QStringLiteral(".js");
}

void PlayerScript::ready(Handler handler)
{
    m_pending.append(std::move(handler));
    if (m_pending.size() > 1 || m_fetching)
        return;
    start();
}

QUrl PlayerScript::descramble(const QUrl &url)
{
    if (!m_engine || !m_transform.isCallable())
        return {};
    m_idle.start();
    const QJSValue outcome = m_transform.call({url.toString()});
    if (outcome.isError())
        return {};
    const QString built = outcome.toString();
    return built.isEmpty() ? QUrl() : QUrl(built);
}

void PlayerScript::start()
{
    if (m_engine && m_transform.isCallable()) {
        serve(m_signatureTimestamp);
        return;
    }
    m_fetching = true;
    if (m_version.isEmpty())
        fetchVersion();
    else
        fetchSource();
}

void PlayerScript::fetchVersion()
{
    net::HttpClient::instance().get(QUrl(kVersionUrl), {}, net::Credentialed::No,
                                    [this](const net::Response &response) {
        if (!response.ok()) {
            qCWarning(logStream) << "player version request failed" << response.error;
            serve(0);
            return;
        }
        m_version = versionFrom(response.body);
        if (m_version.isEmpty()) {
            qCWarning(logStream) << "player version missing from the iframe api";
            serve(0);
            return;
        }
        fetchSource();
    });
}

void PlayerScript::fetchSource()
{
    QFile cached(cacheFile(m_version));
    if (cached.open(QIODevice::ReadOnly)) {
        adoptSource(cached.readAll());
        return;
    }

    net::HttpClient::instance().get(QUrl(kSourceUrl.arg(m_version)), {}, net::Credentialed::No,
                                    [this](const net::Response &response) {
        if (!response.ok() || response.body.isEmpty()) {
            qCWarning(logStream) << "player script request failed" << response.error;
            serve(0);
            return;
        }
        const QString path = cacheFile(m_version);
        QDir folder(QFileInfo(path).absolutePath());
        for (const QString &stale : folder.entryList(QDir::Files))
            folder.remove(stale);
        QSaveFile file(path);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(response.body);
            file.commit();
        }
        adoptSource(response.body);
    });
}

void PlayerScript::adoptSource(const QByteArray &source)
{
    const QString script = QString::fromUtf8(source);
    m_signatureTimestamp = signatureTimestampFrom(script);
    m_fetching = false;
    if (!build(script)) {
        m_version.clear();
        m_signatureTimestamp = 0;
        serve(0);
        return;
    }
    serve(m_signatureTimestamp);
}

bool PlayerScript::build(const QString &source)
{
    const SilencedDeclarations quiet;
    m_engine = std::make_unique<QJSEngine>();
    const QJSValue shim = m_engine->evaluate(QString::fromUtf8(kBrowserShim));
    if (shim.isError()) {
        qCWarning(logStream) << "player shim failed" << shim.toString();
        m_engine.reset();
        return false;
    }

    const QJSValue loaded = m_engine->evaluate(source, QStringLiteral("base.js"));
    if (loaded.isError()) {
        qCWarning(logStream) << "player script failed at line"
                             << loaded.property(QStringLiteral("lineNumber")).toInt()
                             << loaded.toString();
        m_engine.reset();
        return false;
    }

    m_transform = m_engine->evaluate(QString::fromUtf8(kTransform));
    if (m_transform.isError() || !m_transform.isCallable()) {
        qCWarning(logStream) << "player transform unavailable" << m_transform.toString();
        m_transform = QJSValue();
        m_engine.reset();
        return false;
    }
    qCInfo(logStream) << "player script" << m_version << "ready, signature timestamp"
                      << m_signatureTimestamp;
    return true;
}

void PlayerScript::serve(int signatureTimestamp)
{
    m_fetching = false;
    const QList<Handler> waiting = std::move(m_pending);
    m_pending.clear();
    for (const Handler &handler : waiting)
        handler(signatureTimestamp);
    if (signatureTimestamp > 0)
        m_idle.start();
}

}
