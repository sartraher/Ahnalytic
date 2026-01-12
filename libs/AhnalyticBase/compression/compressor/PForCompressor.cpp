#include "pforCompressor.hpp"

/*
#include "headers/codecfactory.h"
#include "headers/deltautil.h"

std::vector<uint32_t> decompress_pfor(const std::vector<uint32_t>& in_data, size_t originalCount, const std::string& codecId)
{
  using namespace FastPForLib;
  CODECFactory factory;
  IntegerCODEC& codec = *factory.getFromName(codecId);

  size_t compSize = static_cast<uint32_t>(in_data.size());

  std::vector<uint32_t> out(originalCount * 2);
  size_t outInts = static_cast<uint32_t>(originalCount);

  codec.decodeArray(in_data.data(), compSize, out.data(), outInts);

  out.resize(outInts); // Adjust size if codec changes output length

  return out;
}

PForCompressor::PForCompressor(std::string id)
{
  codecId = id;
}

CompressData PForCompressor::compress(const CompressData& data)
{
  using namespace FastPForLib;
  CODECFactory factory;
  IntegerCODEC& codec = *factory.getFromName(codecId);

  // size_t inputCount = data.size() / sizeof(uint32_t) + ((data.size() % sizeof(uint32_t)) > 0 ? 1 : 0);

  std::vector<uint32_t> inList = data.getUint32Data();
  // memcpy(inList.data(), data.data(), data.size());

  std::vector<uint32_t> compressed(inList.size() * 10); // Oversize buffer
  size_t compressedSize = compressed.size();
  codec.encodeArray(inList.data(), inList.size(), compressed.data(), compressedSize);
  compressed.resize(compressedSize);

  std::vector<uint32_t> test = decompress_pfor(compressed, inList.size(), codecId);

  return compressed;
}

CompressData PForCompressor::decompress(const CompressData& data)
{
  return decompress_pfor(data.getUint32Data(), data.getHeader().originalSize / sizeof(uint32_t), codecId);
}

std::string PForCompressor::getId()
{
  return codecId;
}
*/