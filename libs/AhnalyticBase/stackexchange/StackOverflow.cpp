#include "StackOverflow.hpp"

#include "AhnalyticBase/database/SnippedDatabase.hpp"
#include "AhnalyticBase/tree/SourceScanner.hpp"
#include "AhnalyticBase/database/StackExchangeExtractDatabase.hpp"
#include "AhnalyticBase/stackexchange/DataDump.hpp"

#include "BS_thread_pool.hpp"

#include <fstream>

StackOverflowHandler::StackOverflowHandler()
{
}

void StackOverflowHandler::convertDataDump(const std::string& commentsXmlPath, const std::string& dbPath)
{
  DataDump dataDump;
  dataDump.parseXMLFile(commentsXmlPath.c_str());

  StackExchangeExtractDatabase db(DBType::SQLite, dbPath);

  while (dataDump.hasNext())
  {
    SnippedData snippedData = dataDump.next();
    db.addSnipped(snippedData.id, snippedData.date, snippedData.licence, snippedData.code);
  }
}

void StackOverflowHandler::importData(const std::string& stackDb, const std::string& outDb)
{
  StackExchangeExtractDatabase db(DBType::SQLite, stackDb);
  // SnippedDatabase snippedDB(DBType::SQLite, outDb);

  struct TreeData
  {
    SourceStructureTree* tree;
    uint32_t size;
    SnippedData snipped;
  };

  std::unordered_map<std::string, std::list<TreeData>> dataCollector;
  std::unordered_map<std::string, uint32_t> sizes;
  std::unordered_map<std::string, SnippedDatabase*> dbs;

  std::unordered_map<std::string, uint32_t> licenceLookup;

  BS::thread_pool pool;
  std::recursive_mutex mutex;

  auto processData = [&dbs, &mutex, &licenceLookup, outDb](std::string sourceType, std::unordered_map<std::string, std::list<TreeData>> dataCollector)
  {
    if (!dbs.contains(sourceType))
      dbs[sourceType] = new SnippedDatabase(DBType::SQLite, outDb + "_" + sourceType + ".db");

    SourceStructureTree* root = new SourceStructureTree();
    for (const TreeData& data : dataCollector[sourceType])
      root->children.push_back(data.tree);

    std::vector<FlatNodeDeDupData> deduped;
    std::vector<uint32_t> indexList;
    reduceTree(root, deduped, indexList);

    std::vector<char> result;
    SourceStructureTree::serialize(deduped, indexList, result, nullptr);

    /*
    std::vector<FlatNodeDeDupData> nodeListTestOut;
    std::vector<uint32_t> indexListTestOut;
    SourceStructureTree::deserialize(result, nodeListTestOut, indexListTestOut, nullptr);
    root = (SourceStructureTree*)rebuildTree(nodeListTestOut, indexListTestOut);
    */

    // std::ofstream out("D:/source/Ahnalytic/db/stackoverflow/output.bin", std::ios::binary);
    // out.write(result.data(), result.size());

    SnippedDatabase* db = dbs[sourceType];

    uint32_t dataId = db->createSourceTreeData(result);
    uint32_t index = 0;

    // std::vector<char> resultTest;
    // db->getSourceTreeData(dataId, resultTest);
    // int bla = memcmp(result.data(), resultTest.data(), result.size());

    for (const TreeData& treeData : dataCollector[sourceType])
    {
      uint32_t licId = 0;

      mutex.lock();
      auto licIter = licenceLookup.find(treeData.snipped.licence);
      if (licIter == licenceLookup.end())
      {
        licId = db->insertLicence(treeData.snipped.licence);
        licenceLookup[treeData.snipped.licence] = licId;
      }
      else
        licId = licIter->second;
      mutex.unlock();

      db->createSnipped(std::stoi(treeData.snipped.id), licId, dataId, index);
      index++;
    }

    dataCollector[sourceType].clear();
    delete root;
  };

  uint32_t counter = 0;
  SourceScanner scanner;
  db.processSnippeds([&scanner, &dataCollector, &sizes, &processData, &pool, &counter](const SnippedData& data)
  {
    uint32_t resSize;
    std::string sourceType;
    SourceStructureTree* tree = scanner.scan(data.code, resSize, sourceType);

    if (tree != nullptr)
    {
      dataCollector[sourceType].push_back({tree, resSize, data});
      if (sizes.contains(sourceType))
        sizes[sourceType] += resSize;
      else
        sizes[sourceType] = resSize;

      if (sizes[sourceType] > 1024 * 1024 * 10)
      {
        pool.detach_task([&processData, sourceType, dataCollector] { processData(sourceType, dataCollector); });
        sizes[sourceType] = 0;
        dataCollector.clear();

        if (counter++ == 100)
        {
          pool.wait();
          counter = 0;
        }
      }
    }
  });

  for (auto iter = dataCollector.begin(); iter != dataCollector.end(); iter++)
    if (iter->second.size() > 0)
    {
      std::string sourceType = iter->first;
      pool.detach_task([&processData, sourceType, dataCollector] { processData(sourceType, dataCollector); });
      sizes[sourceType] = 0;
    }

  pool.wait();
}