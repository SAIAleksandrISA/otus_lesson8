#pragma once
#include <vector>
#include <boost/filesystem.hpp>
#include "options.hpp"

struct FileInfo
{
    boost::filesystem::path m_path;
    uintmax_t m_size;
};

class FileScanner
{
public:
    explicit FileScanner(const Options& options);
    std::vector<FileInfo> scan();

protected:
    bool isExcluded(const boost::filesystem::path& path) const;
    bool matchesMask(const boost::filesystem::path& path) const;

private:
    const Options& m_options;
};