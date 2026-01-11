#include "bsc.hpp"

#include "libbsc.h" // from libbsc
#include <cstring>
#include <stdexcept>
#include <vector>

void compress_bsc(const std::vector<char>& input, std::vector<char>& out_data)
{
  if (input.empty())
  {
    out_data.clear();
    return;
  }

  if (bsc_init(LIBBSC_FEATURE_FASTMODE) != LIBBSC_NO_ERROR)
  {
    throw std::runtime_error("libbsc initialization failed");
  }

  const int inputSize = static_cast<int>(input.size());
  const int maxCompressedSize = inputSize + inputSize / 10 + 1024;

  // std::vector<unsigned char> tempOut(maxCompressedSize);
  out_data.resize(maxCompressedSize);

  int compressedSize =
      bsc_compress(reinterpret_cast<const unsigned char*>(input.data()), (unsigned char*)out_data.data(), inputSize, LIBBSC_DEFAULT_LZPHASHSIZE,
                   LIBBSC_DEFAULT_LZPMINLEN, LIBBSC_DEFAULT_BLOCKSORTER, LIBBSC_DEFAULT_CODER, LIBBSC_DEFAULT_FEATURES);

  if (compressedSize <= 0)
  {
    throw std::runtime_error("libbsc compression failed with error code: " + std::to_string(compressedSize));
  }

  // Add original size as header (first 4 bytes)
  out_data.resize(sizeof(uint32_t) + compressedSize);
  // std::memcpy(out_data.data(), &inputSize, sizeof(uint32_t));  // Write original size
  // std::memcpy(out_data.data() + sizeof(uint32_t), tempOut.data(), compressedSize);
}

void decompress_bsc(const std::vector<char>& in_data, std::vector<char>& out_data, uint32_t originalSize)
{
  if (in_data.size() < sizeof(uint32_t))
  {
    throw std::runtime_error("BSC compressed data too small to contain header");
  }

  if (bsc_init(LIBBSC_FEATURE_FASTMODE) != LIBBSC_NO_ERROR)
  {
    throw std::runtime_error("libbsc initialization failed");
  }

  // uint32_t originalSize;
  // std::memcpy(&originalSize, in_data.data(), sizeof(uint32_t));  // Read original size

  std::vector<unsigned char> tempOut(originalSize);

  int status = bsc_decompress(reinterpret_cast<const unsigned char*>(in_data.data()), static_cast<int>(in_data.size()), tempOut.data(),
                              static_cast<int>(originalSize), LIBBSC_DEFAULT_FEATURES);

  if (status != LIBBSC_NO_ERROR)
  {
    throw std::runtime_error("libbsc decompression failed with code: " + std::to_string(status));
  }

  out_data.assign(reinterpret_cast<char*>(tempOut.data()), reinterpret_cast<char*>(tempOut.data()) + originalSize);
}

BscCompressor::BscCompressor()
{
}

CompressData BscCompressor::compress(const CompressData& data)
{
  std::vector<char> ret;
  compress_bsc(data.getCharData(), ret);
  return ret;
}

CompressData BscCompressor::decompress(const CompressData& data)
{
  std::vector<char> ret;
  decompress_bsc(data.getCharData(), ret, data.getHeader().originalSize);
  return ret;
}

std::string BscCompressor::getId()
{
  return "bsc";
}