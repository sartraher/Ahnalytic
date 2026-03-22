#include "gtest/gtest.h"

#include "AhnalyticBase/Export.hpp"
#include "AhnalyticBase/tree/SourceScanner.hpp"
#include "AhnalyticBase/tree/SourceStructureTree.hpp"

#include <filesystem>

void cmpTree(SourceStructureTree* tree)
{
  SourceStructureTree* root = new SourceStructureTree();
  root->children.push_back(tree);

  ahn::vector<FlatNodeDeDupData> deduped;
  ahn::vector<uint32_t> indexList;
  reduceTree(root, deduped, indexList);

  ahn::vector<char> result;
  SourceStructureTree::serialize(deduped, indexList, result, nullptr);

  ahn::vector<FlatNodeDeDupData> nodeListTestOut;
  ahn::vector<uint32_t> indexListTestOut;
  SourceStructureTree::deserialize(result, nodeListTestOut, indexListTestOut, nullptr);

  auto restoredTree = rebuildTree(nodeListTestOut, indexListTestOut);
  bool isEq = ((*restoredTree) == (*root));
  EXPECT_TRUE(isEq);
}

#define TESTTREE(name)                                                                                                                                         \
  TEST(TreeCompressionTest, TreeCompression##name)                                                                                                             \
  {                                                                                                                                                            \
    std::filesystem::path path = std::filesystem::current_path();                                                                                              \
    path += "/../../tests/data/treecompression/" #name ".cpp";                                                                                                 \
    SourceScanner scanner;                                                                                                                                     \
    std::string sourceType;                                                                                                                                    \
    uint32_t resSize;                                                                                                                                          \
    cmpTree(scanner.scan(path, resSize, sourceType));                                                                                                          \
  }

TESTTREE(test1)
TESTTREE(test2)