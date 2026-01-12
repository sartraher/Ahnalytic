#include "LZ4Helper.hpp"

/*
#include <lz4.h>

bool LZ4Helper::compressLZ4(const std::vector<char>& input, std::vector<char>& output)
{
  int maxCompressedSize = LZ4_compressBound(static_cast<int>(input.size()));
  output.resize(maxCompressedSize);

  int compressedSize = LZ4_compress_default(input.data(), output.data(), static_cast<int>(input.size()), maxCompressedSize);

  if (compressedSize <= 0)
  {
    output.clear();
    return false;
  }

  output.resize(compressedSize); // Trim to actual size
  return true;
}

bool LZ4Helper::decompressLZ4(const std::vector<char>& input, std::vector<char>& output, int originalSize)
{
  output.resize(originalSize);

  int decompressedSize = LZ4_decompress_safe(input.data(), output.data(), static_cast<int>(input.size()), originalSize);

  if (decompressedSize < 0)
  {
    output.clear();
    return false;
  }

  return true;
}
*/