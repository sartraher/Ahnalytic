#ifndef huffmanCompressor_hpp__
#define huffmanCompressor_hpp__

#include "compressor.hpp"

#include <functional>
#include <queue>
#include <unordered_map>
#include <vector>
/*
struct Byter
{
  std::vector<char> data;
  int curIndex = 0;
  int curSubIndex = 0;

  Byter()
  {
    data.push_back(0);
  }

  void addBit(bool value)
  {
    if (curSubIndex++ == 8)
    {
      curSubIndex = 0;
      curIndex++;
      data.push_back(0);
    }

    data[curIndex] = (data[curIndex] << 1) | (value ? 0x1 : 0x0);
  }
};

template <typename T>
struct HuffmanNode
{
  HuffmanNode(HuffmanNode<T>* l, HuffmanNode<T>* r) : left(l), right(r), amount(l->amount + r->amount), id(0)
  {
  }

  HuffmanNode(T data, uint32_t weight) : amount(weight), id(data)
  {
  }

  ~HuffmanNode() = default;

  HuffmanNode<T>* left = nullptr;
  HuffmanNode<T>* right = nullptr;
  uint32_t amount;

  T id;
};

class HuffmannCompressor : public CompressorI
{
public:
  enum ModeE
  {
    Byte1,
    Byte2,
    Byte4
  };

  HuffmannCompressor(ModeE mode);

  virtual std::vector<char> compress(std::vector<char> data);
  virtual std::string getId();

  template <typename T>
  HuffmanNode<T>* createHuffmanTree(const std::unordered_map<T, uint32_t>& heuristic)
  {
    auto cmp = [](const HuffmanNode<T>* a, const HuffmanNode<T>* b) { return a->amount > b->amount; };
    std::priority_queue < HuffmanNode<T>*, std::vector<HuffmanNode<T>*>, decltype(cmp)> queue(cmp);

    for (auto it = heuristic.begin(); it != heuristic.end(); ++it)
      queue.push(new HuffmanNode<T>(it->first, it->second));

    while (queue.size() > 1)
    {
      HuffmanNode<T>* left = queue.top();
      queue.pop();
      HuffmanNode<T>* right = queue.top();
      queue.pop();
      queue.push(new HuffmanNode<T>(left, right));
    };

    return queue.top();
  }

  template <typename T>
  std::vector<char> compressByteInternal(std::vector<T> data)
  {
    std::unordered_map<T, uint32_t> heuristic;

    auto incCount = [](auto& heuristic, auto value)
      {
        if (!heuristic.contains(value))
          heuristic[value] = 1;
        else
          heuristic[value]++;
      };

    for (auto value : data)
      incCount(heuristic, value);

    HuffmanNode<T>* huffmanTree = createHuffmanTree(heuristic);

    std::function<void(const HuffmanNode<T>*, std::unordered_map<T, std::vector<bool>>&, std::vector<bool>)> resolveTree;
    resolveTree = [&resolveTree](const HuffmanNode<T>* node, std::unordered_map<T, std::vector<bool>>& huffmanMap, std::vector<bool> cur)
      {
        if (node->left == nullptr)
          huffmanMap[node->id] = cur;
        else
        {
          std::vector<bool> leftNext = cur;
          leftNext.push_back(false);

          std::vector<bool> rightNext = cur;
          rightNext.push_back(true);

          resolveTree(node->left, huffmanMap, leftNext);
          resolveTree(node->right, huffmanMap, rightNext);
        }
      };

    std::unordered_map<T, std::vector<bool>> huffmanMap;

    resolveTree(huffmanTree, huffmanMap, std::vector<bool>());

    auto writeValue = [](Byter* byter, std::unordered_map<T, std::vector<bool>> map, T value)
      {
        const std::vector<bool>& bits = map[value];
        for (bool bit : bits)
          byter->addBit(bit);
      };

    Byter byter;
    for (int index : data)
      writeValue(&byter, huffmanMap, index);

    std::vector<char> ret;
    for (auto iter = heuristic.begin(); iter != heuristic.end(); iter++)
    {
      std::vector<char> byteData(sizeof(T) + sizeof(uint32_t));
      memcpy(byteData.data(), &iter->first, sizeof(T));
      memcpy(byteData.data() + sizeof(T), &iter->second, sizeof(uint32_t));

      ret.insert(ret.end(), byteData.begin(), byteData.end());
    }

    ret.insert(ret.end(), byter.data.begin(), byter.data.end());

    return ret;
  }

private:
protected:
  ModeE sizeMode;
};
*/
#endif