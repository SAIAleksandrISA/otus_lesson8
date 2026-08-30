#pragma once
#include <vector>
#include <string>
#include <boost/filesystem.hpp>

struct Options
{
    std::vector<boost::filesystem::path> m_directories;
    std::vector<boost::filesystem::path> m_exclude_dirs;
    int m_scan_level = -1;
    uintmax_t m_min_file_size = 1;
    std::vector<std::string> m_filename_masks;
    size_t m_block_size = 4096;
    std::string m_hash_algo = "crc32";
};