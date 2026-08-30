#include "hasher.hpp"
#include <vector>
#include <algorithm>

Hasher::Hasher(size_t blockSize, const std::string& hashAlgo)
    : m_blockSize(blockSize), m_hashAlgo(hashAlgo) {
}

uint64_t Hasher::hashNextBlock(std::ifstream& stream)
{
    std::vector<char> buffer(m_blockSize, '\0');
    stream.read(buffer.data(), m_blockSize);
    std::streamsize read = stream.gcount();

    if (read <= 0) return 0;

    m_crc32_calculator.reset();
    m_crc32_calculator.process_bytes(buffer.data(), m_blockSize);
    return m_crc32_calculator.checksum();
}