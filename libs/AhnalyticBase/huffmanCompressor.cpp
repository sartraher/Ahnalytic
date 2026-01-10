#include "huffmanCompressor.hpp"

#include <algorithm>
#include <functional>
#include <queue>
#include <unordered_map>

/*
HuffmannCompressor::HuffmannCompressor(ModeE mode)
{
  sizeMode = mode;
}

std::vector<char> HuffmannCompressor::compress(std::vector<char> data)
{
  //
  switch (sizeMode)
  {
  case Byte1:
    return compressByteInternal(data);
    break;
  case Byte2:
  {
    size_t resSize = data.size() / sizeof(uint16_t) + ((data.size() % sizeof(uint16_t)) > 0 ? 1 : 0);

    std::vector<uint16_t> data16(resSize);
    memcpy(data16.data(), data.data(), data.size());

    return compressByteInternal(data16);
  }
  break;
  case Byte4:
  {
    size_t resSize = data.size() / sizeof(uint32_t) + ((data.size() % sizeof(uint32_t)) > 0 ? 1 : 0);

    std::vector<uint32_t> data32(resSize);
    memcpy(data32.data(), data.data(), data.size());

    return compressByteInternal(data32);
  }
  break;
  }

  return std::vector<char>();
}

std::string HuffmannCompressor::getId()
{
  return "huffman";
}
*/