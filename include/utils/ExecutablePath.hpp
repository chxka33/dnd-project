#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <system_error>
#include <vector>

#if defined(__APPLE__)
#include <mach-o/dyld.h>
#elif defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace dnd::utils
{

    /// Директория, в которой расположен запущенный исполняемый файл программы.
    inline const std::filesystem::path &executableDir()
    {
        static const std::filesystem::path dir = []() -> std::filesystem::path
        {
            std::error_code ec;
#if defined(__APPLE__)
            std::uint32_t size = 0;
            (void)_NSGetExecutablePath(nullptr, &size);
            if (size > 0)
            {
                std::vector<char> buf(size);
                if (_NSGetExecutablePath(buf.data(), &size) == 0)
                {
                    const std::filesystem::path raw(buf.data());
                    const std::filesystem::path resolved = std::filesystem::weakly_canonical(raw, ec);
                    const std::filesystem::path &exe = (!ec && !resolved.empty()) ? resolved : raw;
                    if (exe.has_parent_path())
                        return exe.parent_path();
                }
            }
#elif defined(__linux__)
            const std::filesystem::path exe = std::filesystem::canonical("/proc/self/exe", ec);
            if (!ec && !exe.empty() && exe.has_parent_path())
                return exe.parent_path();
#elif defined(_WIN32)
            std::vector<wchar_t> buf(MAX_PATH, L'\0');
            while (buf.size() <= 32768)
            {
                const DWORD len = GetModuleFileNameW(nullptr, buf.data(), static_cast<DWORD>(buf.size()));
                if (len == 0)
                    break;
                if (len < buf.size())
                {
                    const std::filesystem::path raw(std::wstring(buf.data(), len));
                    const std::filesystem::path resolved = std::filesystem::weakly_canonical(raw, ec);
                    const std::filesystem::path &exe = (!ec && !resolved.empty()) ? resolved : raw;
                    if (exe.has_parent_path())
                        return exe.parent_path();
                    break;
                }
                buf.resize(buf.size() * 2, L'\0');
            }
#endif
            return std::filesystem::current_path(ec);
        }();
        return dir;
    }

    /// Преобразует относительный путь в путь относительно каталога бинарного файла;
    /// абсолютные пути возвращает без изменений.
    inline std::filesystem::path executableRelativePath(const std::filesystem::path &path)
    {
        if (path.empty())
            return executableDir();
        if (path.is_absolute())
            return path;
        return executableDir() / path;
    }

} // namespace dnd::utils
