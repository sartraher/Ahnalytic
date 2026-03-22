#include "TreeSearch.hpp"
#include "AhnalyticBase/cli/GitCliHelper.hpp"
#include "AhnalyticBase/database/FileDatabase.hpp"
#include "AhnalyticBase/database/SnippedDatabase.hpp"
#include "AhnalyticBase/database/StackExchangeExtractDatabase.hpp"
#include "AhnalyticBase/helper/DataHelper.hpp"
#include "AhnalyticBase/helper/SSE2ASC2memcmp.hpp"
#include "AhnalyticBase/tree/SourceScanner.hpp"

#define CPPHTTPLIB_OPENSSL_SUPPORT
#define CPPHTTPLIB_ZLIB_SUPPORT
#include <httplib.h>

#include "BS_thread_pool.hpp"

#include <fstream>

// #define WINDOW_SIZE 64

struct IdentityHash
{
  size_t operator()(uint32_t x) const noexcept
  {
    return x;
  }
};

TreeSearch::TreeSearch()
{
}

TreeSearch::~TreeSearch()
{
}

void TreeSearch::initNodeData(SearchNodeData& searchData, SourceStructureTree* tree, const std::filesystem::path& path, uint32_t windowSize)
{
  size_t pathId = 0;
  auto pathIter = searchData.pathLookup.find(path);
  if (pathIter == searchData.pathLookup.end())
  {
    pathId = searchData.pathLookup.size();
    searchData.pathLookup[path] = pathId;
    searchData.pathLookupReverse[pathId] = path;
  }
  else
    pathId = pathIter->second;

  size_t nodeCount = 0;
  nodeCount = tree->getNodeCount();

  size_t origSize = searchData.nodeData.size();

  // searchData.nodeData.reserve(searchData.nodeData.size() + nodeCount);

  ahn::vector<const Tree<SourceStructureData>*> nodeVec;
  nodeVec.reserve(nodeCount);

  tree->getNodes(nodeVec);

  ahn::vector<uint32_t> nodeId;
  nodeId.reserve(nodeVec.size());
  for (int index = 0; index < nodeVec.size(); index++)
    nodeId.push_back(nodeVec[index]->data.id.cmpData);

  auto write = [&nodeVec, &searchData](size_t pos, size_t size)
  {
    for (int index = 0; index < size; index++)
    {
      const Tree<SourceStructureData>* first = nodeVec[index + pos];
      searchData.nodeData.push_back(first->data.id.cmpData);
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

  constexpr uint32_t base = 0x9E3779B1u;

  static thread_local ahn::vector<uint32_t> powB;
  if (powB.size() < windowSize + 1)
  {
    powB.resize(windowSize + 1);
    powB[0] = 1;
    for (size_t i = 1; i <= windowSize; ++i)
      powB[i] = powB[i - 1] * base;
  }

  if (searchData.nodeData.size() >= origSize + windowSize)
  {
    uint32_t hash = 0;
    for (size_t i = origSize; i < origSize + windowSize; ++i)
      hash = hash * base + searchData.nodeData[i];

    searchData.searchData[hash][pathId].push_back(0);

    for (size_t index = origSize + 1; index <= searchData.nodeData.size() - windowSize; ++index)
    {
      uint32_t outgoing = searchData.nodeData[index - 1];
      uint32_t incoming = searchData.nodeData[index + windowSize - 1];

      hash = (hash - outgoing * powB[windowSize - 1]) * base + incoming;

      searchData.searchData[hash][pathId].push_back(index);
    }
  }
}

void TreeSearch::collectHashData(ankerl::unordered_dense::set<uint32_t>& hashes, SourceStructureTree* tree, uint32_t windowSize)
{
  ahn::vector<uint32_t> nodeData;

  size_t nodeCount = 0;
  nodeCount = tree->getNodeCount();

  ahn::vector<const Tree<SourceStructureData>*> nodeVec;
  nodeVec.reserve(nodeCount);
  nodeData.reserve(nodeCount);

  tree->getNodes(nodeVec);

  ahn::vector<uint32_t> nodeId;
  nodeId.reserve(nodeVec.size());
  for (int index = 0; index < nodeVec.size(); index++)
    nodeId.push_back(nodeVec[index]->data.id.cmpData);

  auto write = [&nodeVec, &nodeData](size_t pos, size_t size)
  {
    for (int index = 0; index < size; index++)
    {
      const Tree<SourceStructureData>* first = nodeVec[index + pos];
      nodeData.push_back(first->data.id.cmpData);
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

  constexpr uint32_t base = 0x9E3779B1u;

  static thread_local ahn::vector<uint32_t> powB;
  if (powB.size() < windowSize + 1)
  {
    powB.resize(windowSize + 1);
    powB[0] = 1;
    for (size_t i = 1; i <= windowSize; ++i)
      powB[i] = powB[i - 1] * base;
  }

  if (nodeData.size() >= windowSize)
  {
    uint32_t hash = 0;
    for (size_t i = 0; i < windowSize; ++i)
      hash = hash * base + nodeData[i];

    hashes.insert(hash);

    for (size_t index = 1; index <= nodeData.size() - windowSize; ++index)
    {
      uint32_t outgoing = nodeData[index - 1];
      uint32_t incoming = nodeData[index + windowSize - 1];

      hash = (hash - outgoing * powB[windowSize - 1]) * base + incoming;

      hashes.insert(hash);
    }
  }
}

std::set<std::filesystem::path> TreeSearch::searchRawHash(const SearchNodeData& dbNodes, SourceStructureTree* tree, const std::filesystem::path& path,
                                                          uint32_t windowSize)
{
  std::set<std::filesystem::path> ret;

  size_t nodeCount = 0;
  nodeCount = tree->getNodeCount();

  ahn::vector<uint32_t> nodeData;
  nodeData.reserve(nodeCount);

  ahn::vector<const Tree<SourceStructureData>*> nodeVec;
  nodeVec.reserve(nodeCount);

  tree->getNodes(nodeVec);

  ahn::vector<uint32_t> nodeId;
  nodeId.reserve(nodeVec.size());
  for (int index = 0; index < nodeVec.size(); index++)
    nodeId.push_back(nodeVec[index]->data.id.cmpData);

  auto write = [&nodeVec, &nodeData](size_t pos, size_t size)
  {
    for (int index = 0; index < size; index++)
    {
      const Tree<SourceStructureData>* first = nodeVec[index + pos];
      nodeData.push_back(first->data.id.cmpData);
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

  // Tuned odd constant with good avalanche under mod 2^32
  constexpr uint32_t base = 0x9E3779B1u; // 2^32 / golden ratio

  static thread_local ahn::vector<uint32_t> powB;
  if (powB.size() < windowSize + 1)
  {
    powB.resize(windowSize + 1);
    powB[0] = 1;
    for (size_t i = 1; i <= windowSize; ++i)
      powB[i] = powB[i - 1] * base;
  }

  auto searchFunc = [&dbNodes, &nodeData, &ret, windowSize](uint32_t hash, int index)
  {
    auto searchIter = dbNodes.searchData.find(hash);

    if (searchIter != dbNodes.searchData.end())
    {
      for (auto searchFileIter = searchIter->second.begin(); searchFileIter != searchIter->second.end(); searchFileIter++)
      {
        for (const uint32_t& searchIndex : searchFileIter->second)
        {
          if (memcmp_equal(&nodeData[index], &dbNodes.nodeData[searchIndex], windowSize * sizeof(uint32_t)))
          {
            ret.insert(dbNodes.pathLookupReverse.find(searchFileIter->first)->second);
          }
        }
      }
    }
  };

  if (nodeData.size() >= windowSize)
  {
    uint32_t hash = 0;
    for (size_t i = 0; i < windowSize; ++i)
      hash = hash * base + nodeData[i];

    searchFunc(hash, 0);

    for (size_t index = 1; index <= nodeData.size() - windowSize; ++index)
    {
      uint32_t outgoing = nodeData[index - 1];
      uint32_t incoming = nodeData[index + windowSize - 1];

      hash = (hash - outgoing * powB[windowSize - 1]) * base + incoming;
      searchFunc(hash, index);
    }
  }

  return ret;
}

SearchNodes TreeSearch::initNodesDeep(SourceStructureTreeDeep* tree, uint32_t windowSize) const
{
  SearchNodes ret;

  size_t nodecount = 0;
  nodecount = tree->getNodeCount();

  ret.lineNrs.reserve(nodecount);
  ret.nodeData.reserve(nodecount);

  ahn::vector<const Tree<SourceStructureDeepData>*> nodeVec;
  nodeVec.reserve(nodecount);

  tree->getNodes(nodeVec);

  ahn::vector<uint32_t> nodeId;
  nodeId.reserve(nodeVec.size());
  for (int index = 0; index < nodeVec.size(); index++)
    nodeId.push_back(nodeVec[index]->data.id.cmpData);

  for (int index = 0; index < nodeVec.size(); index++)
  {
    const Tree<SourceStructureDeepData>* first = nodeVec[index];
    ret.nodeData.push_back(first->data.id.cmpData);
    ret.nameData.push_back(first->data.name);
    ret.lineNrs.push_back(first->data.lineNr);
  }

  ret.hashData.reserve(ret.nodeData.size());
  if (ret.nodeData.size() >= windowSize)
  {
    constexpr uint32_t base = 0x9E3779B1u;

    static thread_local ahn::vector<uint32_t> powB;
    if (powB.size() < windowSize + 1)
    {
      powB.resize(windowSize + 1);
      powB[0] = 1;
      for (size_t i = 1; i <= windowSize; ++i)
        powB[i] = powB[i - 1] * base;
    }

    if (ret.nodeData.size() >= windowSize)
    {
      uint32_t hash = 0;
      for (size_t i = 0; i < windowSize; ++i)
        hash = hash * base + ret.nodeData[i];

      ret.hashData[hash].push_back(0);

      for (size_t index = 1; index <= ret.nodeData.size() - windowSize; ++index)
      {
        uint32_t outgoing = ret.nodeData[index - 1];
        uint32_t incoming = ret.nodeData[index + windowSize - 1];

        hash = (hash - outgoing * powB[windowSize - 1]) * base + incoming;

        ret.hashData[hash].push_back(index);
      }
    }
  }

  return ret;
}

TreeSearchResult TreeSearch::searchHash(const SearchNodes& baseNodes, const SearchNodes& searchNodes, int windowSize)
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
            bool nameNEQ = false;
            for (int index = 0; index < windowSize && !nameNEQ; index++)
              if (baseNodes.nameData[baseIndex + index] != searchNodes.nameData[searchIndex + index])
                nameNEQ = true;

            if (!nameNEQ)
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

  ankerl::unordered_dense::map<std::string, SearchNodeData> searchData;

  for (auto iter = trees.begin(); iter != trees.end(); iter++)
  {
    SearchNodeData& searchDataByLang = searchData[iter->first];

    size_t nodeCount = 0;
    for (int index = 0; index < iter->second.size(); index++)
    {
      ScanTreeData& data = iter->second[index];
      nodeCount += data.tree->getNodeCount();
    }

    searchDataByLang.nodeData.reserve(nodeCount);

    for (int index = 0; index < iter->second.size(); index++)
    {
      ScanTreeData& data = iter->second[index];
      initNodeData(searchDataByLang, data.tree, data.path, env.windowSize);
      delete data.tree;
    }
  }
  trees.clear();

  auto checkHash = [](const auto& smallContainer, const auto& bigContainer) -> bool
  {
    for (auto iter = smallContainer.begin(); iter != smallContainer.end(); iter++)
    {
      auto bigIter = bigContainer.find(iter.first);
      if (bigIter != bigContainer.end())
        return true;
    }

    return false;
  };

  auto checkIndex = [env, &checkHash](const std::filesystem::path& dbPath, const SearchNodeData& nodes) -> bool
  {
    bool ret = false;

    std::filesystem::path indexPath = dbPath;
    indexPath.replace_extension("." + std::to_string(env.windowSize));

    if (!std::filesystem::exists(indexPath))
    {
      // TODO: create missing index
    }

    ankerl::unordered_dense::set<uint32_t> hashes;

    if (std::filesystem ::exists(indexPath))
    {
      std::ifstream file(indexPath, std::ios::binary | std::ios::ate);
      std::streamsize size = file.tellg();
      file.seekg(0, std::ios::beg);
      ahn::vector<uint32_t> data(size / sizeof(uint32_t));

      if (file.read(reinterpret_cast<char*>(data.data()), size))
      {
        hashes.reserve(data.size());
        hashes.insert(data.begin(), data.end());
      }

      if (hashes.size() < nodes.searchData.size())
      {
        for (auto iter = hashes.begin(); iter != hashes.end(); iter++)
        {
          auto bigIter = nodes.searchData.find(*iter);
          if (bigIter != nodes.searchData.end())
            return true;
        }

        return false;
      }
      else
      {
        for (auto iter = nodes.searchData.begin(); iter != nodes.searchData.end(); iter++)
        {
          auto bigIter = hashes.find(iter->first);
          if (bigIter != hashes.end())
            return true;
        }

        return false;
      }
    }

    return true;
  };

  auto scanSnippedDb = [this, resultInter, env, &checkIndex](const std::filesystem::path& dbPath, const SearchNodeData& nodes)
  {
    return;
    if (!checkIndex(dbPath, nodes))
    {
      resultInter->incFinishedCount(1);
      return;
    }

    SnippedDatabase db(DBType::SQLite, dbPath.string());

    db.iterateSnippeds([this, &nodes, dbPath, resultInter, env](uint32_t internalId, const std::string& licence, SourceStructureTree* tree)
    {
      if (resultInter->isAborted())
        return;

      std::set<std::filesystem::path> files = searchRawHash(nodes, tree, std::to_string(internalId), env.windowSize);
      for (const std::filesystem::path& path : files)
      {
        TreeSearchResult result;
        result.type = TreeSearchResult::Stackexchange;
        result.sourceDb = dbPath.string();
        result.sourceInternalId = internalId;
        result.searchFile = path.string();

        resultInter->addResult(result);
      }
    });

    resultInter->incFinishedCount(1);
  };

  auto scanGitHubDb = [this, resultInter, env, &checkIndex](const std::filesystem::path& dbPath, const SearchNodeData& nodes)
  {
    if (!checkIndex(dbPath, nodes))
    {
      resultInter->incFinishedCount(1);
      return;
    }

    FileDatabase db(DBType::SQLite, dbPath.string());

    db.iterateFiles([this, &nodes, resultInter, dbPath, env](uint32_t fileId, const std::string& sha, const std::string& licence, SourceStructureTree* tree)
    {
      if (resultInter->isAborted())
        return;

      std::set<std::filesystem::path> files = searchRawHash(nodes, tree, std::to_string(fileId), env.windowSize);
      for (const std::filesystem::path& path : files)
      {
        TreeSearchResult result;
        result.type = TreeSearchResult::Github;
        result.sourceDb = dbPath.string();
        result.sourceRevision = sha;
        result.sourceInternalId = fileId;
        result.searchFile = path.string();

        resultInter->addResult(result);
      }
    });

    resultInter->incFinishedCount(1);
  };

  auto scanSourceforgeDb = [this, resultInter, env, &checkIndex](const std::filesystem::path& dbPath, const SearchNodeData& nodes)
  {
    if (!checkIndex(dbPath, nodes))
    {
      resultInter->incFinishedCount(1);
      return;
    }

    FileDatabase db(DBType::SQLite, dbPath.string());

    db.iterateFiles(
        [this, &nodes, resultInter, dbPath, env](uint32_t fileId, const std::string& revision, const std::string& licence, SourceStructureTree* tree)
    {
      if (resultInter->isAborted())
        return;

      std::set<std::filesystem::path> files = searchRawHash(nodes, tree, std::to_string(fileId), env.windowSize);
      for (const std::filesystem::path& path : files)
      {
        TreeSearchResult result;
        result.type = TreeSearchResult::SourceForge;
        result.sourceDb = dbPath.string();
        result.sourceRevision = revision;
        result.sourceInternalId = fileId;
        result.searchFile = path.string();

        resultInter->addResult(result);
      }
    });

    resultInter->incFinishedCount(1);
  };

  int maxCount = 0;

  BS::thread_pool pool;
  for (auto iter = searchData.begin(); iter != searchData.end(); iter++)
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
          const SearchNodeData& nodes = iter->second;
          std::future<void> result = pool.submit_task([&scanSnippedDb, dbPath, &nodes]() { return scanSnippedDb(dbPath, nodes); });
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
          const SearchNodeData& nodes = iter->second;
          std::future<void> result = pool.submit_task([&scanGitHubDb, dbPath, &nodes]() { return scanGitHubDb(dbPath, nodes); });
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
          const SearchNodeData& nodes = iter->second;
          std::future<void> result = pool.submit_task([&scanSourceforgeDb, dbPath, &nodes]() { return scanSourceforgeDb(dbPath, nodes); });
          currentTasks.push_back(std::move(result));
          maxCount++;
        }
      }
    }
  }

  if (resultInter->isAborted())
    return;

  resultInter->setMaxCount(maxCount);

  for (std::future<void>& task : currentTasks)
  {
    if (resultInter->isAborted())
      return;

    task.get();
  }
}

void TreeSearch::searchDeep(std::filesystem::path& path, const EnviromentC& env, TreeResultInterface* resultInter)
{
  if (resultInter->isAborted())
    return;

  std::vector<TreeSearchResult> fastResults = resultInter->getResult();

  resultInter->setDeepMaxCount(fastResults.size());

  std::unordered_map<std::string, std::vector<TreeSearchResult>> resultByCatalog;
  for (const TreeSearchResult& entry : fastResults)
    resultByCatalog[entry.sourceDb].push_back(entry);

  SourceScanner scanner;
  struct FileData
  {
    std::string cmpFile;
    std::string sourceFile;
    std::string sourceFileVis;
    SourceStructureTreeDeep* dbTree = nullptr;
    uint32_t resSize;
    std::string sourceType;
    std::string licence;

    TreeSearchResult entry;
  };

  BS::thread_pool pool;

  std::vector<std::string> doneList;
  std::unordered_map<TreeSearchResult::ResultSourceTypeE, std::unordered_map<std::string, FileData>> cache;
  std::vector<std::future<ahn::vector<FileData>>> tasks;

  for (auto iter = resultByCatalog.begin(); iter != resultByCatalog.end(); iter++)
  {
    std::vector<TreeSearchResult> entries = iter->second;

    tasks.push_back(pool.submit_task([entries, &scanner, &env, this, &resultInter]() -> ahn::vector<FileData>
    {
      ahn::vector<FileData> ret;
      ret.reserve(entries.size());

      for (const TreeSearchResult& entry : entries)
      {
        if (resultInter->isAborted())
          return ret;

        FileData result;

        bool found = false;

        // result.id = id;
        result.entry = entry;

        // Sanitize sourceDb path for use as a cache folder name
        std::string sanitizedSourceDb = DataHelperC::cleanFileName(entry.sourceDb);
        std::string sanitizedRevision = entry.sourceRevision.empty() ? "default" : entry.sourceRevision;
        std::filesystem::path cachePath = env.dataFolder / sanitizedSourceDb / sanitizedRevision / std::to_string(entry.sourceInternalId);

        try
        {
          if (std::filesystem::exists(cachePath))
          {
            for (auto& filePath : std::filesystem::recursive_directory_iterator(cachePath))
            {
              if (filePath.is_regular_file())
              {
                std::filesystem::path dbPath = filePath.path();
                std::ifstream in(dbPath, std::ios::binary);
                result.cmpFile.assign((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
                result.sourceFile = std::filesystem::relative(dbPath, cachePath).string();
                found = true;
                break;
              }
            }
          }
        }
        catch (const std::filesystem::filesystem_error&)
        {
        }

        // if (!found)
        //{
        switch (entry.type)
        {
        case TreeSearchResult::Github:
        {
          if (!found)
          {
            std::pair<std::string, std::string> fileData = getGitHubFile(entry.sourceDb, entry.sourceInternalId, entry.sourceRevision, result.licence, env);
            result.sourceFile = fileData.first;
            result.cmpFile = fileData.second;
          }

          result.sourceFileVis = result.sourceFile;
          result.dbTree = scanner.scanDeep(result.sourceFile, result.cmpFile, result.resSize, result.sourceType);
        }
        break;
        case TreeSearchResult::SourceForge:
          if (!found)
          {
            result.cmpFile = getSourceForgeFile(entry.sourceDb, entry.sourceInternalId, entry.sourceRevision, result.licence);
            result.sourceFile = std::to_string(entry.sourceInternalId);
          }
          result.sourceFileVis = result.sourceFile;
          result.dbTree = scanner.scanDeep(result.cmpFile, result.resSize, result.sourceType);
          break;
        case TreeSearchResult::Stackexchange:
          if (!found)
          {
            result.cmpFile = getStackexchangeFile(entry.sourceDb, entry.sourceInternalId, result.licence);
            result.sourceFile = std::to_string(entry.sourceInternalId) + ".txt";
          }
          result.sourceFileVis = "https://stackoverflow.com/questions/" + std::to_string(entry.sourceInternalId);
          result.dbTree = scanner.scanDeep(result.cmpFile, result.resSize, result.sourceType);
          break;
        }

        try
        {
          std::filesystem::path cacheFilePath = cachePath / result.sourceFile;
          std::filesystem::create_directories(cacheFilePath.parent_path());
          std::ofstream out(cacheFilePath, std::ios::binary);
          out << result.cmpFile;
        }
        catch (const std::filesystem::filesystem_error&)
        {
        }
        //}

        ret.push_back(result);
      }
      return ret;
    }));
  }

  struct DeepScanData
  {
    std::string content;
    SourceStructureTreeDeep* tree;
  };

  std::unordered_map<std::string, DeepScanData> trees;
  for (int index = 0; index < tasks.size(); index++)
  {
    ahn::vector<FileData> datas = tasks[index].get();

    for (const FileData& data : datas)
    {
      if (resultInter->isAborted())
        return;

      if (data.dbTree == nullptr)
      {
        resultInter->incDeepFinishedCount(1);
        continue;
      }

      auto iter = trees.find(data.entry.searchFile);
      if (iter == trees.end())
      {
        uint32_t resSize;
        std::string sourceType;

        std::ifstream in(data.entry.searchFile, std::ios::binary);
        if (!in.is_open())
        {
          resultInter->incDeepFinishedCount(1);
          continue;
        }

        std::ostringstream ss;
        ss << in.rdbuf();
        std::string content = ss.str();

        std::filesystem::path path = data.entry.searchFile;
        SourceStructureTreeDeep* tree = scanner.scanDeep(path, resSize, sourceType);

        if (tree == nullptr)
        {
          resultInter->incDeepFinishedCount(1);
          continue;
        }

        trees[data.entry.searchFile] = {content, tree};
      }

      SearchNodes dbNodes = initNodesDeep(data.dbTree, env.windowSize);
      SearchNodes searchNodes = initNodesDeep(trees[data.entry.searchFile].tree, env.windowSize);

      TreeSearchResult deepResult = searchHash(dbNodes, searchNodes, env.windowSize);
      if (deepResult)
      {
        // Stich the resultSets back together;
        TreeSearchResult stiched;

        for (int outerIndex = 0; outerIndex < deepResult.size(); outerIndex++)
        {
          TreeSearchResultSet outerSetCpy = deepResult.at(outerIndex);
          stiched.push_back(outerSetCpy);

          for (int innerIndex = outerIndex + 1; innerIndex < deepResult.size(); innerIndex++)
          {
            const TreeSearchResultSet innerSet = deepResult.at(innerIndex);

            if (outerSetCpy.intersectsOrTouches(innerSet))
            {
              outerSetCpy.merge(innerSet);
            }
            else
            {
              outerIndex = innerIndex - 1;
              break;
            }
          }
        }

        stiched.sourceDb = data.entry.sourceDb;
        stiched.sourceFile = data.sourceFile;
        stiched.sourceRevision = data.entry.sourceRevision;
        stiched.sourceInternalId = deepResult.sourceInternalId;
        stiched.searchFile = std::filesystem::relative(data.entry.searchFile, path).string();
        stiched.type = deepResult.type;
        stiched.sourceContent = data.cmpFile;
        stiched.searchContent = trees[data.entry.searchFile].content;
        stiched.licence = data.licence;

        resultInter->addDeepResult(stiched);
      }

      resultInter->incDeepFinishedCount(1);

      delete data.dbTree;
    }
  }

  for (auto iter = trees.begin(); iter != trees.end(); iter++)
    delete iter->second.tree;
}

std::pair<std::string, std::string> TreeSearch::getGitHubFile(const std::string& sourceDb, const uint32_t& fileId, const std::string& sha, std::string& licence,
                                                              const EnviromentC& env)
{
  std::string ret;

  FileDatabase fileDb(DBType::SQLite, sourceDb);

  std::string fileName = fileDb.getName(fileId);
  std::string repoUrl = fileDb.getRepoUrl();

  std::unordered_map<std::string, std::string> result;
  if (repoUrl.find("github") != std::string::npos)
  {
    auto downloadFromGithub = [](const std::string& repoUrl, const std::string& sha, const std::string& relPath, std::string& outputFile)
    {
      // Extract owner and repo using regex
      std::regex re(R"(https://github\.com/([^/]+)/([^/]+))");
      std::smatch match;

      if (!std::regex_search(repoUrl, match, re))
        return false;

      std::string owner = match[1];
      std::string repo = match[2];

      // Build raw.githubusercontent.com path
      std::string host = "raw.githubusercontent.com";
      std::string path = "/" + owner + "/" + repo + "/" + sha + "/" + relPath;

      httplib::SSLClient cli(host);
      cli.set_follow_location(true);

      auto res = cli.Get(path.c_str());
      if (!res)
        return false;

      if (res->status != 200)
        return false;

      outputFile = res->body;

      return true;
    };

    downloadFromGithub(repoUrl, sha, fileName, result[fileName]);
  }
  else
  {
    std::string workRepoName = DataHelperC::cleanFileName(DataHelperC::extractOwnerRepo(repoUrl));

    std::filesystem::path repoPath = std::filesystem::path(env.workFolder) / workRepoName;
    std::filesystem::path workPath = repoPath / "work";

    std::filesystem::create_directories(repoPath);
    std::filesystem::create_directories(workPath);

    GitCliHelperC::getGitClone(repoPath, repoUrl, env.workFolder.string());

    GitCliHelperC::fetchTag(repoPath.string(), sha, env.workFolder.string());

    std::vector<std::string> files;
    files.push_back(fileName);
    result = GitCliHelperC::getFilesWithContent(repoPath.string(), sha, files);

    while (std::filesystem::exists(repoPath))
    {
      std::error_code ec;
      std::filesystem::remove_all(repoPath, ec);
    }
  }
  return std::make_pair(fileName, result[fileName]);
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
