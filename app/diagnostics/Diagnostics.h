#pragma once

#include "Value.h"

namespace diagnostics {

bool active();

void breadcrumb(const char *category, Fields fields = {}, Level level = Level::Info);
void captureMessage(const char *name, Fields fields = {}, Level level = Level::Warning);
void captureError(const char *name, Fields fields = {});
void setTag(const Field &tag);
void setContext(const char *name, Fields fields);

void guardTermination();
void shutDown();

}
