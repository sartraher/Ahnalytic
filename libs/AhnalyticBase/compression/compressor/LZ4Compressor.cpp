#include "LZ4Compressor.hpp"

/*
#include "AhnalyticBase/compression/LZ4Helper.hpp"

Lz4Compressor::Lz4Compressor()
{
}

CompressData Lz4Compressor::compress(const CompressData& data)
{
  std::vector<char> cmpData;
  LZ4Helper::compressLZ4(data.getCharData(), cmpData);
  return std::move(cmpData);
}

CompressData Lz4Compressor::decompress(const CompressData& data)
{
  std::vector<char> cmpData;
  LZ4Helper::decompressLZ4(data.getCharData(), cmpData, data.getHeader().originalSize);
  return std::move(cmpData);
}

std::string Lz4Compressor::getId()
{
  return "lz4";
}
*/