#ifndef lz4helper_hpp__
#define lz4helper_hpp__

#include "export.hpp"

#include <vector>

class DLLEXPORT LZ4Helper
{
public:
  static bool compressLZ4(const std::vector<char>& input, std::vector<char>& output);
  static bool decompressLZ4(const std::vector<char>& input, std::vector<char>& output, int originalSize);
};

#endif