#include "lzma.hpp"

#include "LzmaLib.h" // From the official LZMA SDK
#include <cstring>
#include <stdexcept>
#include <vector>

void compress_memory(const std::vector<char>& input, std::vector<char>& out_data)
{
  const size_t propsSize = LZMA_PROPS_SIZE;
  std::vector<unsigned char> props(propsSize);

  size_t destLen = input.size() + input.size() / 3 + 128; // estimated size
  std::vector<unsigned char> compressed(destLen);

  size_t outPropsSize = propsSize;

  int res =
      LzmaCompress(compressed.data() + propsSize, &destLen, reinterpret_cast<const unsigned char*>(input.data()), input.size(), props.data(), &outPropsSize,
                   5,       // level: 0 <= level <= 9
                   1 << 24, // dictionary size (16MB)
                   3,       // lc
                   0,       // lp
                   2,       // pb
                   32,      // fb
                   1        // numThreads
      );

  if (res != SZ_OK)
  {
    throw std::runtime_error("LZMA compression failed with error code: " + std::to_string(res));
  }

  // Combine props + compressed data
  std::vector<char> result;
  result.reserve(propsSize + destLen);

  result.insert(result.end(), props.begin(), props.end());
  result.insert(result.end(), compressed.begin() + propsSize, compressed.begin() + propsSize + destLen);

  std::swap(result, out_data);
}

void decompress_lzma(const std::vector<char>& in_data, std::vector<char>& out_data, size_t originalSize)
{
  if (in_data.size() < LZMA_PROPS_SIZE)
  {
  }

  std::vector<unsigned char> props(LZMA_PROPS_SIZE);
  memcpy(props.data(), in_data.data(), LZMA_PROPS_SIZE);
  const unsigned char* compressedData = reinterpret_cast<const unsigned char*>(in_data.data() + LZMA_PROPS_SIZE);
  size_t compSize = in_data.size() - LZMA_PROPS_SIZE;

  out_data.resize(originalSize);
  size_t outSize = originalSize;
  int res = LzmaUncompress(reinterpret_cast<unsigned char*>(out_data.data()), &outSize, compressedData, &compSize, props.data(), LZMA_PROPS_SIZE);

  if (res != SZ_OK || outSize != originalSize)
  {
  }
}

LZMACompressor::LZMACompressor()
{
}

CompressData LZMACompressor::compress(const CompressData& data)
{
  std::vector<char> compressed;
  compress_memory(data.getCharData(), compressed);
  return compressed;
}

CompressData LZMACompressor::decompress(const CompressData& data)
{
  std::vector<char> ret;
  decompress_lzma(data.getCharData(), ret, data.getHeader().originalSize);
  return ret;
}

std::string LZMACompressor::getId()
{
  return "lzma";
}