#include "Backend.h"

namespace diagnostics::backend {

bool compiled()
{
    return false;
}

bool start(const Configuration &)
{
    return false;
}

void stop(Ending) { }

bool running()
{
    return false;
}

void addBreadcrumb(const char *, const QByteArray &, std::span<const Field>, Level) { }

void capture(const Event &) { }

void setTag(const char *, const QByteArray &) { }

void setContext(const char *, std::span<const Field>) { }

void crash() { }

}
