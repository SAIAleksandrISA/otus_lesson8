#pragma once
#include <fstream>
#include <vector>
#include <string>
#include <boost/crc.hpp>

class Hasher
{
public:
    Hasher(size_t blockSize, const std::string& hashAlgo);
    uint64_t hashNextBlock(std::ifstream& stream);

private:
    size_t m_blockSize;
    std::string m_hashAlgo;
    boost::crc_32_type m_crc32;
    boost::crc_16_type m_crc16;
};