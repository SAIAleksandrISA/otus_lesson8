#include "hasher.hpp"
#include <vector>
#include <algorithm>

Hasher::Hasher(size_t blockSize, const std::string& hashAlgo)
    : m_blockSize(blockSize), m_hashAlgo(hashAlgo)
{
    if (m_hashAlgo != "crc32" && m_hashAlgo != "crc16")
        throw std::runtime_error("Unsupported: " + m_hashAlgo);
}

uint64_t Hasher::hashNextBlock(std::ifstream& stream)
{
    std::vector<char> buffer(m_blockSize, '\0');
    stream.read(buffer.data(), m_blockSize);
    std::streamsize read = stream.gcount();

    if (read <= 0)
        return 0; 

    if (m_hashAlgo == "crc32")
    {
        m_crc32.reset();
        m_crc32.process_bytes(buffer.data(), m_blockSize);
        return m_crc32.checksum();
    }
    else if (m_hashAlgo == "crc16")
    {
        m_crc16.reset();
        m_crc16.process_bytes(buffer.data(), m_blockSize);
        return static_cast<uint64_t>(m_crc16.checksum());
    }
    else
    {
        throw std::runtime_error("Unsupported: " + m_hashAlgo);
    }

    return 0;
}