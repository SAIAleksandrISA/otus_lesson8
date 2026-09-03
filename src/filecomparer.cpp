#include "FileComparer.hpp"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <vector>
#include <map>
#include <limits>

struct FileStreamInfo
{
    boost::filesystem::path m_path;
    std::ifstream m_stream;
    bool m_is_candidate_for_duplicate = true;
    bool m_reached_eof = false; 
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

        std::vector<FileStreamInfo> file_stream_infos;
        file_stream_infos.reserve(paths.size());
        for (const auto& p : paths)
            file_stream_infos.push_back({ p, std::ifstream(p.c_str(), std::ios::binary), true });

        while (true)
        {
            bool progress_made_in_round = false;
            std::map<uint64_t, std::vector<size_t>> hash_to_candidate_indices;
            std::vector<size_t> files_that_ended_this_round; 

            for (size_t i = 0; i < file_stream_infos.size(); ++i)
            {
                if (!file_stream_infos[i].m_is_candidate_for_duplicate || file_stream_infos[i].m_reached_eof)
                {
                    continue;
                }

                if (file_stream_infos[i].m_stream.eof() || file_stream_infos[i].m_stream.fail())
                {
                    file_stream_infos[i].m_reached_eof = true;
                    continue;
                }

                uint64_t h = m_hasher.hashNextBlock(file_stream_infos[i].m_stream);

                bool stream_now_eof = file_stream_infos[i].m_stream.eof() || file_stream_infos[i].m_stream.fail();

                if (h != 0 || !stream_now_eof)
                {
                    hash_to_candidate_indices[h].push_back(i);
                    progress_made_in_round = true;
                }
                else
                {
                    file_stream_infos[i].m_reached_eof = true;
                    files_that_ended_this_round.push_back(i);
                    progress_made_in_round = true;
                }
            }

            if (!progress_made_in_round)
            {
                break;
            }
            for (auto& fsi : file_stream_infos)
            {
                fsi.m_is_candidate_for_duplicate = false;
            }

            bool any_candidates_remain_for_next_round = false;

            for (const auto& pair : hash_to_candidate_indices)
            {
                const std::vector<size_t>& current_indices = pair.second;

                if (current_indices.size() > 1)
                {
                    bool all_files_in_group_ended = true;
                    for (size_t original_idx : current_indices)
                    {
                        if (!file_stream_infos[original_idx].m_reached_eof)
                        {
                            all_files_in_group_ended = false;
                            break;
                        }
                    }

                    if (all_files_in_group_ended)
                    {
                        for (size_t original_idx : current_indices)
                        {
                            file_stream_infos[original_idx].m_is_candidate_for_duplicate = true;
                            any_candidates_remain_for_next_round = true;
                        }
                    }
                    else
                    {
                        for (size_t original_idx : current_indices) // Используем current_indices
                        {
                            if (!file_stream_infos[original_idx].m_reached_eof)
                            {
                                file_stream_infos[original_idx].m_is_candidate_for_duplicate = true;
                                any_candidates_remain_for_next_round = true;
                            }
                        }
                    }
                }
            }
            if (!any_candidates_remain_for_next_round)
            {
                break;
            }
        } 

        std::vector<boost::filesystem::path> dups;

        bool all_files_in_this_size_group_ended = true;
        for (const auto& fsi : file_stream_infos) {
            if (!fsi.m_reached_eof) {
                all_files_in_this_size_group_ended = false;
                break;
            }
        }

        if (all_files_in_this_size_group_ended)
        {
            for (size_t i = 0; i < file_stream_infos.size(); ++i)
            {
                if (file_stream_infos[i].m_is_candidate_for_duplicate)
                {
                    dups.push_back(file_stream_infos[i].m_path);
                }
            }
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