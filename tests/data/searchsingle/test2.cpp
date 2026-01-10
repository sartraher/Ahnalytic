#include "SourceScanner.hpp"
#include "Database.hpp"
#include "AhnalyticBase/diagnostic.hpp"
#include "AhnalyticBase/compressionManager.hpp"

#include <fstream>
#include <stack>
#include <set>

#include <tree_sitter/api.h>

#include "BS_thread_pool.hpp"

typedef struct TSLanguage TSLanguage;

#ifdef __cplusplus
extern "C" {
#endif

  const TSLanguage* tree_sitter_cpp(void);

#ifdef __cplusplus
}
#endif

class CppSourceHandler : public SourceHandlerI
{
public:
  const TSLanguage* getLanguage() override
  {
    return tree_sitter_cpp();
  }

  void filter(uint16_t& symboldId, uint16_t& fieldId) override
  {
    //symboldId = -1;
    //fieldId = -1;
  }

  std::string getId() const
  {
    return "CPP";
  }
};

SourceScanner::SourceScanner()
{
  CppSourceHandler* cppHandler = new CppSourceHandler();

  handlers[".cpp"] = cppHandler;
  handlers[".hpp"] = cppHandler;
  handlers[".c"] = cppHandler;
  handlers[".h"] = cppHandler;
  handlers[".cxx"] = cppHandler;
  handlers[".hxx"] = cppHandler;
}

SourceScanner::~SourceScanner()
{
}

void SourceScanner::traverse(TSTreeCursor& cursor, SourceStructureTree* parent, SourceHandlerI* handler)
{
  TSNode node = ts_tree_cursor_current_node(&cursor);

  uint16_t symbolId = ts_node_symbol(node);
  uint16_t fieldId = ts_tree_cursor_current_field_id(&cursor);

  SourceStructureTree* child = parent;
  handler->filter(symbolId, fieldId);

  child = new SourceStructureTree();
  child->data.id.data.symboldId = symbolId;
  child->data.id.data.fieldId = fieldId;
  child->parent = parent;
  parent->children.push_back(child);

  if (ts_tree_cursor_goto_first_child(&cursor)) {
    do {
      traverse(cursor, child, handler);
    } while (ts_tree_cursor_goto_next_sibling(&cursor));
    ts_tree_cursor_goto_parent(&cursor);
  }
}

void SourceStructureTree::deserialize(const std::vector<char>& data, std::vector<FlatNodeDeDupData>& nodeList, std::vector<uint32_t>& indexList, Diagnostic* dia)
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
    nodeList[i].data.id.data.symboldId = symbolList[i];
    nodeList[i].data.id.data.fieldId = fieldList[i];
    nodeList[i].amount = static_cast<int>(amountList[i]);  // Cast back from uint32_t
  }
}

SourceStructureTree* SourceScanner::scan(const std::filesystem::path& path, uint32_t& resSize, std::string& sourceType)
{
  auto handlerIter = handlers.find(path.extension().string());
  if (handlerIter == handlers.end())
    return nullptr;

  sourceType = handlerIter->second->getId();

  SourceStructureTree* ret = new SourceStructureTree();

  TSParser* parser = ts_parser_new();
  ts_parser_set_language(parser, handlerIter->second->getLanguage());

  auto size = std::filesystem::file_size(path);
  std::string content(size, '\0');
  std::ifstream in(path);
  in.read(&content[0], size);

  TSTree* tree = ts_parser_parse_string(
    parser,
    NULL,
    content.c_str(),
    (unsigned int)content.size()
  );

  if (tree != nullptr)
  {
    TSNode root_node = ts_tree_root_node(tree);
    SourceStructureTree* cur = ret;

    TSTreeCursor cursor = ts_tree_cursor_new(root_node);
    traverse(cursor, cur, handlerIter->second);
    ts_tree_cursor_delete(&cursor);
  }

  ts_tree_delete(tree);
  ts_parser_delete(parser);

  resSize = size;

  return ret;
}

void SourceScanner::printTree(SourceStructureTree* node, const std::string& prefix)
{
  if (!node) return;

  // Print current node in [symboldId, fieldId] format
  std::cout << prefix;
  std::cout << "[" << node->data.id.data.symboldId << ", " << node->data.id.data.fieldId << "]" << std::endl;

  // Recursively print children with appropriate prefixes for tree structure
  for (size_t i = 0; i < node->children.size(); ++i)
  {
    const std::string newPrefix = prefix + (i == node->children.size() - 1 ? "   " : "|--");
    printTree((SourceStructureTree*)node->children[i], newPrefix);
  }
}

std::list<std::vector<char>> SourceScanner::scanFolder(const std::filesystem::path& path)
{
  std::vector<std::string> types{ "Source", "Image", "Text", "Other" };
  std::vector<std::string> sourceTypes;

  for (auto& p : handlers)
    sourceTypes.push_back(p.second->getId());

  SourceScanner scanner;

  std::unordered_map<std::string, std::vector<std::filesystem::path>> fileMap;
  std::vector<std::filesystem::path> files;
  for (auto& p : std::filesystem::recursive_directory_iterator(path))
    fileMap[p.path().extension().string()].push_back(p.path());

  //std::list<SourceStructureTree*> nodesCollected;
  std::vector<std::filesystem::path> nodesCollected;
  uint32_t sizeSum = 0;

  Database db(DBType::SQLite, "C:/temp/ahnalytic.db");
  uint32_t repoId = db.getOrCreateRepository("qt", "LGPL2,GPL2", "0", "Local", path);

  //BS::thread_pool pool(1);
  BS::thread_pool pool;

  auto processTrees = [&nodesCollected, &sizeSum, &pool, &db, path, repoId, this]()
    {
      std::future<std::unordered_map<uint32_t, std::pair<std::string, std::vector<std::filesystem::path>>>> resultFuture = pool.submit_task(
        [nodesCollected, sizeSum, &db, path, repoId, this]
        {
          std::unordered_map<uint32_t, std::pair<std::string, std::vector<std::filesystem::path>>> ret;

          std::list<SourceStructureTree*> nodesCollectedRes;

          std::string sourceType;
          uint32_t sizeSumRes = 0;
          for (const std::filesystem::path& filePath : nodesCollected)
          {
            uint32_t resSize;
            SourceStructureTree* tree = scan(filePath, resSize, sourceType);

            if (tree == nullptr)
              continue;

            //printTree(tree);

            nodesCollectedRes.push_back(tree);
            sizeSumRes += resSize;
          }

          Diagnostic dia(sizeSumRes);


          std::vector<char> result;

          SourceStructureTree* root = new SourceStructureTree();
          root->children.reserve(nodesCollectedRes.size());

          for (auto& node : nodesCollectedRes)
            root->children.push_back(node);

          std::vector<FlatNodeDeDupData> deduped;
          std::vector<uint32_t> indexList;
          reduceTree(root, deduped, indexList);

          dia.setDupReduce(deduped.size() * sizeof(FlatNodeDeDupData) + indexList.size() * sizeof(uint32_t));

          SourceStructureTree::serialize(deduped, indexList, result, &dia);

          // Test
          std::vector<FlatNodeDeDupData> nodeListTestOut;
          std::vector<uint32_t> indexListTestOut;
          SourceStructureTree::deserialize(result, nodeListTestOut, indexListTestOut, &dia);
          
          auto restoredTree = rebuildTree(nodeListTestOut, indexListTestOut);
          bool isEq = ((*restoredTree) == (*root));
          

          /*
          auto flat = treeToFlat(root);


          std::vector<FlatNodeDeDupData> deduped;
          std::vector<uint32_t> indexList;
          flattenAndDedupFlatNodes(flat, deduped, indexList);

          dia.setDupReduce(deduped.size() * sizeof(FlatNodeDeDupData) + indexList.size() * sizeof(uint32_t));

          SourceStructureTree::serialize(deduped, indexList, result, &dia);

          // Test
          std::vector<FlatNodeDeDupData> nodeListTestOut;
          std::vector<uint32_t> indexListTestOut;
          SourceStructureTree::deserialize(result, nodeListTestOut, indexListTestOut, &dia);

          auto restoredFlat = reconstructFlatNodesFromDedup(nodeListTestOut, indexListTestOut, flat.size());
          SourceStructureTree* restoredTree = (SourceStructureTree*)rebuildTree(restoredFlat);

          bool isEq = ((*restoredTree) == (*root));
          */

          /*
          std::vector<SourceStructureTreeFlat> nodeList;
          std::vector<uint32_t> indexList;
          SourceStructureTree::reduceTree(nodesCollectedRes, nodeList, indexList);

          dia.setDupReduce(nodeList.size() * sizeof(SourceStructureTreeFlat) + indexList.size() * sizeof(uint32_t));

          SourceStructureTree::serialize(nodeList, indexList, result, &dia);

          std::vector<SourceStructureTreeFlat> nodeListTestOut;
          std::vector<uint32_t> indexListTestOut;
          SourceStructureTree::deserialize(result, nodeListTestOut, indexListTestOut, &dia);

          std::list<SourceStructureTree*> treesTest;
          SourceStructureTree::rebuildTree(nodeListTestOut, indexListTestOut, treesTest);
          */

          dia.write();

          for (SourceStructureTree* node : nodesCollectedRes)
            delete node;

          uint32_t dataID = db.createSourceTreeData(result);

          ret[dataID] = std::pair<std::string, std::vector<std::filesystem::path>>(sourceType, nodesCollected);

          return ret;
        });

      sizeSum = 0;
      nodesCollected.clear();

      return resultFuture;
    };

  constexpr uint32_t maxSize = 1024 * 1024;
  //constexpr uint32_t maxSize = 1;

  std::list<std::future<std::unordered_map<uint32_t, std::pair<std::string, std::vector<std::filesystem::path>>>>> datas;

  std::unordered_map< SourceHandlerI*, std::vector<std::filesystem::path>> fileByHandler;
  for (auto iter = fileMap.begin(); iter != fileMap.end(); iter++)
  {
    for (const auto& path : iter->second)
    {
      auto handlerIter = handlers.find(path.extension().string());
      if (handlerIter == handlers.end())
        continue;
      fileByHandler[handlerIter->second].push_back(path);
    }
  }

  for (auto iter = fileByHandler.begin(); iter != fileByHandler.end(); iter++)
  {
    for (const auto& path : iter->second)
    {
      uint32_t resSize = std::filesystem::file_size(path);

      nodesCollected.push_back(path);

      sizeSum += resSize;
      if (sizeSum >= maxSize)
      {
        datas.push_back(std::move(processTrees()));
      }
    }
    if (nodesCollected.size() > 0)
      datas.push_back(processTrees());
    nodesCollected.clear();
  }

  pool.wait();

  std::list<std::vector<char>> result;

  std::set<std::string> names;

  std::list<std::unordered_map<uint32_t, std::pair<std::string, std::vector<std::filesystem::path>>>> results;
  for (std::future<std::unordered_map<uint32_t, std::pair<std::string, std::vector<std::filesystem::path>>>>& futureData : datas)
    results.push_back(futureData.get());

  for (std::unordered_map<uint32_t, std::pair<std::string, std::vector<std::filesystem::path>>>& data : results)
  {
    for (auto iter = data.begin(); iter != data.end(); iter++)
    {
      for (int index = 0; index < iter->second.second.size(); index++)
      {
        std::filesystem::path filePath = iter->second.second.at(index);
        std::filesystem::path relPath = std::filesystem::relative(filePath, path);

        for (auto p : relPath)
          names.insert(p.string());
        //names.insert(relPath.begin(), relPath.end());
      }
    }
  }

  std::vector<std::string> vectorNames(names.begin(), names.end());
  std::unordered_map<std::string, uint32_t> existingNames = db.insertNames(vectorNames);
  std::unordered_map<std::string, uint32_t> existingTypes = db.insertTypes(types);
  std::unordered_map<std::string, uint32_t> existingSourceTypes = db.insertSourceTypes(sourceTypes);
  /*
    std::vector<std::string> missingNames;
    std::unordered_map<std::string, uint32_t> existingNames = db.getNames();

    for (const std::string& name : names)
      if (!existingNames.contains(name))
        missingNames.push_back(name);

    if (missingNames.size() > 0)
      existingNames = db.insertNames(missingNames);
      */
  std::vector< std::vector<uint32_t>> idPathes;
  for (std::unordered_map<uint32_t, std::pair<std::string, std::vector<std::filesystem::path>>>& data : results)
  {
    for (auto iter = data.begin(); iter != data.end(); iter++)
    {
      for (int index = 0; index < iter->second.second.size(); index++)
      {
        std::filesystem::path filePath = iter->second.second.at(index);
        std::filesystem::path relPath = std::filesystem::relative(filePath, path);

        std::vector<std::string> segments;
        for (auto p : relPath)
          segments.push_back(p.string());

        std::vector<uint32_t> idPath;
        for (const std::string segment : segments)
          idPath.push_back(existingNames[segment]);
        idPathes.push_back(idPath);
      }
    }
  }

  std::unordered_map<std::vector<uint32_t>, uint32_t, VectorHash, VectorEqual> pathIds = db.insertPathes(idPathes);

  std::vector<std::string> revisions{ "" };
  std::unordered_map<std::string, uint32_t> existingRevisions = db.insertRevisions(revisions);

  for (std::unordered_map<uint32_t, std::pair<std::string, std::vector<std::filesystem::path>>>& data : results)
  {
    for (auto iter = data.begin(); iter != data.end(); iter++)
    {
      for (int index = 0; index < iter->second.second.size(); index++)
      {
        std::filesystem::path filePath = iter->second.second.at(index);
        std::filesystem::path relPath = std::filesystem::relative(filePath, path);

        std::vector<std::string> segments;
        for (auto p : relPath)
          segments.push_back(p.string());

        std::vector<uint32_t> idPath;
        for (const std::string segment : segments)
          idPath.push_back(existingNames[segment]);

        db.createFile(pathIds[idPath], existingTypes["Source"], existingSourceTypes[iter->second.first], repoId, existingRevisions[""], iter->first, index);
      }
    }

  }

  return result;
}