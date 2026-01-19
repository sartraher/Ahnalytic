#include "TreeSearch.hpp"
#include "AhnalyticBase/database/FileDatabase.hpp"
#include "AhnalyticBase/database/SnippedDatabase.hpp"
#include "AhnalyticBase/database/StackExchangeExtractDatabase.hpp"
#include "AhnalyticBase/helper/SSE2ASC2memcmp.hpp"
#include "AhnalyticBase/tree/SourceScanner.hpp"

#include "BS_thread_pool.hpp"

#include <fstream>

#define WINDOW_SIZE 64

TreeSearch::TreeSearch()
{
}

TreeSearch::~TreeSearch()
{
}

SearchNodes TreeSearch::initNodes(SourceStructureTree* tree, uint32_t windowSize) const
{
  SearchNodes ret;

  size_t nodecount = 0;
  nodecount = tree->getNodeCount();

  ret.lineNrs.reserve(nodecount);
  ret.nodeData.reserve(nodecount);

  std::vector<const Tree<SourceStructureData>*> nodeVec;
  nodeVec.reserve(nodecount);

  tree->getNodes(nodeVec);

  std::vector<uint32_t> nodeId;
  nodeId.reserve(nodeVec.size());
  for (int index = 0; index < nodeVec.size(); index++)
    nodeId.push_back(nodeVec[index]->data.id.cmpData);

  auto write = [&nodeVec, &ret](size_t pos, size_t size)
  {
    for (int index = 0; index < size; index++)
    {
      const Tree<SourceStructureData>* first = nodeVec[index + pos];
      ret.nodeData.push_back(first->data.id.cmpData);
      ret.lineNrs.push_back(first->data.lineNr);
    }
  };

  for (int index = 0; index < nodeVec.size(); index++)
  {
    bool foundDup = false;

    int nextIndex = index;
    int rest = (int)(nodeVec.size() - (nextIndex + 1));
    for (int dupIndex = 16; dupIndex > 1; dupIndex--)
    {
      bool hasMatch = false;

      if (nodeVec.size() <= nextIndex + dupIndex)
        continue;

      while (1)
      {
        if (nodeVec.size() <= nextIndex + dupIndex * 2)
          break;

        const uint32_t* base = &nodeId[nextIndex];
        const uint32_t* next = &nodeId[nextIndex + dupIndex];

        bool found = memcmp_equal(base, next, dupIndex * sizeof(uint32_t));

        if (found)
        {
          nextIndex += dupIndex;
          hasMatch = true;
        }
        else
          break;
      }

      if (hasMatch)
      {
        // Write dupIndex
        write(index, dupIndex);
        index = nextIndex;
        foundDup = true;
        break;
      }
    }

    if (!foundDup)
      write(index, 1);
  }

  ret.hashData.reserve(ret.nodeData.size());
  if (ret.nodeData.size() >= windowSize)
  {
    for (int index = 0; index < ret.nodeData.size() - windowSize; index++)
    {
      /*
      uint32_t hashValue = 0;
      for (int hashIndex = 0; hashIndex < windowSize; hashIndex++)
        hashValue ^= ret.nodeData[index + hashIndex];
      */

      /*
      uint32_t hashValue = 0x9e3779b9;
      for (int j = 0; j < windowSize; ++j) {
        hashValue ^= ret.nodeData[index + j] + 0x9e3779b9 + (hashValue << 6) + (hashValue >> 2);
      }
      */

      uint32_t hashValue = 2166136261u; // FNV offset
      for (uint32_t j = 0; j < windowSize; j++)
      {
        hashValue ^= (ret.nodeData[index + j] >> 16) & 0xFFFF;
        hashValue *= 16777619u; // FNV prime
      }

      ret.hashData[hashValue].push_back(index);
    }
  }

  return ret;
}

TreeSearchResult TreeSearch::searchTree(SourceStructureTree* base, SourceStructureTree* search, const TreeSearchOptions& options)
{
  SearchNodes baseNodes = initNodes(base, options.windowSize);
  SearchNodes searchNodes = initNodes(search, options.windowSize);

  return searchTree(baseNodes, searchNodes, options);
}

TreeSearchResult TreeSearch::searchTree(const SearchNodes& baseNodes, const SearchNodes& searchNodes, const TreeSearchOptions& options)
{
  TreeSearchResult ret;
  /*
  for (auto searchIter = searchNodes.indexLookup.begin(); searchIter != searchNodes.indexLookup.end(); searchIter++)
  {
    auto baseIter = baseNodes.indexLookup.find(searchIter->first);

    if (baseIter != baseNodes.indexLookup.end())
    {
      const std::vector<uint32_t>& lookUp = baseIter->second;

      for (uint32_t searchIndex : searchIter->second)
      {
        for (uint32_t curPos : lookUp)
        {
          if (curPos + options.windowSize < baseNodes.nodeData.size())
          {
            const uint32_t* baseArray = baseNodes.nodeData.data() + curPos;
            const uint32_t* searchArray = searchNodes.nodeData.data() + searchIndex;

            //if (memcmp(baseArray, searchArray, options.minDistance * sizeof(uint32_t)) == 0)
            if (memcmp_equal(baseArray, searchArray, options.windowSize * sizeof(uint32_t)))
            {
              TreeSearchResultSet resultSet;
              resultSet.baseStart = baseNodes.lineNrs.at(curPos);
              resultSet.baseEnd = baseNodes.lineNrs.at(curPos + options.windowSize);
              resultSet.searchStart = searchNodes.lineNrs.at(searchIndex);
              resultSet.searchEnd = searchNodes.lineNrs.at(searchIndex + options.windowSize);

              ret.push_back(resultSet);
              //index += options.minDistance;
              break;
            }
          }
        }
      }
    }
  }
  */

  return ret;
}

TreeSearchResult TreeSearch::searchHash(const SearchNodes& baseNodes, const SearchNodes& searchNodes, int windowSize, bool fast)
{
  TreeSearchResult ret;

  std::set<size_t> doneBaseLines;
  std::set<size_t> doneSearchLines;

  for (auto baseIter = baseNodes.hashData.begin(); baseIter != baseNodes.hashData.end(); baseIter++)
  {
    auto searchIter = searchNodes.hashData.find(baseIter->first);

    if (searchIter != searchNodes.hashData.end())
    {
      for (const uint32_t& baseIndex : baseIter->second)
      {
        if (doneBaseLines.size() > 0)
        {
          int baseStart = baseNodes.lineNrs.at(baseIndex);
          if (doneBaseLines.contains(baseStart))
            continue;
        }
        for (const uint32_t& searchIndex : searchIter->second)
        {
          if (doneSearchLines.size() > 0)
          {
            int searchStart = searchNodes.lineNrs.at(searchIndex);
            if (doneSearchLines.contains(searchStart))
              continue;
          }

          if (memcmp_equal(&baseNodes.nodeData[baseIndex], &searchNodes.nodeData[searchIndex], windowSize * sizeof(uint32_t)))
          {
            if (fast)
            {
              // We will do a deep search either way
              TreeSearchResultSet resultSet;
              ret.push_back(resultSet);
              return ret;
            }
            else
            {
              TreeSearchResultSet resultSet;
              resultSet.baseStart = baseNodes.lineNrs.at(baseIndex);
              resultSet.baseEnd = baseNodes.lineNrs.at(baseIndex + windowSize);
              resultSet.searchStart = searchNodes.lineNrs.at(searchIndex);
              resultSet.searchEnd = searchNodes.lineNrs.at(searchIndex + windowSize);

              for (uint32_t index = resultSet.baseStart; index <= resultSet.baseEnd; index++)
                doneBaseLines.insert(index);

              for (uint32_t index = resultSet.searchStart; index <= resultSet.searchEnd; index++)
                doneSearchLines.insert(index);

              ret.push_back(resultSet);
              break;
            }
          }
        }
      }
    }
  }

  return ret;
}

void TreeSearch::search(std::filesystem::path& path, const EnviromentC& env, TreeResultInterface* resultInter)
{
  std::list<std::future<void>> currentTasks;

  // first lets convert out input data
  SourceScanner scanner;

  std::unordered_map<std::string, std::vector<ScanTreeData>> trees = scanner.scanPath(path);

  std::unordered_map<std::string, std::vector<SearchNodes>> searchNodes;

  for (auto iter = trees.begin(); iter != trees.end(); iter++)
  {
    std::vector<SearchNodes>& nodes = searchNodes[iter->first];
    nodes.resize(iter->second.size());

    for (int index = 0; index < iter->second.size(); index++)
    {
      ScanTreeData& data = iter->second[index];
      nodes[index] = initNodes(data.tree, WINDOW_SIZE);
      nodes[index].filePath = data.path;
      delete data.tree;
    }
  }
  trees.clear();

  auto scanSnippedDb = [this, resultInter](const std::filesystem::path& dbPath, const std::vector<SearchNodes>& nodes)
  {
    SnippedDatabase db(DBType::SQLite, dbPath.string());

    db.iterateSnippeds([this, &nodes, dbPath, resultInter](uint32_t internalId, const std::string& licence, SourceStructureTree* tree)
    {
      if (resultInter->isAborted())
        return;

      SearchNodes dbNodes = initNodes(tree, WINDOW_SIZE);

      for (const SearchNodes& searchNodes : nodes)
      {
        if (resultInter->isAborted())
          return;

        TreeSearchResult result = searchHash(dbNodes, searchNodes, WINDOW_SIZE, true);
        if (result)
        {
          result.type = TreeSearchResult::Stackexchange;
          result.sourceDb = dbPath.string();
          result.sourceInternalId = internalId;
          result.searchFile = searchNodes.filePath.string();

          resultInter->addResult(result);
        }
      }
    });

    resultInter->incFinishedCount(1);
  };

  auto scanGitHubDb = [this, resultInter](const std::filesystem::path& dbPath, const std::vector<SearchNodes>& nodes)
  {
    FileDatabase db(DBType::SQLite, dbPath.string());

    db.iterateFiles([this, &nodes, resultInter, dbPath](uint32_t fileId, const std::string& sha, const std::string& licence, SourceStructureTree* tree)
    {
      if (resultInter->isAborted())
        return;

      SearchNodes dbNodes = initNodes(tree, WINDOW_SIZE);

      for (const SearchNodes& searchNodes : nodes)
      {
        if (resultInter->isAborted())
          return;

        TreeSearchResult result = searchHash(dbNodes, searchNodes, WINDOW_SIZE, true);
        if (result)
        {
          result.type = TreeSearchResult::Github;
          result.sourceDb = dbPath.string();
          result.sourceRevision = sha;
          result.sourceInternalId = fileId;
          result.searchFile = searchNodes.filePath.string();

          resultInter->addResult(result);
        }
      }
    });

    resultInter->incFinishedCount(1);
  };

  auto scanSourceforgeDb = [this, resultInter](const std::filesystem::path& dbPath, const std::vector<SearchNodes>& nodes)
  {
    FileDatabase db(DBType::SQLite, dbPath.string());

    db.iterateFiles([this, &nodes, resultInter, dbPath](uint32_t fileId, const std::string& revision, const std::string& licence, SourceStructureTree* tree)
    {
      if (resultInter->isAborted())
        return;

      SearchNodes dbNodes = initNodes(tree, WINDOW_SIZE);

      for (const SearchNodes& searchNodes : nodes)
      {
        if (resultInter->isAborted())
          return;

        TreeSearchResult result = searchHash(dbNodes, searchNodes, WINDOW_SIZE, true);
        if (result)
        {
          result.type = TreeSearchResult::SourceForge;
          result.sourceDb = dbPath.string();
          result.sourceRevision = revision;
          result.sourceInternalId = fileId;
          result.searchFile = searchNodes.filePath.string();

          resultInter->addResult(result);
        }
      }
    });

    resultInter->incFinishedCount(1);
  };

  int maxCount = 0;

  BS::thread_pool pool;
  for (auto iter = searchNodes.begin(); iter != searchNodes.end(); iter++)
  {
    std::filesystem::path dbByFormatPath = env.dbFolder / iter->first;

    // stackexchange
    std::filesystem::path dbStackExchnage = dbByFormatPath / "stackexchange";
    if (std::filesystem::exists(dbStackExchnage))
    {
      for (auto& filePath : std::filesystem::recursive_directory_iterator(dbStackExchnage))
      {
        if (resultInter->isAborted())
          return;

        std::filesystem::path dbPath = filePath.path();
        if (dbPath.extension() == ".db")
        {
          std::vector<SearchNodes> nodes = iter->second;
          std::future<void> result = pool.submit_task([&scanSnippedDb, dbPath, nodes]() { return scanSnippedDb(dbPath, nodes); });
          currentTasks.push_back(std::move(result));
          maxCount++;
        }
      }
    }

    // github
    std::filesystem::path dbGithub = dbByFormatPath / "github";
    if (std::filesystem::exists(dbGithub))
    {
      for (auto& filePath : std::filesystem::recursive_directory_iterator(dbGithub))
      {
        if (resultInter->isAborted())
          return;

        std::filesystem::path dbPath = filePath.path();
        if (dbPath.extension() == ".db")
        {
          std::vector<SearchNodes> nodes = iter->second;
          std::future<void> result = pool.submit_task([&scanGitHubDb, dbPath, nodes]() { return scanGitHubDb(dbPath, nodes); });
          currentTasks.push_back(std::move(result));
          maxCount++;
        }
      }
    }

    // sourceforge
    std::filesystem::path dbSourceForge = dbByFormatPath / "sourceforge";
    if (std::filesystem::exists(dbSourceForge))
    {
      for (auto& filePath : std::filesystem::recursive_directory_iterator(dbSourceForge))
      {
        if (resultInter->isAborted())
          return;

        std::filesystem::path dbPath = filePath.path();
        if (dbPath.extension() == ".db")
        {
          std::vector<SearchNodes> nodes = iter->second;
          std::future<void> result = pool.submit_task([&scanSourceforgeDb, dbPath, nodes]() { return scanSourceforgeDb(dbPath, nodes); });
          currentTasks.push_back(std::move(result));
          maxCount++;
        }
      }
    }
  }

  if (resultInter->isAborted())
    return;

  resultInter->setMaxCount(maxCount+1);

  for (std::future<void>& task : currentTasks)
  {
    if (resultInter->isAborted())
      return;

    task.get();
  }
}

void TreeSearch::searchDeep(std::filesystem::path& path, const EnviromentC& env, TreeResultInterface* resultInter)
{
  std::vector<TreeSearchResult> fastResults = resultInter->getResult();

  struct DeepScanData
  {
    std::string content;
    SourceStructureTree* tree;
  };

  SourceScanner scanner;
  std::unordered_map<std::string, DeepScanData> trees;

  for (const TreeSearchResult& result : fastResults)
  {
    auto iter = trees.find(result.searchFile);
    if (iter == trees.end())
    {
      uint32_t resSize;
      std::string sourceType;

      std::ifstream in(result.searchFile, std::ios::binary);
      if (!in.is_open())
        continue;

      std::ostringstream ss;
      ss << in.rdbuf();
      std::string content = ss.str();

      std::filesystem::path path = result.searchFile;
      SourceStructureTree* tree = scanner.scan(path, resSize, sourceType);
      trees[result.searchFile] = {content, tree};
    }

    std::string licence;
    std::string cmpFile;
    switch (result.type)
    {
    case TreeSearchResult::Github:
      cmpFile = getGitHubFile(result.sourceDb, result.sourceInternalId, result.sourceRevision, licence);
      break;
    case TreeSearchResult::SourceForge:
      cmpFile = getSourceForgeFile(result.sourceDb, result.sourceInternalId, result.sourceRevision, licence);
      break;
    case TreeSearchResult::Stackexchange:
      cmpFile = getStackexchangeFile(result.sourceDb, result.sourceInternalId, licence);
      break;
    }

    uint32_t resSize;
    std::string sourceType;
    SourceStructureTree* dbTree = scanner.scan(cmpFile, resSize, sourceType);

    if (dbTree == nullptr)
    {
      // TODO: add error, should not happen
      continue;
    }

    SearchNodes dbNodes = initNodes(dbTree, WINDOW_SIZE);
    SearchNodes searchNodes = initNodes(trees[result.searchFile].tree, WINDOW_SIZE);

    TreeSearchResult result = searchHash(dbNodes, searchNodes, WINDOW_SIZE, false);
    if (result)
    {
      result.sourceContent = trees[result.searchFile].content;
      result.searchContent = cmpFile;
      result.licence = licence;

      resultInter->addDeepResult(result);
    }
  }

  resultInter->incFinishedCount(1);
}

std::string TreeSearch::getGitHubFile(const std::string& sourceDb, const uint32_t& fileId, const std::string& sha, std::string& licence)
{
  std::string ret;
  // TODO
  return ret;
}

std::string TreeSearch::getSourceForgeFile(const std::string& sourceDb, const uint32_t& fileId, const std::string& sourceRevision, std::string& licence)
{
  std::string ret;
  // TODO
  return ret;
}

std::string TreeSearch::getStackexchangeFile(const std::string& sourceDb, const uint32_t& sourceInternalId, std::string& licence)
{
  std::string ret;

  auto replace = [](std::string& str, const std::string& from, const std::string& to)
  {
    size_t start_pos = str.find(from);
    if (start_pos == std::string::npos)
      return str;

    str.replace(start_pos, from.length(), to);
    return str;
  };

  // Hack for now
  std::string dataDb = sourceDb;
  dataDb = replace(dataDb, "_CPP.db", ".db");
  dataDb = replace(dataDb, "CPP", "base");

  std::string date;
  StackExchangeExtractDatabase extractDb(DBType::SQLite, dataDb);
  extractDb.getSnipped(std::to_string(sourceInternalId), date, licence, ret);

  return ret;
}