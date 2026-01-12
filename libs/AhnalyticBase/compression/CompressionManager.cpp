#include "CompressionManager.hpp"

#include "AhnalyticBase/compression/modifier/DeltaModifier.hpp"
#include "AhnalyticBase/compression/modifier/NoneModifier.hpp"

#include "AhnalyticBase/compression/compressor/BSC.hpp"
#include "AhnalyticBase/compression/compressor/GZip.hpp"
#include "AhnalyticBase/compression/compressor/LZ4Compressor.hpp"
#include "AhnalyticBase/compression/compressor/LZMA.hpp"
#include "AhnalyticBase/compression/compressor/PForCompressor.hpp"
#include "AhnalyticBase/compression/compressor/VSEncode.hpp"
#include "AhnalyticBase/compression/compressor/ZStd.hpp"

#include "AhnalyticBase/helper/Diagnostic.hpp"

#include <cstring> 
#include <algorithm>

CompressionManager::CompressionManager()
{
  registerModifier(ModAlgosE::None, new NoneModifier());
  registerModifier(ModAlgosE::Delta, new DeltaModifier());

  // registerCompressor(new PForCompressor("fastbinarypacking8"));
  // registerCompressor(new PForCompressor("fastbinarypacking16"));
  // registerCompressor(new PForCompressor("fastbinarypacking32"));
  // registerCompressor(new PForCompressor("BP32"));
  // registerCompressor(CompressionAlgosE::VSEncoding, new PForCompressor("vsencoding"));
  // registerCompressor(CompressionAlgosE::VSEncoding, new VSEncodeCompressor());
  // registerCompressor(new PForCompressor("fastpfor128"));
  // registerCompressor(new PForCompressor("fastpfor256"));
  // registerCompressor(new PForCompressor("simdfastpfor128"));
  // registerCompressor(new PForCompressor("simdfastpfor256"));
  // registerCompressor(new PForCompressor("simplepfor"));
  // registerCompressor(new PForCompressor("simdsimplepfor"));
  // registerCompressor(new PForCompressor("pfor"));
  // registerCompressor(new PForCompressor("simdpfor"));
  // registerCompressor(new PForCompressor("pfor2008"));
  // registerCompressor(new PForCompressor("simdnewpfor"));
  // registerCompressor(new PForCompressor("newpfor"));
  // registerCompressor(new PForCompressor("optpfor"));
  //registerCompressor(CompressionAlgosE::SIMDoptpFor, new PForCompressor("simdoptpfor"));
  // registerCompressor(CompressionAlgosE::Test,new PForCompressor("varint"));
  // registerCompressor(CompressionAlgosE::Test, new PForCompressor("vbyte"));
  // registerCompressor(new PForCompressor("maskedvbyte"));
  // registerCompressor(new PForCompressor("streamvbyte"));
  //registerCompressor(CompressionAlgosE::Test, new PForCompressor("varintgb"));
  /*
  registerCompressor(new PForCompressor("simple16;"));
  registerCompressor(new PForCompressor("simple9"));
  registerCompressor(new PForCompressor("simple9_rle);"));
  registerCompressor(new PForCompressor("simple8b"));
  registerCompressor(new PForCompressor("simple8b_rle"));
  */
  // registerCompressor(new PForCompressor("simdbinarypacking"));
  // registerCompressor(new PForCompressor("simdgroupsimple"));
  // registerCompressor(new PForCompressor("simdgroupsimple_ringbuf"));
  // registerCompressor(CompressionAlgosE::LZ4, new Lz4Compressor());
  // registerCompressor(CompressionAlgosE::GZip, new GZipCompressor());
  registerCompressor(CompressionAlgosE::LZMA, new LZMACompressor());
  //registerCompressor(CompressionAlgosE::ZStd, new ZStdCompressor());
  registerCompressor(CompressionAlgosE::BSC, new BscCompressor());

  // registerCompressor(new HuffmannCompressor(HuffmannCompressor::Byte1));
  // registerCompressor(new HuffmannCompressor(HuffmannCompressor::Byte2));
  // registerCompressor(new HuffmannCompressor(HuffmannCompressor::Byte4));
}

CompressionManager::~CompressionManager()
{
}

CompressData CompressionManager::compress(const CompressData& data, Diagnostic* dia, std::vector<ModAlgosE> modFilter, std::vector<CompressionAlgosE> cmpFilter,
                                          bool force)
{
  CompressData ret = data;

  // uint32_t amount = 0;
  std::vector<CompressDataHeader> modComps;
  std::vector<CompressionAlgosE> alternatives;
  std::vector<CompressDataHeader> headers;

  bool first = false;
  bool compressed = false;
  do
  {
    uint32_t originalSize = (uint32_t)ret.getCharSize();

    ModAlgosE curMod;
    CompressionAlgosE curComp;

    compressed = false;

    for (auto iter = modifiers.begin(); iter != modifiers.end(); iter++)
    {
      if (modFilter.size() > 0 && std::find(modFilter.begin(), modFilter.end(), iter->first) == modFilter.end())
        continue;

      ModifierI* modifier = iter->second;

      CompressData mod = modifier->modify(data);

      for (auto cmpIter = compressors.begin(); cmpIter != compressors.end(); cmpIter++)
      {
        if (cmpFilter.size() > 0 && std::find(cmpFilter.begin(), cmpFilter.end(), cmpIter->first) == cmpFilter.end())
          continue;

        CompressorI* compressor = cmpIter->second;

        CompressData comp = compressor->compress(mod);
        if (comp.getCharSize() < ret.getCharSize() || (!first && force))
        {
          ret = std::move(comp);
          compressed = true;
          first = true;
          curMod = iter->first;
          curComp = cmpIter->first;
          alternatives.clear();
        }
        else if (comp.getCharSize() == ret.getCharSize())
          alternatives.push_back(cmpIter->first);
      }
    }

    if (compressed)
    {
      CompressDataHeader header;
      header.modifier = curMod;
      header.algo = curComp;
      header.originalSize = originalSize;
      ret.setHeader(header);
      headers.push_back(header);
    }
  } while (compressed);

  /*
  std::vector<uint32_t> header;
  header.push_back(amount);
  header.insert(header.end(), modComps.begin(), modComps.end());

  std::vector<char> ret1(header.size() * sizeof(uint32_t));
  memcpy(ret1.data(), header.data(), header.size() * sizeof(uint32_t));
  ret1.insert(ret1.end(), ret.begin(), ret.end());
  */

  if (dia != nullptr)
  {
    std::string compression;

    for (int index = 0; index < headers.size(); index++)
    {
      if (index > 1)
        compression += ", ";
      compression += getModifierName(headers.at(index).modifier) + "<->" + getCompressorName(headers.at(index).algo);
    }

    std::string alternativesStr;
    for (int index = 0; index < alternatives.size(); index++)
    {
      if (index > 0)
        alternativesStr += ", ";
      alternativesStr += getCompressorName(alternatives.at(index));
    }

    dia->setCompression(data.getCharSize(), ret.getCharSize(), compression, alternativesStr);
  }

  return ret;
}

CompressData CompressionManager::decompress(const CompressData& input, Diagnostic* dia)
{
  CompressData ret = input;

  // const uint32_t* header = reinterpret_cast<const uint32_t*>(input.data());
  // uint32_t amount = header[0];
  /*
  struct CompressionStep
  {
    ModAlgosE modId;
    CompressionAlgosE compId;
    uint32_t dataSize;
  };

  std::vector<CompressionStep> steps;
  for (uint32_t i = 0; i < amount; ++i) {
    ModAlgosE mod = static_cast<ModAlgosE>(header[1 + i * 2]);
    CompressionAlgosE comp = static_cast<CompressionAlgosE>(header[1 + i * 2 + 1]);
    uint32_t dataSize = header[1 + i * 2 + 2];
    steps.push_back({ mod, comp, dataSize });
  }
  */

  // Extract compressed payload after the header
  // size_t headerSize = sizeof(uint32_t) * (1 + amount * 3);
  // std::vector<char> data(input.begin() + headerSize, input.end());

  // Reverse the compression steps (from last to first)
  // for (int i = amount - 1; i >= 0; --i) {
  while (ret.getHeader().algo != CompressionAlgosE::None)
  {
    auto compIt = compressors.find(ret.getHeader().algo);
    auto modIt = modifiers.find(ret.getHeader().modifier);

    if (compIt == compressors.end() || modIt == modifiers.end())
    {
      // Missing decompressor or unmodifier
      return ret;
    }

    CompressorI* comp = compIt->second;
    ModifierI* mod = modIt->second;

    // ret.resize(steps[i].dataSize);

    CompressDataHeader header = ret.getHeader();
    header.algo = CompressionAlgosE::None;
    ret.setHeader(header);

    ret = comp->decompress(ret);
    ret = mod->unmodify(ret);
  }
  //}

  /*
  if (dia != nullptr) {
    std::string compression;
    for (uint32_t i = 0; i < amount; ++i) {
      if (i > 0)
        compression += ", ";
      compression += getModifierName(steps[i].modId) + "<->" + getCompressorName(steps[i].compId);
    }

    //dia->setDecompression(input.size(), data.size(), compression);
  }
  */

  return ret;
}

/*
std::vector<uint32_t> CompressionManager::decompress(const std::vector<uint32_t>& input, Diagnostic* dia)
{
  std::vector<char> data(input.size() * sizeof(uint32_t));
  memcpy(data.data(), input.data(), input.size() * sizeof(uint32_t));

  std::vector<char> decompressed = decompress(data, dia);

  std::vector<uint32_t> output;
  size_t outputSize = decompressed.size() / sizeof(uint32_t);
  if (decompressed.size() % sizeof(uint32_t) != 0)
    outputSize++;

  output.resize(outputSize);
  memcpy(output.data(), decompressed.data(), decompressed.size());

  return output;
}

std::vector<uint16_t> CompressionManager::decompress(const std::vector<uint16_t>& input, Diagnostic* dia)
{
  std::vector<char> data(input.size() * sizeof(uint16_t));
  memcpy(data.data(), input.data(), input.size() * sizeof(uint16_t));

  std::vector<char> decompressed = decompress(data, dia);

  std::vector<uint16_t> output;
  size_t outputSize = decompressed.size() / sizeof(uint16_t);
  if (decompressed.size() % sizeof(uint16_t) != 0)
    outputSize++;

  output.resize(outputSize);
  memcpy(output.data(), decompressed.data(), decompressed.size());

  return output;
}

std::vector<uint32_t> CompressionManager::compress(std::vector<uint32_t> data, Diagnostic* dia, std::vector<ModAlgosE> modFilter, std::vector<CompressionAlgosE>
cmpFilter)
{
  std::vector<char> data4(data.size() * sizeof(uint32_t));
  memcpy(data4.data(), data.data(), data.size() * sizeof(uint32_t));

  std::vector<char> ret1 = compress(data4, dia, modFilter, cmpFilter);

  std::vector<uint32_t> ret4;

  size_t resSize = ret1.size() / sizeof(uint32_t) + ((ret1.size() % sizeof(uint32_t)) > 0 ? 1 : 0);

  ret4.resize(resSize);
  memcpy(ret4.data(), ret1.data(), ret1.size());

  return ret4;
}

std::vector<uint16_t> CompressionManager::compress(std::vector<uint16_t> data, Diagnostic* dia, std::vector<ModAlgosE> modFilter, std::vector<CompressionAlgosE>
cmpFilter)
{
  std::vector<char> data2(data.size() * sizeof(uint16_t));
  memcpy(data2.data(), data.data(), data.size() * sizeof(uint16_t));

  std::vector<char> ret1 = compress(data2, dia, modFilter, cmpFilter);

  std::vector<uint16_t> ret2;

  size_t resSize = ret1.size() / sizeof(uint16_t) + ((ret1.size() % sizeof(uint16_t)) > 0 ? 1 : 0);

  ret2.resize(resSize);
  memcpy(ret2.data(), ret1.data(), ret1.size());

  return ret2;
}
*/

std::string CompressionManager::getModifierName(ModAlgosE id)
{
  if (modifiers.contains(id))
    return modifiers[id]->getId();
  return "";
}

std::string CompressionManager::getCompressorName(CompressionAlgosE id)
{
  if (compressors.contains(id))
    return compressors[id]->getId();
  return "";
}

void CompressionManager::registerModifier(ModAlgosE algo, ModifierI* modifier)
{
  modifiers[algo] = modifier;
}

void CompressionManager::registerCompressor(CompressionAlgosE algo, CompressorI* modifier)
{
  compressors[algo] = modifier;
}