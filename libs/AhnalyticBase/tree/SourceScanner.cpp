#include "AhnalyticBase/tree/SourceScanner.hpp"
#include "AhnalyticBase/compression/CompressionManager.hpp"
#include "AhnalyticBase/helper/Diagnostic.hpp"
#include "AhnalyticBase/database/FileDatabase.hpp"

#include <fstream>
#include <set>
#include <stack>

#include <tree_sitter/api.h>

#include "BS_thread_pool.hpp"

#include <chrono>

typedef struct TSLanguage TSLanguage;

#ifdef __cplusplus
extern "C"
{
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
    // symboldId = -1;
    // fieldId = -1;
  }

  std::string getId() const
  {
    return "CPP";
  }
};

SourceScanner::SourceScanner()
{
  CppSourceHandler* cppHandler = new CppSourceHandler();

  handlerList.push_back(cppHandler);
  handlers[".cpp"] = cppHandler;
  handlers[".hpp"] = cppHandler;
  handlers[".c"] = cppHandler;
  handlers[".h"] = cppHandler;
  handlers[".cc"] = cppHandler;
  handlers[".hh"] = cppHandler;
  handlers[".cxx"] = cppHandler;
  handlers[".hxx"] = cppHandler;
}

SourceScanner::~SourceScanner()
{
}

void SourceScanner::traverse(TSTreeCursor& cursor, SourceStructureTree* parent, SourceHandlerI* handler, size_t depth) const
{
  if (depth > 1000)
    return;

  TSNode node = ts_tree_cursor_current_node(&cursor);

  uint16_t symbolId = ts_node_symbol(node);
  uint16_t fieldId = ts_tree_cursor_current_field_id(&cursor);

  TSPoint start = ts_node_start_point(node);
  unsigned lineNr = start.row + 1;

  SourceStructureTree* child = nullptr;
  handler->filter(symbolId, fieldId);

  child = new SourceStructureTree();
  child->data.id.data.symboldId = symbolId;
  child->data.id.data.fieldId = fieldId;
  child->parent = parent;
  child->data.lineNr = lineNr;
  parent->children.push_back(child);

  if (ts_tree_cursor_goto_first_child(&cursor))
  {
    do
    {
      traverse(cursor, child, handler, depth + 1);
    } while (ts_tree_cursor_goto_next_sibling(&cursor));
    ts_tree_cursor_goto_parent(&cursor);
  }
}

SourceStructureTree* SourceScanner::scan(const std::filesystem::path& path, const std::string& content, uint32_t& resSize, std::string& sourceType) const
{
  auto handlerIter = handlers.find(path.extension().string());
  if (handlerIter == handlers.end())
    return nullptr;

  sourceType = handlerIter->second->getId();

  SourceStructureTree* ret = new SourceStructureTree();

  TSParser* parser = ts_parser_new();
  ts_parser_set_language(parser, handlerIter->second->getLanguage());

  uint64_t timeout = 60000000;
  ts_parser_set_timeout_micros(parser, timeout); // 1m

  auto start = std::chrono::steady_clock::now();
  TSTree* tree = ts_parser_parse_string(parser, NULL, content.c_str(), (unsigned int)content.size());
  auto end = std::chrono::steady_clock::now();

  auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
  if (elapsed >= timeout)
  {
    if (tree != nullptr)
      ts_tree_delete(tree);

    ts_parser_delete(parser);

    return nullptr;
  }

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

  resSize = (uint32_t)content.size();

  return ret;
}

SourceStructureTree* SourceScanner::scan(const std::filesystem::path& path, uint32_t& resSize, std::string& sourceType) const
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

  TSTree* tree = ts_parser_parse_string(parser, NULL, content.c_str(), (unsigned int)content.size());

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

  resSize = (uint32_t)size;

  return ret;
}

int SourceScanner::countErrorNodes(const TSTree* tree) const
{
  int error_count = 0;

  // Get the root node of the tree
  TSNode root = ts_tree_root_node(tree);

  // Traverse the tree and count error nodes
  int child_count = ts_node_child_count(root);
  for (int i = 0; i < child_count; i++)
  {
    TSNode node = ts_node_child(root, i);
    if (ts_node_has_error(node))
    {
      error_count++;
    }
  }

  return error_count;
}

SourceStructureTree* SourceScanner::scan(const std::string& content, uint32_t& resSize, std::string& sourceType) const
{
  SourceStructureTree* ret = nullptr;

  TSTree* lastTree = nullptr;
  SourceHandlerI* curHandler = nullptr;

  for (SourceHandlerI* handler : handlerList)
  {
    TSParser* parser = ts_parser_new();
    ts_parser_set_language(parser, handler->getLanguage());

    TSTree* tree = ts_parser_parse_string(parser, NULL, content.c_str(), (unsigned int)content.size());

    if (tree != nullptr)
    {
      int errors = countErrorNodes(tree);

      if (errors == 0)
      {
        if (lastTree)
          ts_tree_delete(lastTree);

        lastTree = tree;
        curHandler = handler;
        break;
      }
      else
        ts_tree_delete(tree);
    }

    ts_parser_delete(parser);
  }

  if (lastTree)
  {
    TSNode root_node = ts_tree_root_node(lastTree);
    ret = new SourceStructureTree();

    TSTreeCursor cursor = ts_tree_cursor_new(root_node);
    traverse(cursor, ret, curHandler);
    ts_tree_cursor_delete(&cursor);

    ts_tree_delete(lastTree);

    sourceType = curHandler->getId();
    resSize = (uint32_t)content.size();
  }

  return ret;
}

void SourceScanner::printTree(SourceStructureTree* node, const std::string& prefix)
{
  if (!node)
    return;

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
/*
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

  FileDatabase db(DBType::SQLite, "C:/temp/ahnalytic.db");
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
*/

std::unordered_map<std::string, std::vector<ScanTreeData>> SourceScanner::scanPath(const std::filesystem::path& path)
{
  std::unordered_map<std::string, std::vector<ScanTreeData>> ret;

  for (auto& filePath : std::filesystem::recursive_directory_iterator(path))
  {
    uint32_t resSize;
    std::string sourceType;
    SourceStructureTree* tree = scan(filePath, resSize, sourceType);
    if (tree != nullptr)
      ret[sourceType].push_back({filePath, tree, resSize});
  }

  return ret;
}

std::unordered_map<std::string, std::vector<ScanTreeData>> SourceScanner::scanBuffer(std::unordered_map<std::string, std::string> buffers)
{
  struct ResData
  {
    std::string path;
    uint32_t resSize = 0;
    std::string sourceType;
    SourceStructureTree* tree = nullptr;
  };

  std::unordered_map<std::string, std::vector<ScanTreeData>> ret;

  BS::thread_pool pool(2);

  std::list<std::future<ResData>> tasks;

  for (auto buffer : buffers)
  {
    std::future<ResData> resultFuture = pool.submit_task([buffer, this]()
    {
      ResData ret;
      ret.path = buffer.first;
      ret.tree = scan(buffer.first, buffer.second, ret.resSize, ret.sourceType);
      return ret;
    });

    tasks.push_back(std::move(resultFuture));
  }

  for (std::future<ResData>& task : tasks)
  {
    ResData result = task.get();

    if (result.tree != nullptr)
      ret[result.sourceType].push_back({result.path, result.tree, result.resSize});
  }

  return ret;
}

std::list<std::string> SourceScanner::getFileTypes()
{
  std::list<std::string> ret;
  for (auto ext : handlers)
    ret.push_back(ext.first);
  return ret;
}

std::list<std::string> SourceScanner::getFileGroups()
{
  std::list<std::string> ret;
  for (auto ext : handlerList)
    ret.push_back(ext->getId());
  return ret;
}