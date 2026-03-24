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

CompressData CompressionManager::compress(const CompressData& data, Diagnostic* dia, ahn::vector<ModAlgosE> modFilter, ahn::vector<CompressionAlgosE> cmpFilter,
                                          bool force)
{
  CompressData ret = data;

  // uint32_t amount = 0;
  ahn::vector<CompressDataHeader> modComps;
  ahn::vector<CompressionAlgosE> alternatives;
  ahn::vector<CompressDataHeader> headers;

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

    CompressDataHeader header = ret.getHeader();
    header.algo = CompressionAlgosE::None;
    ret.setHeader(header);

    ret = comp->decompress(ret);
    ret = mod->unmodify(ret);
  }

  return ret;
}

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