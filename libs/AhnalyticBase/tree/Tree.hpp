#ifndef tree_hpp__
#define tree_hpp__

#include <functional>
#include <iostream>
#include <unordered_map>
#include <vector>

#include <stdint.h>

#include "AhnalyticBase/Export.hpp"

#include <ankerl/unordered_dense.h>

template <typename T>
struct DLLEXPORT Tree
{
  T data;
  std::vector<Tree<T>*> children;
  Tree<T>* parent = nullptr;

  // mutable uint32_t complexity = 0;

  Tree(const T& val) : data(val)
  {
  }

  ~Tree()
  {
    for (Tree<T>* child : children)
      delete child;
  }

  Tree<T>* clone() const
  {
    Tree<T>* ret = new Tree<T>(data);
    ret->children.reserve(children.size());

    for (Tree<T>* child : children)
    {
      Tree<T>* childClone = child->clone();
      childClone->parent = ret;
      ret->children.push_back(childClone);
    }

    return ret;
  }

  bool operator!=(const Tree<T>& other) const
  {
    return !((*this) == other);
  }

  bool operator==(const Tree<T>& other) const
  {
    if (data != other.data)
      return false;

    if (children.size() != other.children.size())
      return false;

    for (size_t i = 0; i < children.size(); ++i)
    {
      if (children[i] == nullptr && other.children[i] == nullptr)
        continue;

      if ((children[i] == nullptr) != (other.children[i] == nullptr))
        return false;

      if (*children[i] != *other.children[i])
        return false;
    }

    return true;
  }

  uint32_t getNodeCount() const
  {
    uint32_t ret = 1;
    for (const Tree<T>* child : children)
      ret += child->getNodeCount();

    return ret;
  }

  void getNodes(std::vector<const Tree<T>*>& nodes) const
  {
    nodes.push_back(this);
    for (const Tree<T>* child : children)
      child->getNodes(nodes);
  }

  void traverse(std::function<void(const Tree<T>*)> callback) const
  {
    callback(this);
    for (const Tree<T>* child : children)
      child->traverse(callback);
  }

  double fuzzyCompare(const Tree<T>* other) const
  {
    double ret = 0.0;
    if (this->data == other->data)
      ret = 0.3;

    size_t minSize = std::min(this->children.size(), other->children.size());
    size_t maxSize = std::max(this->children.size(), other->children.size());
    if (minSize > 0)
    {
      double subRet = 0.0;
      for (int index = 0; index < minSize; index++)
        subRet += this->children.at(index)->fuzzyCompare(other->children.at(index));

      ret += (subRet / maxSize) * 0.7;
    }
    else
      ret += 0.7;

    return ret;
  }
};

template <typename T>
struct DLLEXPORT FlatNode
{
  T data;

  std::vector<uint32_t> nodeIndex;

  bool operator==(const FlatNode& other) const
  {
    return data == other.data && nodeIndex == other.nodeIndex;
  }
};

template <typename T>
struct DLLEXPORT FlatNodeDeDup
{
  T data;

  uint32_t amount; // number of children
};

namespace std
{
template <typename T>
struct DLLEXPORT hash<FlatNode<T>>
{
  size_t operator()(const FlatNode<T>& node) const
  {
    size_t h = std::hash<T>{}(node.data);

    for (auto idx : node.nodeIndex)
    {
      h ^= std::hash<uint32_t>{}(idx) + 0x9e3779b9 + (h << 6) + (h >> 2);
    }

    return h;
  }
};
} // namespace std

template <typename T>
class DLLEXPORT CompareContainer
{
public:
  T data;

  CompareContainer(T data) : data(data)
  {
  }

  bool operator==(const CompareContainer<T>& other) const
  {
    return *data == *other.data;
  }
};

template <typename T>
struct DLLEXPORT std::hash<CompareContainer<T>>
{
  std::size_t operator()(const CompareContainer<T>& container) const
  {
    size_t h = container.data->getNodeCount();
    return std::hash<uint32_t>{}(container.data->data.id.cmpData) + 0x9e3779b9 + (h << 6) + (h >> 2);
  }
};

template <typename T>
void reduceTree(Tree<T>* root, std::vector<FlatNodeDeDup<T>>& dedupedNodes, std::vector<uint32_t>& indexList)
{
  std::vector<FlatNode<T>*> flatList;
  // std::vector<Tree<T>*> cmpList;

  std::function<uint32_t(Tree<T>*)> dedupTree = nullptr;

  // std::unordered_map<CompareContainer<Tree<T>*>, size_t> posList;
  ankerl::unordered_dense::map<CompareContainer<Tree<T>*>, size_t> posList;
  // posList[root] = 123;

  dedupTree = [&dedupTree, &flatList, &posList](Tree<T>* node)
  {
    uint32_t ret = 0;

    // auto iter = std::find_if(cmpList.begin(), cmpList.end(), [&](Tree<T>* n) { return *n == *node; });
    auto iter = posList.find(node);
    if (iter != posList.end())
      // ret = iter - cmpList.begin();
      ret = (uint32_t)iter->second;
    else
    {
      FlatNode<T>* flatNode = new FlatNode<T>();

      flatNode->data.id.data.symboldId = node->data.id.data.symboldId;
      flatNode->data.id.data.fieldId = node->data.id.data.fieldId;

      ret = (uint32_t)flatList.size();
      posList[node] = ret;
      // cmpList.push_back(node);
      flatList.push_back(flatNode);

      for (Tree<T>* child : node->children)
        flatNode->nodeIndex.push_back(dedupTree(child));
    }

    return ret;
  };

  dedupTree(root);

  dedupedNodes.resize(flatList.size());
  for (int index = 0; index < flatList.size(); index++)
  {
    auto& node = dedupedNodes[index];
    node.data = flatList[index]->data;
    node.amount = (uint32_t)flatList[index]->nodeIndex.size();
    for (uint32_t childIndex : flatList[index]->nodeIndex)
      indexList.push_back(childIndex);
  }
}

template <typename T>
Tree<T>* rebuildTree(const std::vector<FlatNodeDeDup<T>>& dedupedNodes, const std::vector<uint32_t>& indexList)
{
  std::vector<FlatNode<T>*> flatList;
  flatList.resize(dedupedNodes.size());

  int cursor = 0;
  for (int index = 0; index < dedupedNodes.size(); index++)
  {
    flatList[index] = new FlatNode<T>();
    flatList[index]->data.id.data.symboldId = dedupedNodes[index].data.id.data.symboldId;
    flatList[index]->data.id.data.fieldId = dedupedNodes[index].data.id.data.fieldId;

    for (int childIndex = 0; childIndex < dedupedNodes[index].amount; childIndex++, cursor++)
      flatList[index]->nodeIndex.push_back(indexList.at(cursor));
  }

  std::function<Tree<T>*(int)> dubTree = nullptr;
  std::unordered_map<int, Tree<T>*> treeLookup;

  dubTree = [&dubTree, &flatList, &treeLookup](int index) -> Tree<T>*
  {
    auto iter = treeLookup.find(index);
    if (iter != treeLookup.end())
      return iter->second->clone();
    else
    {
      FlatNode<T>* flatNode = flatList[index];

      Tree<T>* tree = new Tree<T>(flatNode->data);
      tree->children.reserve(flatNode->nodeIndex.size());
      for (int childIndex = 0; childIndex < flatNode->nodeIndex.size(); childIndex++)
        tree->children.push_back(dubTree(flatNode->nodeIndex[childIndex]));

      treeLookup[index] = tree;
      return tree;
    }

    return nullptr;
  };

  Tree<T>* ret = dubTree(0);

  for (FlatNode<T>* flatNode : flatList)
    delete flatNode;

  return ret;
}

template <typename T>
void flattenTree(Tree<T>* root, std::vector<FlatNode<T>>& flatVec, std::unordered_map<Tree<T>*, uint32_t>& nodeToIndex)
{
  if (!root)
    return;

  uint32_t currentIndex = flatVec.size();
  nodeToIndex[root] = currentIndex;
  flatVec.push_back(FlatNode<T>{root->data, {}});

  for (Tree<T>* child : root->children)
  {
    flattenTree(child, flatVec, nodeToIndex);
    flatVec[currentIndex].nodeIndex.push_back(nodeToIndex[child]);
  }
}

template <typename T>
std::vector<FlatNode<T>> treeToFlat(Tree<T>* root)
{
  std::vector<FlatNode<T>> flatVec;
  std::unordered_map<Tree<T>*, uint32_t> nodeToIndex;
  flattenTree(root, flatVec, nodeToIndex);
  return flatVec;
}

template <typename T>
void flattenAndDedupFlatNodes(const std::vector<FlatNode<T>>& flatNodes, std::vector<FlatNodeDeDup<T>>& dedupedNodes, std::vector<uint32_t>& indexList)
{
  dedupedNodes.clear();
  indexList.clear();
  std::unordered_map<FlatNode<T>, uint32_t> dedupMap;
  std::vector<uint32_t> nodeToDedupedIndex(flatNodes.size());

  // Deduplicate and assign deduped indices
  for (size_t i = 0; i < flatNodes.size(); ++i)
  {
    const FlatNode<T>& node = flatNodes[i];
    auto it = dedupMap.find(node);

    if (it == dedupMap.end())
    {
      uint32_t dedupIndex = (uint32_t)dedupedNodes.size();
      dedupMap[node] = dedupIndex;
      dedupedNodes.push_back(FlatNodeDeDup<T>{node.data, static_cast<uint32_t>(node.nodeIndex.size())});

      nodeToDedupedIndex[i] = dedupIndex;
    }
    else
    {
      nodeToDedupedIndex[i] = it->second;
    }
  }

  // Reserve space for indexList:
  // first N = deduped node indices per original node
  // then sum of all children counts (sum of dedupedNodes amounts for each original node)
  size_t totalChildrenCount = 0;
  for (size_t i = 0; i < flatNodes.size(); ++i)
  {
    uint32_t dedupIdx = nodeToDedupedIndex[i];
    totalChildrenCount += dedupedNodes[dedupIdx].amount;
  }

  indexList.reserve(flatNodes.size() + totalChildrenCount);

  // Step 1: store deduped node indices per original node
  for (uint32_t idx : nodeToDedupedIndex)
  {
    indexList.push_back(idx);
  }

  // Step 2: store children indices of all original nodes concatenated
  for (const FlatNode<T>& node : flatNodes)
  {
    for (uint32_t childIdx : node.nodeIndex)
    {
      indexList.push_back(childIdx);
    }
  }
}

template <typename T>

std::vector<FlatNode<T>> reconstructFlatNodesFromDedup(const std::vector<FlatNodeDeDup<T>>& dedupedNodes, const std::vector<uint32_t>& indexList,
                                                       size_t originalNodeCount)
{
  if (indexList.size() < originalNodeCount)
    throw std::runtime_error("Index list too short");

  std::vector<FlatNode<T>> flatNodes(originalNodeCount);

  // The first originalNodeCount entries are deduped node indices
  // The rest are concatenated children indices
  size_t childrenStart = originalNodeCount;
  size_t cursor = childrenStart;

  for (size_t i = 0; i < originalNodeCount; ++i)
  {
    uint32_t dedupIdx = indexList[i];

    if (dedupIdx >= dedupedNodes.size())
      throw std::runtime_error("Invalid deduped node index");

    const FlatNodeDeDup<T>& dedupNode = dedupedNodes[dedupIdx];
    flatNodes[i].data = dedupNode.data;
    flatNodes[i].nodeIndex.reserve(dedupNode.amount);

    for (uint32_t c = 0; c < dedupNode.amount; ++c)
    {
      if (cursor >= indexList.size())
        throw std::runtime_error("Children index list too short");

      flatNodes[i].nodeIndex.push_back(indexList[cursor++]);
    }
  }

  if (cursor != indexList.size())
    throw std::runtime_error("Did not consume entire children index list");

  return flatNodes;
}

template <typename T>
void printTree(const Tree<T>* node, int depth = 0)
{
  if (!node)
    return;

  std::cout << std::string(depth * 2, ' ') << node->data << std::endl;

  for (auto c : node->children)
    printTree(c, depth + 1);
}

#endif