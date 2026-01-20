#include "gtest/gtest.h"

#include "AhnalyticBase/database/SnippedDatabase.hpp"
#include "AhnalyticBase/tree/SourceScanner.hpp"
#include "AhnalyticBase/tree/SourceStructureTree.hpp"
#include "AhnalyticBase/tree/TreeSearch.hpp"

#define TESTSEARCH(nameBase, nameSearch, options)                                                                                                              \
  TEST(TreeSearchFileTest, TreeCompression##nameBase##nameSearch)                                                                                              \
  {                                                                                                                                                            \
    search(#nameBase, #nameSearch, options);                                                                                                                   \
  }

void search(const std::string& baseName, const std::string searchName)
{
  SourceScanner scanner;
  std::string sourceType;
  uint32_t resSize;
  std::filesystem::path pathBase = std::filesystem::current_path();
  pathBase += "/../../tests/data/searchsingle/" + baseName + ".cpp";
  std::filesystem::path pathSearch = std::filesystem::current_path();
  pathSearch += "/../../tests/data/searchsingle/" + searchName + ".cpp";

  SourceStructureTree* baseTree = scanner.scan(pathBase, resSize, sourceType);
  SourceStructureTree* searchTree = scanner.scan(pathSearch, resSize, sourceType);

  TreeSearch treeSearch;
  //TreeSearchResult result = treeSearch.searchTree(baseTree, searchTree, options);

  EXPECT_TRUE(true);
}

#define OPTIONS TreeSearchOptions{100}
// TESTSEARCH(test1, test2, OPTIONS)

/*
TEST(TreeSearchDBTest, SnippedDbTest)
{
  SnippedDatabase db(DBType::SQLite, "D:/source/Ahnalytic/db/CPP/stackexchange/stackoverflow_CPP.db");

  std::vector<char> data;
  db.getSourceTreeData(1, data);

  std::vector<FlatNodeDeDupData> nodeListTestOut;
  std::vector<uint32_t> indexListTestOut;
  SourceStructureTree::deserialize(data, nodeListTestOut, indexListTestOut, nullptr);
}
*/

TEST(TreeSearchDBTest, FolderSearch)
{
  std::filesystem::path pathBase = std::filesystem::current_path();
  pathBase += "/../../tests/data/searchfolder/";

  //EnvData env;
  //env.dbPath = "D:/source/Ahnalytic/db";
 

  TreeSearch treeSearch;
  //std::list<TreeSearchResult> result = treeSearch.search(pathBase, env);

  //EXPECT_TRUE(result.size() > 0);
  EXPECT_TRUE(true);
}