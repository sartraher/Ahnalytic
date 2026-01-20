#include "AhnalyticBase/tree/SourceScanner.hpp"
#include "AhnalyticBase/compression/CompressionManager.hpp"
#include "AhnalyticBase/database/FileDatabase.hpp"
#include "AhnalyticBase/helper/Diagnostic.hpp"

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

void SourceScanner::traverseDeep(TSTreeCursor& cursor, SourceStructureTreeDeep* parent, SourceHandlerI* handler, const std::string& content, size_t depth) const
{
  if (depth > 1000)
    return;

  auto nodeText = [](TSNode node, const char* source)
  {
    uint32_t start = ts_node_start_byte(node);
    uint32_t end = ts_node_end_byte(node);
    return std::string(source + start, end - start);
  };

  TSNode node = ts_tree_cursor_current_node(&cursor);

  uint16_t symbolId = ts_node_symbol(node);
  uint16_t fieldId = ts_tree_cursor_current_field_id(&cursor);

  std::string type = ts_node_type(node);

  std::string name;

  // TODO: move this into handler as soon as there is more then one lang
  if (type == "string_literal")
    name = nodeText(node, content.c_str());
  else if (type == "number_literal")
    name = nodeText(node, content.c_str());
  else if (type == "primitive_type" || type == "type_identifier")
    name = nodeText(node, content.c_str());
  else if (type == "class_specifier")
  {
    if (TSNode nameNode = ts_node_child_by_field_name(node, "name", 4); !ts_node_is_null(nameNode))
      name = nodeText(nameNode, content.c_str());
  }
  else if (type == "function_definition")
  {
    if (TSNode declarator = ts_node_child_by_field_name(node, "declarator", 10); !ts_node_is_null(declarator))
      if (TSNode inner = ts_node_child_by_field_name(declarator, "declarator", 10); !ts_node_is_null(inner))
        name = nodeText(inner, content.c_str());
  }
  else if (type == "call_expression")
  {
    TSNode functionNode = ts_node_child_by_field_name(node, "function", 8);
    if (!ts_node_is_null(functionNode))
    {
      std::string type = ts_node_type(functionNode);

      if (type == "identifier" || type == "qualified_identifier")
        name = nodeText(functionNode, content.c_str());
      else if (type == "field_expression")
      {
        TSNode field = ts_node_child_by_field_name(functionNode, "field", 5);
        name = nodeText(field, content.c_str());
      }
      else
        name = nodeText(functionNode, content.c_str());
    }
  }

  TSPoint start = ts_node_start_point(node);
  unsigned lineNr = start.row + 1;

  SourceStructureTreeDeep* child = nullptr;
  handler->filter(symbolId, fieldId);

  child = new SourceStructureTreeDeep();
  child->data.id.data.symboldId = symbolId;
  child->data.id.data.fieldId = fieldId;
  child->data.name = name;
  child->parent = parent;
  child->data.lineNr = lineNr;
  parent->children.push_back(child);

  if (ts_tree_cursor_goto_first_child(&cursor))
  {
    do
    {
      traverseDeep(cursor, child, handler, content, depth + 1);
    } while (ts_tree_cursor_goto_next_sibling(&cursor));
    ts_tree_cursor_goto_parent(&cursor);
  }
}

SourceStructureTreeDeep* SourceScanner::scanDeep(const std::filesystem::path& path, uint32_t& resSize, std::string& sourceType) const
{
  auto handlerIter = handlers.find(path.extension().string());
  if (handlerIter == handlers.end())
    return nullptr;

  sourceType = handlerIter->second->getId();

  SourceStructureTreeDeep* ret = new SourceStructureTreeDeep();

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
    SourceStructureTreeDeep* cur = ret;

    TSTreeCursor cursor = ts_tree_cursor_new(root_node);
    traverseDeep(cursor, cur, handlerIter->second, content);
    ts_tree_cursor_delete(&cursor);
  }

  ts_tree_delete(tree);
  ts_parser_delete(parser);

  resSize = (uint32_t)size;

  return ret;
}

SourceStructureTreeDeep* SourceScanner::scanDeep(const std::string& content, uint32_t& resSize, std::string& sourceType) const
{
  SourceStructureTreeDeep* ret = nullptr;

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
    ret = new SourceStructureTreeDeep();

    TSTreeCursor cursor = ts_tree_cursor_new(root_node);
    traverseDeep(cursor, ret, curHandler, content);
    ts_tree_cursor_delete(&cursor);

    ts_tree_delete(lastTree);

    sourceType = curHandler->getId();
    resSize = (uint32_t)content.size();
  }

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