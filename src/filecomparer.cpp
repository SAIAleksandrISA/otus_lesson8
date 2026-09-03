#include "filecomparer.hpp"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <vector>
#include <map>
#include <list>
#include <memory>

struct FileStreamInfo
{
    boost::filesystem::path m_path;
    std::ifstream m_stream;
    bool m_is_candidate_for_duplicate;
    bool m_reached_eof;
    uint64_t m_last_block_hash;

    FileStreamInfo(const boost::filesystem::path& p, std::ifstream&& stream)
        : m_path(p), m_stream(std::move(stream)), m_is_candidate_for_duplicate(true), m_reached_eof(false), m_last_block_hash(0) {
    }

    FileStreamInfo(FileStreamInfo&& other) noexcept = default;
    FileStreamInfo& operator=(FileStreamInfo&& other) noexcept = default;

    FileStreamInfo(const FileStreamInfo&) = delete;
    FileStreamInfo& operator=(const FileStreamInfo&) = delete;
};

struct DuplicateGroup
{
    std::list<FileStreamInfo> m_files;
    bool m_is_resolved;

    DuplicateGroup(std::list<FileStreamInfo>&& files)
        : m_files(std::move(files)), m_is_resolved(false) {
    }

    DuplicateGroup(DuplicateGroup&& other) noexcept = default;
    DuplicateGroup& operator=(DuplicateGroup&& other) noexcept = default;

    DuplicateGroup(const DuplicateGroup&) = delete;
    DuplicateGroup& operator=(const DuplicateGroup&) = delete;
};

FileComparer::FileComparer(const Options& options)
    : m_options(options), m_hasher(options.m_block_size, options.m_hash_algo)
{
}

void FileComparer::findAndPrintDuplicates(const std::vector<FileInfo>& files)
{
    auto sizeGroups = groupBySize(files);

    for (auto const& pair : sizeGroups)
    {
        const std::vector<boost::filesystem::path>& paths = pair.second;

        if (paths.size() < 2)
        {
            continue;
        }

        std::list<FileStreamInfo> initial_file_list;
        for (const auto& p : paths)
        {
            std::ifstream stream(p.c_str(), std::ios::binary);
            if (!stream.is_open())
            {
                continue;
            }
            initial_file_list.emplace_back(p, std::move(stream));
        }

        if (initial_file_list.size() < 2)
        {
            continue;
        }

        std::vector<DuplicateGroup> active_groups;
        active_groups.emplace_back(std::move(initial_file_list));

        std::vector<DuplicateGroup> final_results;

        while (!active_groups.empty())
        {
            std::vector<DuplicateGroup> next_round_groups;
            bool any_progress_this_round = false;

            for (auto& current_group : active_groups)
            {
                if (current_group.m_is_resolved)
                {
                    continue;
                }

                std::map<uint64_t, std::list<FileStreamInfo>> partition_map;
                std::list<FileStreamInfo> files_that_ended_this_round_in_group;
                bool group_made_progress = false;

                auto it = current_group.m_files.begin();
                while (it != current_group.m_files.end())
                {
                    FileStreamInfo& fsi = *it;

                    if (!fsi.m_is_candidate_for_duplicate || fsi.m_reached_eof)
                    {
                        it = current_group.m_files.erase(it);
                        continue;
                    }

                    if (fsi.m_stream.eof() || fsi.m_stream.fail())
                    {
                        fsi.m_reached_eof = true;
                        files_that_ended_this_round_in_group.push_back(std::move(*it));
                        it = current_group.m_files.erase(it);
                        group_made_progress = true;
                        continue;
                    }

                    uint64_t h = m_hasher.hashNextBlock(fsi.m_stream);
                    bool stream_now_eof = fsi.m_stream.eof() || fsi.m_stream.fail();

                    if (h != 0 || !stream_now_eof)
                    {
                        fsi.m_last_block_hash = h;
                        partition_map[h].push_back(std::move(*it));
                        it = current_group.m_files.erase(it);
                        group_made_progress = true;
                    }
                    else
                    {
                        fsi.m_reached_eof = true;
                        fsi.m_last_block_hash = h;
                        files_that_ended_this_round_in_group.push_back(std::move(*it));
                        it = current_group.m_files.erase(it);
                        group_made_progress = true;
                    }
                }

                if (!group_made_progress && !current_group.m_files.empty())
                {
                    int active_candidates_count = 0;
                    for (const auto& fsi : current_group.m_files)
                    {
                        if (fsi.m_is_candidate_for_duplicate && !fsi.m_reached_eof)
                        {
                            active_candidates_count++;
                        }
                    }
                    if (active_candidates_count <= 1)
                    {
                        current_group.m_is_resolved = true;
                    }
                    continue;
                }

                if (group_made_progress)
                {
                    any_progress_this_round = true;
                }

                for (auto& fsi : files_that_ended_this_round_in_group)
                {
                    partition_map[fsi.m_last_block_hash].push_back(std::move(fsi));
                }

                for (auto& part_pair : partition_map)
                {
                    std::list<FileStreamInfo>& partitioned_files_list = part_pair.second;

                    if (partitioned_files_list.size() > 1)
                    {
                        bool all_partition_files_ended_eof = true;
                        for (const auto& fsi : partitioned_files_list)
                        {
                            if (!fsi.m_reached_eof)
                            {
                                all_partition_files_ended_eof = false;
                                break;
                            }
                        }

                        if (all_partition_files_ended_eof)
                        {
                            DuplicateGroup resolved_group(std::move(partitioned_files_list));
                            resolved_group.m_is_resolved = true;
                            final_results.push_back(std::move(resolved_group));
                        }
                        else
                        {
                            std::list<FileStreamInfo> continuing_candidate_files;
                            for (auto& fsi : partitioned_files_list)
                            {
                                if (!fsi.m_reached_eof)
                                {
                                    continuing_candidate_files.push_back(std::move(fsi));
                                }
                            }
                            if (continuing_candidate_files.size() > 1)
                            {
                                next_round_groups.emplace_back(std::move(continuing_candidate_files));
                            }
                        }
                    }
                }
            }

            active_groups = std::move(next_round_groups);
            if (!any_progress_this_round)
            {
                break;
            }
        }

        for (const auto& group : final_results)
        {
            if (group.m_is_resolved && group.m_files.size() > 1)
            {
                for (const auto& fsi : group.m_files)
                {
                    std::cout << fsi.m_path.string() << std::endl;
                }
                std::cout << std::endl;
            }
        }
    }
}

std::map<uintmax_t, std::vector<boost::filesystem::path>> FileComparer::groupBySize(const std::vector<FileInfo>& files) const
{
    std::map<uintmax_t, std::vector<boost::filesystem::path>> groups;
    for (const auto& f : files)
    {
        groups[f.m_size].push_back(f.m_path);
    }
    return groups;
}