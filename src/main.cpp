#include <iostream>
#include <vector>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include "options.hpp"
#include "filescanner.hpp"
#include "filecomparer.hpp"

namespace po = boost::program_options;
namespace fs = boost::filesystem;

int main(int argc, char* argv[])
{
    Options options;
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "Produce help")
        ("directories,d", po::value<std::vector<fs::path>>(&options.m_directories)->multitoken(), "Dirs")
        ("exclude,e", po::value<std::vector<fs::path>>(&options.m_exclude_dirs)->multitoken(), "Exclude")
        ("level,l", po::value<int>(&options.m_scan_level)->default_value(-1), "Level")
        ("min-size,s", po::value<uintmax_t>(&options.m_min_file_size)->default_value(1), "Min size")
        ("mask,m", po::value<std::vector<std::string>>(&options.m_filename_masks)->multitoken(), "Mask")
        ("block-size,S", po::value<size_t>(&options.m_block_size)->default_value(4096), "Block size")
        ("hash-algo,H", po::value<std::string>(&options.m_hash_algo)->default_value("crc32"), "Algo");

    po::variables_map vm;
    try
    {
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);
    }
    catch (...) { return 1; }

    if (vm.count("help") || options.m_directories.empty())
    {
        std::cout << desc << std::endl;
        return 0;
    }

    FileScanner scanner(options);
    FileComparer comparer(options);
    comparer.findAndPrintDuplicates(scanner.scan());

    return 0;
}