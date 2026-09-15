#include "MprisRootAdaptor.h"

#include "MprisService.h"

#include <QGuiApplication>

namespace platform {

MprisRootAdaptor::MprisRootAdaptor(MprisService *service)
    : QDBusAbstractAdaptor(service)
    , m_service(service)
{
}

QString MprisRootAdaptor::identity() const
{
    return QGuiApplication::applicationDisplayName();
}

QString MprisRootAdaptor::desktopEntry() const
{
    return QGuiApplication::desktopFileName();
}

QStringList MprisRootAdaptor::supportedUriSchemes() const
{
    return {QStringLiteral("http"), QStringLiteral("https")};
}

QStringList MprisRootAdaptor::supportedMimeTypes() const
{
    return {};
}

void MprisRootAdaptor::Raise()
{
    m_service->requestRaise();
}

void MprisRootAdaptor::Quit()
{
    m_service->requestQuit();
}

}
