#include "Archive.h"

#include <windows.h>

#include <shlobj.h>

#include <fstream>
#include <string>

namespace {

constexpr wchar_t kCacheFolder[] = L"HyperTubeMusicSetup";
constexpr wchar_t kInterpreter[] = L"ht-music-setup.exe";
constexpr wchar_t kReadyMarker[] = L".ready";
constexpr wchar_t kStaleSuffix[] = L".stale";
constexpr wchar_t kTitle[] = L"HyperTube Music Setup";

void complain(const std::wstring &reason)
{
    MessageBoxW(nullptr, reason.c_str(), kTitle, MB_ICONERROR | MB_OK);
}

std::wstring widen(const std::string &text)
{
    if (text.empty())
        return {};
    const int size = MultiByteToWideChar(CP_UTF8, 0, text.data(), int(text.size()), nullptr, 0);
    std::wstring wide(size_t(size), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.data(), int(text.size()), wide.data(), size);
    return wide;
}

std::filesystem::path selfPath()
{
    std::wstring buffer(32768, L'\0');
    const DWORD used = GetModuleFileNameW(nullptr, buffer.data(), DWORD(buffer.size()));
    buffer.resize(used);
    return std::filesystem::path(buffer);
}

std::filesystem::path temporaryRoot()
{
    std::wstring buffer(32768, L'\0');
    const DWORD used = GetTempPathW(DWORD(buffer.size()), buffer.data());
    buffer.resize(used);
    return std::filesystem::path(buffer);
}

std::wstring argumentTail()
{
    const std::wstring line = GetCommandLineW();
    size_t at = 0;
    if (!line.empty() && line.front() == L'"') {
        at = line.find(L'"', 1);
        at = at == std::wstring::npos ? line.size() : at + 1;
    } else {
        at = line.find(L' ');
        at = at == std::wstring::npos ? line.size() : at;
    }
    while (at < line.size() && line[at] == L' ')
        ++at;
    return line.substr(at);
}

bool unpackRuntime(const std::filesystem::path &self, const setup::ArchiveFooter &footer,
                   const std::filesystem::path &into)
{
    std::ifstream source(self, std::ios::binary);
    if (!source) {
        complain(L"The setup file could not be opened.");
        return false;
    }

    if (!setup::verifyRegion(source, footer.runtimeOffset, footer.runtimeSize,
                             footer.runtimeDigest)) {
        complain(L"This setup file is damaged. Download it again.");
        return false;
    }

    setup::ArchiveReader reader;
    if (!reader.open(source, footer.runtimeOffset, footer.runtimeSize)) {
        complain(widen("The setup file could not be read: " + reader.error()));
        return false;
    }

    std::error_code code;
    std::filesystem::create_directories(into, code);
    if (!reader.extract(into, nullptr)) {
        complain(widen("Setup could not prepare itself: " + reader.error()));
        return false;
    }
    return true;
}

bool discard(const std::filesystem::path &directory)
{
    std::filesystem::path aside = directory;
    aside += kStaleSuffix;

    std::error_code code;
    std::filesystem::remove_all(aside, code);
    std::filesystem::rename(directory, aside, code);
    if (code)
        return false;
    std::filesystem::remove_all(aside, code);
    return !code;
}

void collectStale(const std::filesystem::path &root, const std::filesystem::path &keep)
{
    std::error_code code;
    for (const auto &entry : std::filesystem::directory_iterator(root, code)) {
        if (entry.path() != keep && entry.is_directory(code))
            discard(entry.path());
    }
}

int launch(const std::filesystem::path &interpreter, const std::filesystem::path &self,
           bool waitForExit)
{
    std::wstring line = L'"' + interpreter.wstring() + L"\" --origin \"" + self.wstring() + L'"';
    const std::wstring tail = argumentTail();
    if (!tail.empty())
        line += L' ' + tail;

    STARTUPINFOW startup {};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process {};

    if (!CreateProcessW(interpreter.wstring().c_str(), line.data(), nullptr, nullptr, FALSE, 0,
                        nullptr, interpreter.parent_path().wstring().c_str(), &startup, &process)) {
        complain(L"Setup could not start.");
        return 1;
    }

    DWORD status = 0;
    if (waitForExit) {
        WaitForSingleObject(process.hProcess, INFINITE);
        GetExitCodeProcess(process.hProcess, &status);
    }
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    return int(status);
}

}

int APIENTRY wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int)
{
    const std::filesystem::path self = selfPath();

    setup::ArchiveFooter footer;
    if (!setup::ArchiveFooter::readFrom(self, footer)) {
        complain(L"This file is not a HyperTube Music setup file.");
        return 1;
    }

    const std::string key = footer.version + "-" + setup::hex(footer.runtimeDigest).substr(0, 16);
    const std::filesystem::path root = temporaryRoot() / kCacheFolder;
    const std::filesystem::path cache = root / widen(key);
    const std::filesystem::path marker = cache / kReadyMarker;
    const std::filesystem::path interpreter = cache / kInterpreter;

    collectStale(root, cache);

    std::error_code code;
    if (!std::filesystem::exists(marker, code) || !std::filesystem::exists(interpreter, code)) {
        std::filesystem::remove_all(cache, code);
        if (!unpackRuntime(self, footer, cache))
            return 1;
        std::ofstream(marker, std::ios::binary).put('\n');
    }

    SetCurrentDirectoryW(cache.wstring().c_str());
    const bool waits = footer.payloadSize > 0;
    const int status = launch(interpreter, self, waits);
    if (waits) {
        SetCurrentDirectoryW(root.wstring().c_str());
        discard(cache);
    }
    return status;
}
