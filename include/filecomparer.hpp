#pragma once
#include <vector>
#include <map>
#include "options.hpp"
#include "filescanner.hpp"
#include "hasher.hpp"

class FileComparer
{
public:
    explicit FileComparer(const Options& options);
    void findAndPrintDuplicates(const std::vector<FileInfo>& files);

protected:
    std::map<uintmax_t, std::vector<boost::filesystem::path>> groupBySize(const std::vector<FileInfo>& files) const;

private:
    Options m_options;
    Hasher m_hasher;
};