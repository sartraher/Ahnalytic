#ifndef sourcescanner_hpp__
#define sourcescanner_hpp__

#include "AhnalyticBase/tree/SourceStructureTree.hpp"

#include <filesystem>
#include <memory>
#include <unordered_map>
#include <list>

struct TSTreeCursor;
struct TSLanguage;
struct TSTree;

struct ScanTreeData
{
  std::filesystem::path path;
  SourceStructureTree* tree = nullptr;
  uint32_t resSize;
};

class SourceHandlerI
{
public:
  virtual ~SourceHandlerI()
  {
  }

  virtual const TSLanguage* getLanguage() = 0;
  virtual void filter(uint16_t& symboldId, uint16_t& fieldId) = 0;
  virtual std::string getId() const = 0;
};

constexpr uint32_t maxSize = 1024 * 1024;

class DLLEXPORT SourceScanner
{
public:
  SourceScanner();
  ~SourceScanner();
  SourceStructureTree* scan(const std::filesystem::path& path, uint32_t& resSize, std::string& sourceType) const;
  SourceStructureTree* scan(const std::string& content, uint32_t& resSize, std::string& sourceType) const;
  SourceStructureTree* scan(const std::filesystem::path& path, const std::string& content, uint32_t& resSize, std::string& sourceType) const;
  void traverse(TSTreeCursor& cursor, SourceStructureTree* parent, SourceHandlerI* handler, size_t depth = 0) const;

  SourceStructureTreeDeep* scanDeep(const std::filesystem::path& path, uint32_t& resSize, std::string& sourceType) const;
  SourceStructureTreeDeep* scanDeep(const std::string& content, uint32_t& resSize, std::string& sourceType) const;
  void traverseDeep(TSTreeCursor& cursor, SourceStructureTreeDeep* parent, SourceHandlerI* handler, const std::string& content, size_t depth = 0) const;

  int countErrorNodes(const TSTree* tree) const;
  void printTree(SourceStructureTree* node, const std::string& prefix = "");
  // std::list<std::vector<char>> scanFolder(const std::filesystem::path& path);

  std::unordered_map<std::string, std::vector<ScanTreeData>> scanPath(const std::filesystem::path& path);
  std::unordered_map<std::string, std::vector<ScanTreeData>> scanBuffer(std::unordered_map<std::string, std::string> buffers);
  std::list<std::string> getFileTypes();
  std::list<std::string> getFileGroups();

private:
  std::vector<SourceHandlerI*> handlerList;
  std::unordered_map<std::string, SourceHandlerI*> handlers;

protected:
};

#endif