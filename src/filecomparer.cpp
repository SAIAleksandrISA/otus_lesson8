#include "filecomparer.hpp"
#include <iostream>
#include <fstream>

struct FileContext
{
    boost::filesystem::path m_path;
    std::ifstream m_stream;
    bool m_active = true;
};

FileComparer::FileComparer(const Options& options)
    : m_options(options), m_hasher(options.m_block_size, options.m_hash_algo) {
}

void FileComparer::findAndPrintDuplicates(const std::vector<FileInfo>& files)
{
    auto sizeGroups = groupBySize(files);
    for (auto& [size, paths] : sizeGroups)
    {
        if (paths.size() < 2) continue;

        std::vector<FileContext> active;
        for (const auto& p : paths)
        {
            active.push_back({ p, std::ifstream(p.c_str(), std::ios::binary), true});
        }

        while (true)
        {
            std::map<uint64_t, std::vector<size_t>> hashGroups;
            bool anyActive = false;

            for (size_t i = 0; i < active.size(); ++i)
            {
                if (!active[i].m_active) continue;

                uint64_t h = m_hasher.hashNextBlock(active[i].m_stream);

                if (h != 0)
                {
                    hashGroups[h].push_back(i);
                    anyActive = true;
                }
            }

            if (!anyActive) break;

            for (auto& [hash, indices] : hashGroups)
            {
                if (indices.size() == 1)
                {
                    active[indices[0]].m_active = false;
                }
            }
        }

        std::vector<boost::filesystem::path> dups;
        for (const auto& f : active)
        {
            if (f.m_active) dups.push_back(f.m_path);
        }

        if (dups.size() > 1)
        {
            for (const auto& p : dups) std::cout << p.string() << std::endl;
            std::cout << std::endl;
        }
    }
}

std::map<uintmax_t, std::vector<boost::filesystem::path>> FileComparer::groupBySize(const std::vector<FileInfo>& files) const
{
    std::map<uintmax_t, std::vector<boost::filesystem::path>> groups;
    for (const auto& f : files) groups[f.m_size].push_back(f.m_path);
    return groups;
}