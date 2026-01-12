#include "ZStd.hpp"

/*
#include <stdexcept>
#include <vector>
#include <zstd.h> // Make sure this is installed and properly included

void compress_zstd(const std::vector<char>& input, std::vector<char>& out_data)
{
  if (input.empty())
  {
    out_data.clear();
    return;
  }

  // Estimate the maximum compressed size
  size_t maxCompressedSize = ZSTD_compressBound(input.size());
  out_data.resize(maxCompressedSize); // allocate output buffer

  // Compress with default compression level (3)
  size_t compressedSize = ZSTD_compress(out_data.data(),   // destination buffer
                                        maxCompressedSize, // destination capacity
                                        input.data(),      // source buffer
                                        input.size(),      // source size
                                        22                 // compression level (1-22, higher = better)
  );

  if (ZSTD_isError(compressedSize))
  {
    throw std::runtime_error("ZSTD compression failed: " + std::string(ZSTD_getErrorName(compressedSize)));
  }

  // Resize output to actual compressed size
  out_data.resize(compressedSize);
}

void decompress_zstd(const std::vector<char>& in_data, std::vector<char>& out_data, size_t originalSize)
{
  out_data.resize(originalSize);
  size_t res = ZSTD_decompress(out_data.data(), originalSize, in_data.data(), in_data.size());
  // if (ZSTD_isError(res))
  // throw std::runtime_error("ZSTD decompression failed: " + std::string(ZSTD_getErrorName(res)));
  // if (res != originalSize)
  // throw std::runtime_error("ZSTD decompressed size mismatch");
}

ZStdCompressor::ZStdCompressor()
{
}

CompressData ZStdCompressor::compress(const CompressData& data)
{
  std::vector<char> ret;
  compress_zstd(data.getCharData(), ret);
  return ret;
}

CompressData ZStdCompressor::decompress(const CompressData& data)
{
  std::vector<char> ret;
  decompress_zstd(data.getCharData(), ret, data.getHeader().originalSize);
  return ret;
}

std::string ZStdCompressor::getId()
{
  return "zstd";
}
*/