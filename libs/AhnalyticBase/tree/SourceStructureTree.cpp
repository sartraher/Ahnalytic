#include "SourceStructureTree.hpp"

#include "AhnalyticBase/compression/CompressionManager.hpp"
#include "AhnalyticBase/helper/Diagnostic.hpp"

#include <algorithm>
#include <functional>
#include <optional>
#include <queue>
#include <set>
#include <unordered_map>

void SourceStructureTree::serialize(const ahn::vector<FlatNodeDeDupData>& nodeList, const ahn::vector<uint32_t>& indexList, ahn::vector<char>& data,
                                    Diagnostic* dia)
{
  auto labeledDia = [dia](const std::string& label)
  {
    if (dia)
      dia->setLabel(label);
    return dia;
  };

  ahn::vector<uint32_t> symbolList(nodeList.size());
  ahn::vector<uint32_t> fieldList(nodeList.size());
  ahn::vector<uint32_t> amountList(nodeList.size());

  for (uint32_t index = 0; index < nodeList.size(); index++)
  {
    const FlatNodeDeDupData& node = nodeList.at(index);
    symbolList[index] = node.data.id.data.symboldId;
    fieldList[index] = node.data.id.data.fieldId;
    amountList[index] = static_cast<uint32_t>(node.amount); // Cast to match encoding
  }

  CompressionManager compressionManager;

  CompressData compressedIndexList = compressionManager.compress(indexList, labeledDia("Index"), ahn::vector<ModAlgosE>{ModAlgosE::None},
                                                                 ahn::vector<CompressionAlgosE>{CompressionAlgosE::LZMA});

  CompressData symbolListCompressed = compressionManager.compress(symbolList, labeledDia("Symbols"), ahn::vector<ModAlgosE>{ModAlgosE::None},
                                                                  ahn::vector<CompressionAlgosE>{CompressionAlgosE::BSC});
  CompressData fieldListCompressed = compressionManager.compress(fieldList, labeledDia("Fields"), ahn::vector<ModAlgosE>{ModAlgosE::None},
                                                                 ahn::vector<CompressionAlgosE>{CompressionAlgosE::BSC});
  CompressData amountListCompressed = compressionManager.compress(amountList, labeledDia("Amount"), ahn::vector<ModAlgosE>{ModAlgosE::None},
                                                                  ahn::vector<CompressionAlgosE>{CompressionAlgosE::BSC});

  // Build final buffer with sizes prepended
  ahn::vector<uint32_t> ret;
  ret.reserve(4 + compressedIndexList.getUint32Size() + symbolListCompressed.getUint32Size() + fieldListCompressed.getUint32Size() +
              amountListCompressed.getUint32Size());

  ahn::vector<uint32_t> compressedIndexListData = compressedIndexList.getUint32Data(CompressData::On);
  ahn::vector<uint32_t> symbolListCompressedData = symbolListCompressed.getUint32Data(CompressData::On);
  ahn::vector<uint32_t> fieldListCompressedData = fieldListCompressed.getUint32Data(CompressData::On);
  ahn::vector<uint32_t> amountListCompressedData = amountListCompressed.getUint32Data(CompressData::On);

  ret.push_back(static_cast<uint32_t>(compressedIndexListData.size()));
  ret.push_back(static_cast<uint32_t>(symbolListCompressedData.size()));
  ret.push_back(static_cast<uint32_t>(fieldListCompressedData.size()));
  ret.push_back(static_cast<uint32_t>(amountListCompressedData.size()));

  ret.insert(ret.end(), compressedIndexListData.begin(), compressedIndexListData.end());
  ret.insert(ret.end(), symbolListCompressedData.begin(), symbolListCompressedData.end());
  ret.insert(ret.end(), fieldListCompressedData.begin(), fieldListCompressedData.end());
  ret.insert(ret.end(), amountListCompressedData.begin(), amountListCompressedData.end());

  CompressData result(ret, false);

  if (dia)
    dia->setResultSize(result.getCharSize());

  data = result.getCharData(CompressData::Off);
}

void SourceStructureTree::deserialize(const ahn::vector<char>& data, ahn::vector<FlatNodeDeDupData>& nodeList, ahn::vector<uint32_t>& indexList,
                                      Diagnostic* dia)
{
  auto labeledDia = [dia](const std::string& label)
  {
    if (dia)
      dia->setLabel(label);
    return dia;
  };

  CompressionManager compressionManager;

  // Step 1: decompress entire payload
  // CompressData decompressed = compressionManager.decompress(CompressData(data, true), labeledDia("Result"));
  CompressData inData(data, false);
  ahn::vector<uint32_t> decompressedData = inData.getUint32Data(CompressData::Off);

  const uint32_t* raw = reinterpret_cast<const uint32_t*>(decompressedData.data());
  size_t totalUInts = decompressedData.size() / sizeof(uint32_t);

  uint32_t indexSize = raw[0];
  uint32_t symbolSize = raw[1];
  uint32_t fieldSize = raw[2];
  uint32_t amountSize = raw[3];

  const uint32_t* p = raw + 4;
  ahn::vector<uint32_t> compressedIndexList(p, p + indexSize);
  p += indexSize;
  ahn::vector<uint32_t> symbolListCompressed(p, p + symbolSize);
  p += symbolSize;
  ahn::vector<uint32_t> fieldListCompressed(p, p + fieldSize);
  p += fieldSize;
  ahn::vector<uint32_t> amountListCompressed(p, p + amountSize);
  p += amountSize;

  // Step 2: decompress each list
  indexList = compressionManager.decompress(CompressData(compressedIndexList, true), labeledDia("Index")).getUint32Data(CompressData::Off);
  ahn::vector<uint32_t> symbolList =
      compressionManager.decompress(CompressData(symbolListCompressed, true), labeledDia("Symbols")).getUint32Data(CompressData::Off);
  ahn::vector<uint32_t> fieldList =
      compressionManager.decompress(CompressData(fieldListCompressed, true), labeledDia("Fields")).getUint32Data(CompressData::Off);
  ahn::vector<uint32_t> amountList =
      compressionManager.decompress(CompressData(amountListCompressed, true), labeledDia("Amount")).getUint32Data(CompressData::Off);

  nodeList.resize(symbolList.size());
  for (uint32_t i = 0; i < symbolList.size(); ++i)
  {
    nodeList[i].data.id.data.symboldId = symbolList[i];
    nodeList[i].data.id.data.fieldId = fieldList[i];
    nodeList[i].amount = static_cast<int>(amountList[i]); // Cast back from uint32_t
  }
}
