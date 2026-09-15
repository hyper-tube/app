#pragma once

#include "model/BrowseModel.h"

#include <QObject>
#include <QPointer>
#include <QQmlEngine>
#include <QHash>
#include <QSet>

#include <functional>

namespace media {

class Browser : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(model::BrowseModel *page READ page NOTIFY pageChanged)
    Q_PROPERTY(bool canGoBack READ canGoBack NOTIFY pageChanged)
    Q_PROPERTY(int section READ section NOTIFY pageChanged)
    Q_PROPERTY(int libraryTab READ libraryTab NOTIFY libraryTabChanged)
    Q_PROPERTY(QList<model::Chip> libraryTabs READ libraryTabs NOTIFY libraryTabsChanged)
    Q_PROPERTY(bool uploadsTab READ uploadsTab NOTIFY libraryTabChanged)
    Q_PROPERTY(bool playlistsTab READ playlistsTab NOTIFY libraryTabChanged)
    Q_PROPERTY(QString searchQuery READ searchQuery NOTIFY pageChanged)

public:
    explicit Browser(QObject *parent = nullptr);

    model::BrowseModel *page() const { return m_history.isEmpty() ? nullptr : m_history.last(); }
    bool canGoBack() const { return m_history.size() > 1; }
    int section() const { return m_section; }
    int libraryTab() const { return m_libraryTab; }
    QList<model::Chip> libraryTabs() const;
    bool uploadsTab() const;
    bool playlistsTab() const;
    QString searchQuery() const;

    Q_INVOKABLE void showSection(int section);
    Q_INVOKABLE void showLibraryTab(int tab);
    Q_INVOKABLE void search(const QString &query, const QString &params = {});
    Q_INVOKABLE void chooseChip(int index);
    Q_INVOKABLE void chooseScope(int index);
    Q_INVOKABLE void open(const model::Item &item);
    Q_INVOKABLE void activate(model::ItemModel *items, int index);
    Q_INVOKABLE void playPage();
    Q_INVOKABLE void playItem(model::ItemModel *items, int index);
    Q_INVOKABLE void playMix(const model::Item &item);
    Q_INVOKABLE void playEntry(const model::Item &item);
    Q_INVOKABLE void playShuffled(const model::Item &item);
    Q_INVOKABLE void openPage(const QString &browseId, const QString &kind);
    Q_INVOKABLE void resolveTracks(const model::Item &item, const QString &intent);
    Q_INVOKABLE void back();
    Q_INVOKABLE void retry() const;
    Q_INVOKABLE void reloadPage() const;
    Q_INVOKABLE void showDownloads();
    Q_INVOKABLE void showHistory();

Q_SIGNALS:
    void pageChanged();
    void libraryTabChanged();
    void libraryTabsChanged();
    void playRequested(const QList<media::Track> &tracks, int index, const model::Item &source);
    void resolutionFailed(const QString &message);
    void tracksResolved(const QList<media::Track> &tracks, const QString &intent);

private:
    void push(model::BrowseModel *target);
    void clearHistory();
    void showLibrary();
    void adoptLibraryTabs();
    QString libraryTabId(int tab) const;
    bool libraryOwns(const model::BrowseModel *surface) const;
    bool retains(const model::BrowseModel *surface) const;
    void discardLibrary();
    void staleLibrary();
    void retranslate();
    void refresh();
    void followConnectivity();
    void recover();
    void playTrack(model::ItemModel *items, int index);
    void fetchTracks(const QString &playlistId, const QString &videoId,
                     const std::function<void(const QList<media::Track> &)> &handler);
    static QString queueTarget(const model::Item &item);
    static model::Item sourceOf(const model::BrowseModel &surface);
    model::BrowseModel *library(int tab);

    model::BrowseModel *m_home;
    model::BrowseModel *m_explore;
    model::BrowseModel *m_downloads;
    QHash<QString, model::BrowseModel *> m_library;
    QList<model::Chip> m_serverTabs;
    QSet<model::BrowseModel *> m_stale;
    QList<model::BrowseModel *> m_history;
    QPointer<model::BrowseModel> m_autoplay;
    innertube::Endpoints m_endpoints;
    int m_section = 0;
    int m_libraryTab = 0;
    bool m_signedIn = false;
    bool m_navigated = false;
};

}
