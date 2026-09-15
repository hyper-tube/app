#include "SafePath.h"

#include <windows.h>

#include <algorithm>
#include <cctype>

namespace setup {

bool safeRelativePath(const std::string &path)
{
    if (path.empty() || path.front() == '/' || path.find_first_of("\\:<>\"|?*") != std::string::npos
        || std::any_of(path.begin(), path.end(), [](unsigned char c) { return c < 32; })) {
        return false;
    }
    std::size_t at = 0;
    while (at < path.size()) {
        const auto end = path.find('/', at);
        const std::string part = path.substr(at, end == std::string::npos ? end : end - at);
        if (part.empty() || part == "." || part == ".." || part.back() == '.' || part.back() == ' ')
            return false;
        std::string base = part.substr(0, part.find('.'));
        std::transform(base.begin(), base.end(), base.begin(),
                       [](unsigned char c) { return char(std::toupper(c)); });
        if (base == "CON" || base == "PRN" || base == "AUX" || base == "NUL"
            || (base.size() == 4 && (base.starts_with("COM") || base.starts_with("LPT"))
                && base[3] >= '1' && base[3] <= '9')) {
            return false;
        }
        if (end == std::string::npos)
            return true;
        at = end + 1;
    }
    return false;
}

bool safeDestination(const std::filesystem::path &path)
{
    auto probe = path.lexically_normal();
    while (!probe.empty()) {
        const DWORD attributes = GetFileAttributesW(probe.c_str());
        if (attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_REPARSE_POINT))
            return false;
        const auto parent = probe.parent_path();
        if (parent == probe)
            break;
        probe = parent;
    }
    return true;
}

}
