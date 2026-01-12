#include "SourceStructureTree.hpp"

#include "AhnalyticBase/compression/CompressionManager.hpp"
#include "AhnalyticBase/helper/Diagnostic.hpp"

#include <algorithm>
#include <functional>
#include <optional>
#include <queue>
#include <set>
#include <unordered_map>

void SourceStructureTree::serialize(const std::vector<FlatNodeDeDupData>& nodeList, const std::vector<uint32_t>& indexList, std::vector<char>& data,
                                    Diagnostic* dia)
{
  auto labeledDia = [dia](const std::string& label)
  {
    if (dia)
      dia->setLabel(label);
    return dia;
  };

  std::vector<uint32_t> symbolList(nodeList.size());
  std::vector<uint32_t> fieldList(nodeList.size());
  std::vector<uint32_t> amountList(nodeList.size());

  for (uint32_t index = 0; index < nodeList.size(); index++)
  {
    const FlatNodeDeDupData& node = nodeList.at(index);
    symbolList[index] = node.data.id.data.symboldId;
    fieldList[index] = node.data.id.data.fieldId;
    amountList[index] = static_cast<uint32_t>(node.amount); // Cast to match encoding
  }

  CompressionManager compressionManager;

  CompressData compressedIndexList = compressionManager.compress(indexList, labeledDia("Index"), std::vector<ModAlgosE>{ModAlgosE::None},
                                                                 std::vector<CompressionAlgosE>{CompressionAlgosE::LZMA});
  // CompressData indexList2 = compressionManager.decompress(compressedIndexList, labeledDia("Index"));
  CompressData symbolListCompressed = compressionManager.compress(symbolList, labeledDia("Symbols"), std::vector<ModAlgosE>{ModAlgosE::None},
                                                                  std::vector<CompressionAlgosE>{CompressionAlgosE::BSC});
  CompressData fieldListCompressed = compressionManager.compress(fieldList, labeledDia("Fields"), std::vector<ModAlgosE>{ModAlgosE::None},
                                                                 std::vector<CompressionAlgosE>{CompressionAlgosE::BSC});
  CompressData amountListCompressed = compressionManager.compress(amountList, labeledDia("Amount"), std::vector<ModAlgosE>{ModAlgosE::None},
                                                                  std::vector<CompressionAlgosE>{CompressionAlgosE::BSC});

  // Build final buffer with sizes prepended
  std::vector<uint32_t> ret;
  ret.reserve(4 + compressedIndexList.getUint32Size() + symbolListCompressed.getUint32Size() + fieldListCompressed.getUint32Size() +
              amountListCompressed.getUint32Size());

  std::vector<uint32_t> compressedIndexListData = compressedIndexList.getUint32Data();
  std::vector<uint32_t> symbolListCompressedData = symbolListCompressed.getUint32Data();
  std::vector<uint32_t> fieldListCompressedData = fieldListCompressed.getUint32Data();
  std::vector<uint32_t> amountListCompressedData = amountListCompressed.getUint32Data();

  ret.push_back(static_cast<uint32_t>(compressedIndexListData.size()));
  ret.push_back(static_cast<uint32_t>(symbolListCompressedData.size()));
  ret.push_back(static_cast<uint32_t>(fieldListCompressedData.size()));
  ret.push_back(static_cast<uint32_t>(amountListCompressedData.size()));

  ret.insert(ret.end(), compressedIndexListData.begin(), compressedIndexListData.end());
  ret.insert(ret.end(), symbolListCompressedData.begin(), symbolListCompressedData.end());
  ret.insert(ret.end(), fieldListCompressedData.begin(), fieldListCompressedData.end());
  ret.insert(ret.end(), amountListCompressedData.begin(), amountListCompressedData.end());

  // std::vector<char> rawBytes(ret.size() * sizeof(uint32_t));
  // memcpy(rawBytes.data(), ret.data(), rawBytes.size());

  // CompressData compressed = compressionManager.compress(ret, labeledDia("Result"));
  // CompressData testResult = compressionManager.decompress(compressed, labeledDia("Result"));
  // std::vector<uint32_t> testResultData = testResult.getUint32Data();
  // assert(memcmp(ret.data(), testResultData.data(), ret.size()) == 0);

  CompressData result(ret, false);

  if (dia)
    dia->setResultSize(result.getCharSize());

  data = result.getCharData();
}

void SourceStructureTree::deserialize(const std::vector<char>& data, std::vector<FlatNodeDeDupData>& nodeList, std::vector<uint32_t>& indexList,
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
  std::vector<uint32_t> decompressedData = inData.getUint32Data();

  const uint32_t* raw = reinterpret_cast<const uint32_t*>(decompressedData.data());
  size_t totalUInts = decompressedData.size() / sizeof(uint32_t);

  uint32_t indexSize = raw[0];
  uint32_t symbolSize = raw[1];
  uint32_t fieldSize = raw[2];
  uint32_t amountSize = raw[3];

  const uint32_t* p = raw + 4;
  std::vector<uint32_t> compressedIndexList(p, p + indexSize);
  p += indexSize;
  std::vector<uint32_t> symbolListCompressed(p, p + symbolSize);
  p += symbolSize;
  std::vector<uint32_t> fieldListCompressed(p, p + fieldSize);
  p += fieldSize;
  std::vector<uint32_t> amountListCompressed(p, p + amountSize);
  p += amountSize;

  // Step 2: decompress each list
  indexList = compressionManager.decompress(CompressData(compressedIndexList, true), labeledDia("Index")).getUint32Data();
  std::vector<uint32_t> symbolList = compressionManager.decompress(CompressData(symbolListCompressed, true), labeledDia("Symbols")).getUint32Data();
  std::vector<uint32_t> fieldList = compressionManager.decompress(CompressData(fieldListCompressed, true), labeledDia("Fields")).getUint32Data();
  std::vector<uint32_t> amountList = compressionManager.decompress(CompressData(amountListCompressed, true), labeledDia("Amount")).getUint32Data();

  nodeList.resize(symbolList.size());
  for (uint32_t i = 0; i < symbolList.size(); ++i)
  {
    nodeList[i].data.id.data.symboldId = symbolList[i];
    nodeList[i].data.id.data.fieldId = fieldList[i];
    nodeList[i].amount = static_cast<int>(amountList[i]); // Cast back from uint32_t
  }
}

/*

bool SourceStructureTreeFlat::operator==(const SourceStructureTreeFlat& other) const
{
  return symboldId == other.symboldId && fieldId == other.fieldId && amount == other.amount;
}

SourceStructureTree::SourceStructureTree() : parent(nullptr)
{
  id.cmpData = 0;
}

SourceStructureTree::SourceStructureTree(const SourceStructureTree& other) : id(other.id), parent(other.parent)
{
  for (auto child : other.children) {
    auto newChild = new SourceStructureTree(*child);
    newChild->parent = this;
    children.push_back(newChild);
  }
}

SourceStructureTree::~SourceStructureTree()
{
  for (auto child : children)
  {
    delete child;
  }
}

bool SourceStructureTree::operator==(const SourceStructureTree& other) const
{
  if (id.cmpData != other.id.cmpData || children.size() != other.children.size())
    return false;

  for (size_t i = 0; i < children.size(); ++i)
    if (*children[i] != *other.children[i])
      return false;

  return true;
}

void SourceStructureTree::serialize(const std::vector<SourceStructureTreeFlat>& nodeList, const std::vector<uint32_t>& indexList, std::vector<char>& data,
Diagnostic* dia)
{
  auto labeledDia = [dia](const std::string& label)
    {
      if (dia)
        dia->setLabel(label);
      return dia;
    };

  std::vector<uint32_t> symbolList(nodeList.size());
  std::vector<uint32_t> fieldList(nodeList.size());
  std::vector<uint32_t> amountList(nodeList.size());

  for (uint32_t index = 0; index < nodeList.size(); index++)
  {
    const SourceStructureTreeFlat& node = nodeList.at(index);
    symbolList[index] = node.symboldId;
    fieldList[index] = node.fieldId;
    amountList[index] = static_cast<uint32_t>(node.amount);  // Cast to match encoding
  }

  CompressionManager compressionManager;

  CompressData compressedIndexList = compressionManager.compress(indexList, labeledDia("Index"));
  CompressData indexList2 = compressionManager.decompress(compressedIndexList, labeledDia("Index"));
  CompressData symbolListCompressed = compressionManager.compress(symbolList, labeledDia("Symbols"));
  CompressData fieldListCompressed = compressionManager.compress(fieldList, labeledDia("Fields"));
  CompressData amountListCompressed = compressionManager.compress(amountList, labeledDia("Amount"));

  // Build final buffer with sizes prepended
  std::vector<uint32_t> ret;
  ret.reserve(4 + compressedIndexList.getUint32Size() + symbolListCompressed.getUint32Size() + fieldListCompressed.getUint32Size() +
amountListCompressed.getUint32Size());

  std::vector<uint32_t> compressedIndexListData = compressedIndexList.getUint32Data();
  std::vector<uint32_t> symbolListCompressedData = symbolListCompressed.getUint32Data();
  std::vector<uint32_t> fieldListCompressedData = fieldListCompressed.getUint32Data();
  std::vector<uint32_t> amountListCompressedData = amountListCompressed.getUint32Data();

  ret.push_back(static_cast<uint32_t>(compressedIndexListData.size()));
  ret.push_back(static_cast<uint32_t>(symbolListCompressedData.size()));
  ret.push_back(static_cast<uint32_t>(fieldListCompressedData.size()));
  ret.push_back(static_cast<uint32_t>(amountListCompressedData.size()));

  ret.insert(ret.end(), compressedIndexListData.begin(), compressedIndexListData.end());
  ret.insert(ret.end(), symbolListCompressedData.begin(), symbolListCompressedData.end());
  ret.insert(ret.end(), fieldListCompressedData.begin(), fieldListCompressedData.end());
  ret.insert(ret.end(), amountListCompressedData.begin(), amountListCompressedData.end());

  //std::vector<char> rawBytes(ret.size() * sizeof(uint32_t));
  //memcpy(rawBytes.data(), ret.data(), rawBytes.size());

  //CompressData compressed = compressionManager.compress(ret, labeledDia("Result"));
  //CompressData testResult = compressionManager.decompress(compressed, labeledDia("Result"));
  //std::vector<uint32_t> testResultData = testResult.getUint32Data();
  //assert(memcmp(ret.data(), testResultData.data(), ret.size()) == 0);

  CompressData result(ret, false);

  if (dia)
    dia->setResultSize(result.getCharSize());

  data = result.getCharData();
}

void SourceStructureTree::deserialize(const std::vector<char>& data, std::vector<SourceStructureTreeFlat>& nodeList, std::vector<uint32_t>& indexList,
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
  //CompressData decompressed = compressionManager.decompress(CompressData(data, true), labeledDia("Result"));
  CompressData inData(data, false);
  std::vector<uint32_t> decompressedData = inData.getUint32Data();

  const uint32_t* raw = reinterpret_cast<const uint32_t*>(decompressedData.data());
  size_t totalUInts = decompressedData.size() / sizeof(uint32_t);

  uint32_t indexSize = raw[0];
  uint32_t symbolSize = raw[1];
  uint32_t fieldSize = raw[2];
  uint32_t amountSize = raw[3];

  const uint32_t* p = raw + 4;
  std::vector<uint32_t> compressedIndexList(p, p + indexSize);
  p += indexSize;
  std::vector<uint32_t> symbolListCompressed(p, p + symbolSize);
  p += symbolSize;
  std::vector<uint32_t> fieldListCompressed(p, p + fieldSize);
  p += fieldSize;
  std::vector<uint32_t> amountListCompressed(p, p + amountSize);
  p += amountSize;

  // Step 2: decompress each list
  indexList = compressionManager.decompress(CompressData(compressedIndexList, true), labeledDia("Index")).getUint32Data();
  std::vector<uint32_t> symbolList = compressionManager.decompress(CompressData(symbolListCompressed, true), labeledDia("Symbols")).getUint32Data();
  std::vector<uint32_t> fieldList = compressionManager.decompress(CompressData(fieldListCompressed, true), labeledDia("Fields")).getUint32Data();
  std::vector<uint32_t> amountList = compressionManager.decompress(CompressData(amountListCompressed, true), labeledDia("Amount")).getUint32Data();

  nodeList.resize(symbolList.size());
  for (uint32_t i = 0; i < symbolList.size(); ++i)
  {
    nodeList[i].symboldId = symbolList[i];
    nodeList[i].fieldId = fieldList[i];
    nodeList[i].amount = static_cast<int>(amountList[i]);  // Cast back from uint32_t
  }
}

#define isLeave(node) (node->children.size() == 0)

void SourceStructureTree::reduceTree(std::list<SourceStructureTree*> trees, std::vector<SourceStructureTreeFlat>& nodeList, std::vector<uint32_t>& indexList)
{
  std::unordered_map<SourceStructureTree*, uint32_t> depthMap;

  struct TreeNode
  {
    uint16_t symboldId;
    uint16_t fieldId;
    std::vector<uint32_t> children;
  };

  std::vector<TreeNode*> flatList;
  std::vector<SourceStructureTree*> cmpList;

  std::function<uint32_t(SourceStructureTree*)> dedupTree = nullptr;

  dedupTree = [&dedupTree, &flatList, &cmpList, &depthMap](SourceStructureTree* node)
    {
      uint32_t ret = 0;
      uint32_t depth = depthMap[node];

      auto iter = std::find_if(cmpList.begin(), cmpList.end(), [&](SourceStructureTree* n) { return *n == *node; });
      if (iter != cmpList.end())
        ret = iter - cmpList.begin();
      else
      {
        TreeNode* flatNode = new TreeNode();

        flatNode->symboldId = node->id.data.symboldId;
        flatNode->fieldId = node->id.data.fieldId;

        cmpList.push_back(node);
        flatList.push_back(flatNode);
        ret = flatList.size() - 1;

        for (SourceStructureTree* child : node->children)
          flatNode->children.push_back(dedupTree(child));
      }

      return ret;
    };

  indexList.push_back(trees.size());

  for (SourceStructureTree* tree : trees)
  {
    TreeNode* flatNode = new TreeNode();
    flatNode->symboldId = tree->id.data.symboldId;
    flatNode->fieldId = tree->id.data.fieldId;

    flatList.push_back(flatNode);
    cmpList.push_back(tree);

    for (SourceStructureTree* child : tree->children)
      flatNode->children.push_back(dedupTree(child));
  }

  std::unordered_map<uint32_t, uint32_t> indexMap;
  for (int index = 0; index < flatList.size(); index++)
  {
    TreeNode* flatNode = flatList.at(index);
    auto iter = std::find_if(nodeList.begin(), nodeList.end(), [&](SourceStructureTreeFlat n)
      {
        return flatNode->children.size() == n.amount && flatNode->symboldId == n.symboldId && flatNode->fieldId == n.fieldId;
      });

    if (iter == nodeList.end())
    {
      nodeList.push_back({ flatNode->symboldId, flatNode->fieldId, (int64_t)flatNode->children.size() });
      indexMap[index] = nodeList.size() - 1;
    }
    else
      indexMap[index] = iter - nodeList.begin();
  }

  for (int index = 0; index < flatList.size(); index++)
  {
    TreeNode* flatNode = flatList.at(index);
    indexList.push_back(indexMap[index]);
    for (uint32_t child : flatNode->children)
      indexList.push_back(indexMap[child]);
    //delete flatNode;
  }

  std::vector<TreeNode*> flatListOut;
  for (int index = 1; index < indexList.size(); index++)
  {
    const SourceStructureTreeFlat& treeRoot = nodeList.at(indexList.at(index++));

    TreeNode* flatNode = new TreeNode();
    flatNode->symboldId = treeRoot.symboldId;
    flatNode->fieldId = treeRoot.fieldId;

    for (int childIndex = 0; childIndex < treeRoot.amount; childIndex++)
      flatNode->children.push_back(indexList.at(index + childIndex));

    index += treeRoot.amount - 1;

    flatListOut.push_back(flatNode);
  }
}

void SourceStructureTree::rebuildTree(const std::vector<SourceStructureTreeFlat>& nodeList, const std::vector<uint32_t>& indexList,
std::list<SourceStructureTree*>& trees)
{
  unsigned int treeSize = indexList.at(0);

  struct TreeNode
  {
    uint16_t symboldId;
    uint16_t fieldId;
    std::vector<uint32_t> children;
  };

  std::vector<TreeNode*> flatList;

  for (int index = 1; index < indexList.size(); index++)
  {
    const SourceStructureTreeFlat& treeRoot = nodeList.at(indexList.at(index++));

    TreeNode* flatNode = new TreeNode();
    flatNode->symboldId = treeRoot.symboldId;
    flatNode->fieldId = treeRoot.fieldId;

    for (int childIndex = 0; childIndex < treeRoot.amount; childIndex++)
      flatNode->children.push_back(indexList.at(index + childIndex));

    index += treeRoot.amount - 1;

    flatList.push_back(flatNode);
  }

  std::function<SourceStructureTree* (int)> reconstitudeTree = nullptr;
  reconstitudeTree = [&reconstitudeTree, &flatList](int index) -> SourceStructureTree*
    {
      TreeNode* flatNode = flatList.at(index);

      SourceStructureTree* rootNode = new SourceStructureTree();
      rootNode->id.data.symboldId = flatNode->symboldId;
      rootNode->id.data.fieldId = flatNode->fieldId;

      for (int childIndex = 0; childIndex < flatNode->children.size(); childIndex++)
        rootNode->children.push_back(reconstitudeTree(flatNode->children.at(childIndex)));

      return rootNode;
    };

  for (int index = 0; index < treeSize; index++)
    trees.push_back(reconstitudeTree(index));
}

void SourceStructureTree::hashTrees(std::list<SourceStructureTree*>& trees, std::vector<uint16_t>& hashList, std::vector<uint32_t>& hashListIndex)
{
  using NodeHashValue = uint16_t[3];

  std::function<void(SourceStructureTree*, std::vector<SourceStructureTree*>&)> getLeaves;
  getLeaves = [&getLeaves](SourceStructureTree* node, std::vector<SourceStructureTree*>& leaveList)
    {
      if (isLeave(node))
        leaveList.push_back(node);
      else
        for (SourceStructureTree* child : node->children)
          getLeaves(child, leaveList);
    };

  std::function<int32_t(SourceStructureTree*)> calcHash;
  calcHash = [&calcHash](SourceStructureTree* node)
    {
      int32_t ret = ((int32_t)node->id.data.symboldId) - (node->parent != nullptr ? ((int32_t)node->parent->id.data.symboldId) : 0);

      if (node->children.size() > 0)
      {
        int32_t subRet = 0;
        for (SourceStructureTree* child : node->children)
          subRet += calcHash(child);
        ret += subRet / node->children.size();
      }

      return ret;
    };

  auto getChildren = [](std::vector<SourceStructureTree*> nodes)
    {
      std::vector < SourceStructureTree*> ret;
      for (SourceStructureTree* node : nodes)
        ret.insert(ret.end(), node->children.begin(), node->children.end());
      return ret;
    };

  for (SourceStructureTree* tree : trees)
  {
    SourceStructureTree* last = nullptr;
    std::vector<SourceStructureTree*> leaveList;
    std::vector<SourceStructureTree*> parentList;
    getLeaves(tree, leaveList);
    for (SourceStructureTree* node : leaveList)
    {
      if (node->parent != nullptr && node->parent->parent != nullptr)
      {
        SourceStructureTree* cur = node->parent->parent;
        if (cur != last)
        {
          parentList.push_back(cur);
          last = cur;
        }
      }
    }

    for (SourceStructureTree* node : parentList)
    {
      uint16_t hash = (uint16_t)std::abs(calcHash(node));

      auto iter = std::find(hashList.begin(), hashList.end(), hash);

      if (iter == hashList.end())
      {
        hashList.push_back(hash);
        hashListIndex.push_back(hashList.size() - 1);
      }
      else
      {
        hashListIndex.push_back(iter - hashList.begin());
      }
    }
  }
}

void SourceStructureTree::trimTrees(std::list<SourceStructureTree*>& trees, uint32_t minDepth, uint32_t maxDepth)
{
  std::function<void(SourceStructureTree*, std::vector<SourceStructureTree*>&)> getLeaves;
  getLeaves = [&getLeaves](SourceStructureTree* node, std::vector<SourceStructureTree*>& leaveList)
    {
      if (isLeave(node))
        leaveList.push_back(node);
      else
        for (SourceStructureTree* child : node->children)
          getLeaves(child, leaveList);
    };

  std::unordered_map<SourceStructureTree*, bool> includeMap;

  std::function<std::vector<SourceStructureTree*>(SourceStructureTree*)> getMarkedNodes;
  getMarkedNodes = [&getMarkedNodes, &includeMap](SourceStructureTree* node)
    {
      std::vector<SourceStructureTree*> ret;
      if (includeMap[node])
        ret.push_back(node);
      else
        for (SourceStructureTree* child : node->children)
        {
          std::vector<SourceStructureTree*> subret = getMarkedNodes(child);
          ret.insert(ret.end(), subret.begin(), subret.end());
        }
      return ret;
    };

  std::function<void(SourceStructureTree*)> removeUneeded;
  removeUneeded = [&removeUneeded, &includeMap](SourceStructureTree* node)
    {
      std::vector<SourceStructureTree*> removeList;

      for (SourceStructureTree* child : node->children)
        if (!includeMap[child])
          removeList.push_back(child);

      if (removeList.size() > 0)
      {
        for (SourceStructureTree* child : removeList)
        {
          node->children.erase(std::remove(node->children.begin(), node->children.end(), child), node->children.end());
          delete child;
        }
      }

      for (SourceStructureTree* child : node->children)
        removeUneeded(child);
    };

  for (SourceStructureTree* tree : trees)
  {
    std::vector<SourceStructureTree*> leaveList;
    std::set<SourceStructureTree*> rootList;
    getLeaves(tree, leaveList);

    for (SourceStructureTree* leave : leaveList)
    {
      SourceStructureTree* cur = leave;
      SourceStructureTree* found = nullptr;
      //SourceStructureTree* foundMin = nullptr;

      includeMap[cur] = true;

      for (int index = 0; index <= maxDepth && cur->parent != nullptr && cur->parent->parent != nullptr; index++)
      {
        cur = cur->parent;
        includeMap[cur] = true;
        //if (index >= minDepth && cur->parent != nullptr)
          //found = cur;

        if (index == maxDepth)
          found = cur;
        //else if (index == maxDepth)
          //found = foundMin;
      }

      if (found != nullptr)
        rootList.insert(found);
      //markIncluded(found, 0);
    }

    for (SourceStructureTree* root : rootList)
      removeUneeded(root);

    tree->children = getMarkedNodes(tree);
    for (SourceStructureTree* child : tree->children)
      child->parent = tree;
  }
}
*/