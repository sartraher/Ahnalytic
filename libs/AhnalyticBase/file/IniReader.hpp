#ifndef inireader_hpp__
#define inireader_hpp__

#include "AhnalyticBase/Export.hpp"

#include <algorithm>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

class DLLEXPORT IniReader
{
private:
  enum class LineTypeE
  {
    Unknown,
    Segment,
    Item
  };

  std::map<std::string, std::map<std::string, std::string>> data;

  static std::string trim(const std::string& s);

public:
  IniReader(const std::string& fileName);

  std::string getValue(const std::string& name, const std::string& block, const std::string& def) const;
  std::vector<std::string> getBlocks() const;
  std::vector<std::string> getItems(const std::string& block) const;
};

#endif