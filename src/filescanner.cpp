#include "filescanner.hpp"
#include "utils.hpp"
#include <stack>
#include <algorithm>

namespace fs = boost::filesystem;

FileScanner::FileScanner(const Options& options) : m_options(options) {}

std::vector<FileInfo> FileScanner::scan()
{
    std::vector<FileInfo> result;
    // Используем стек для ручного управления рекурсией (обход директорий)
    std::stack<std::pair<fs::path, int>> dir_stack;

    for (const auto& dir : m_options.m_directories)
    {
        dir_stack.push({ dir, 0 });
    }

    while (!dir_stack.empty())
    {
        auto [current_path, current_depth] = dir_stack.top();
        dir_stack.pop();

        if (isExcluded(current_path)) continue;

        try
        {
            for (fs::directory_iterator it(current_path), end; it != end; ++it)
            {
                const fs::path& p = it->path();
                if (fs::is_regular_file(p))
                {
                    uintmax_t size = fs::file_size(p);
                    // Фильтрация по размеру и маске имени
                    if (size >= m_options.m_min_file_size && matchesMask(p))
                    {
                        result.push_back({ p, size });
                    }
                }
                else if (fs::is_directory(p) && (m_options.m_scan_level == -1 || current_depth < m_options.m_scan_level))
                {
                    dir_stack.push({ p, current_depth + 1 });
                }
            }
        }
        catch (...) {}
    }
    return result;
}

bool FileScanner::isExcluded(const fs::path& path) const
{
    try
    {
        // weakly_canonical обрабатывает даже отсутствующие пути
        fs::path absPath = fs::weakly_canonical(path);
        for (const auto& dir : m_options.m_exclude_dirs)
        {
            fs::path absExclude = fs::weakly_canonical(dir);
            auto pair = std::mismatch(absExclude.begin(), absExclude.end(), absPath.begin());
            if (pair.first == absExclude.end()) return true;
        }
    }
    catch (...) {}
    return false;
}

bool FileScanner::matchesMask(const fs::path& path) const
{
    if (m_options.m_filename_masks.empty()) return true;
    for (const auto& m : m_options.m_filename_masks)
    {
        if (utils::wildcard_match(path.filename().string(), m)) return true;
    }
    return false;
}