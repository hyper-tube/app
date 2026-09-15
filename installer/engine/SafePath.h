#pragma once

#include <filesystem>
#include <string>

namespace setup {

bool safeRelativePath(const std::string &path);
bool safeDestination(const std::filesystem::path &path);

}
