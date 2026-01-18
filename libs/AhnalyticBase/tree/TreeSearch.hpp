#ifndef treesearch_hpp__
#define treesearch_hpp__

#include "AhnalyticBase/helper/Enviroment.hpp"
#include "AhnalyticBase/tree/SourceStructureTree.hpp"

#include <filesystem>
#include <map>
#include <set>
#include <vector>

#include <ankerl/unordered_dense.h>

struct TreeSearchOptions
{
  uint32_t windowSize;

  /*
  uint32_t minComplexitiyBase;
  uint32_t maxComplexitiyBase;

  uint32_t minComplexitiySearch;
  uint32_t maxComplexitiySearch;

  double searchEpsilon;
  */
};

struct TreeSearchResultSet
{
  uint32_t baseStart = 0;
  uint32_t baseEnd = 0;

  uint32_t searchStart = 0;
  uint32_t searchEnd = 0;
};

class TreeSearchResult : public std::vector<TreeSearchResultSet>
{
public:
  enum ResultSourceTypeE
  {
    Stackexchange,
    Github,
    SourceForge
  };

  TreeSearchResult() {};

  std::string sourceDb;
  std::string sourceFile;
  std::string sourceRevision;

  uint32_t sourceInternalId = 0;

  std::string searchFile;
  ResultSourceTypeE type;

  std::string sourceContent;
  std::string searchContent;

  operator bool() const
  {
    return size() > 0;
  }
};

class TreeResultInterface
{
public:
  virtual bool isAborted() = 0;

  virtual void addResult(const TreeSearchResult& result) = 0;
  virtual std::vector<TreeSearchResult> getResult() = 0;

  virtual void addDeepResult(const TreeSearchResult& result) = 0;
  virtual std::vector<TreeSearchResult> getDeepResult() = 0;
};

struct SearchNodes
{
  // std::unordered_map<uint32_t, std::vector<uint32_t>> hashData;
  ankerl::unordered_dense::map<uint32_t, std::vector<uint32_t>> hashData;

  std::vector<uint32_t> nodeData;
  std::vector<uint32_t> lineNrs;

  std::filesystem::path filePath;
};

class DLLEXPORT TreeSearch
{
public:
  TreeSearch();
  ~TreeSearch();

  SearchNodes initNodes(SourceStructureTree* tree, uint32_t windowSize) const;

  TreeSearchResult searchTree(SourceStructureTree* base, SourceStructureTree* search, const TreeSearchOptions& options);
  TreeSearchResult searchTree(const SearchNodes& baseNodes, const SearchNodes& searchNodes, const TreeSearchOptions& options);

  TreeSearchResult searchHash(const SearchNodes& baseNodes, const SearchNodes& searchNodes, int windowSize, bool fast);

  void search(std::filesystem::path& path, const EnviromentC& env, TreeResultInterface* resultInter);
  void searchDeep(std::filesystem::path& path, const EnviromentC& env, TreeResultInterface* resultInter);

private:
protected:
  std::string getGitHubFile(const std::string& sourceDb, const uint32_t& fileId, const std::string& sha);
  std::string getSourceForgeFile(const std::string& sourceDb, const uint32_t& fileId, const std::string& sourceRevision);
  std::string getStackexchangeFile(const std::string& sourceDb, const uint32_t& sourceInternalId);
};

#endif