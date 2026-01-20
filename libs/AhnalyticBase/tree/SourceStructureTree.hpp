#ifndef sourcestructuretree_h__
#define sourcestructuretree_h__

#include "AhnalyticBase/Export.hpp"
#include "AhnalyticBase/tree/Tree.hpp"

#include <memory>
#include <stdint.h>
#include <string>
#include <unordered_map>
#include <vector>

class Diagnostic;

struct SourceStructureData
{
  SourceStructureData()
  {
  }

  union
  {
    struct
    {
      uint16_t symboldId;
      uint16_t fieldId;
    } data;
    uint32_t cmpData = 0;
  } id;

  //uint32_t lineNr = 0;

  bool operator==(const SourceStructureData& other) const
  {
    return id.cmpData == other.id.cmpData;
  }
  bool operator!=(const SourceStructureData& other) const
  {
    return !(*this == other);
  }
};

struct SourceStructureDeepData
{
  SourceStructureDeepData()
  {
  }

  union
  {
    struct
    {
      uint16_t symboldId;
      uint16_t fieldId;
    } data;
    uint32_t cmpData = 0;
  } id;

  std::string name;

  uint32_t lineNr = 0;

  bool operator==(const SourceStructureDeepData& other) const
  {
    return id.cmpData == other.id.cmpData;
  }

  bool operator!=(const SourceStructureDeepData& other) const
  {
    return !(*this == other);
  }

};

namespace std
{
template <>
struct hash<SourceStructureData>
{
  size_t operator()(const SourceStructureData& x) const
  {
    return std::hash<uint32_t>{}(x.id.cmpData);
  }
};
} // namespace std

using FlatNodeDeDupData = FlatNodeDeDup<SourceStructureData>;

struct DLLEXPORT SourceStructureTree : public Tree<SourceStructureData>
{
  SourceStructureTree(const SourceStructureData data = {}) : Tree<SourceStructureData>(data)
  {
  }

  static void serialize(const std::vector<FlatNodeDeDupData>& nodeList, const std::vector<uint32_t>& indexList, std::vector<char>& data,
                        Diagnostic* dia = nullptr);
  static void deserialize(const std::vector<char>& data, std::vector<FlatNodeDeDupData>& nodeList, std::vector<uint32_t>& indexList, Diagnostic* dia);
};

struct DLLEXPORT SourceStructureTreeDeep : public Tree<SourceStructureDeepData>
{
  SourceStructureTreeDeep(const SourceStructureDeepData data = {}) : Tree<SourceStructureDeepData>(data)
  {
  }
};

#endif