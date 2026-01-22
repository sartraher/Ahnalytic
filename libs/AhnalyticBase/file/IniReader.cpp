#include "IniReader.hpp"

IniReader::IniReader(const std::string& fileName)
{
  std::ifstream file(fileName);
  if (!file.is_open())
    return;

  std::string segment;
  std::string line;

  while (std::getline(file, line))
  {
    LineTypeE curLineType = LineTypeE::Unknown;
    std::string curName;
    std::string curValue;
    size_t start = 0;

    for (size_t pos = 0; pos < line.length(); ++pos)
    {
      char val = line[pos];

      switch (val)
      {
      case '[':
        if (curLineType == LineTypeE::Unknown)
        {
          curLineType = LineTypeE::Segment;
          start = pos;
        }
        break;

      case ']':
        if (curLineType == LineTypeE::Segment)
        {
          segment = line.substr(start + 1, pos - start - 1);
        }
        break;

      case '=':
        if (curLineType == LineTypeE::Unknown)
        {
          curLineType = LineTypeE::Item;
          curName = trim(line.substr(0, pos));
          start = pos;
        }
        break;

      case ';':
      case '#':
        pos = line.length();
        continue;
      }

      if (curLineType == LineTypeE::Item)
      {
        curValue = line.substr(start + 1);

        data[segment][curName] = curValue;
      }
    }
  }
}

std::string IniReader::getValue(const std::string& name, const std::string& block, const std::string& def) const
{
  auto blkIt = data.find(block);
  if (blkIt != data.end())
  {
    auto itemIt = blkIt->second.find(name);
    if (itemIt != blkIt->second.end())
      return itemIt->second;
  }
  return def;
}

std::vector<std::string> IniReader::getBlocks() const
{
  std::vector<std::string> blocks;
  for (const auto& kv : data)
    blocks.push_back(kv.first);
  return blocks;
}

std::vector<std::string> IniReader::getItems(const std::string& block) const
{
  std::vector<std::string> items;
  auto it = data.find(block);
  if (it != data.end())
  {
    for (const auto& kv : it->second)
      items.push_back(kv.first);
  }
  return items;
}

std::string IniReader::trim(const std::string& s)
{
  auto start = s.find_first_not_of(" \t\r\n");
  auto end = s.find_last_not_of(" \t\r\n");
  if (start == std::string::npos)
    return "";
  return s.substr(start, end - start + 1);
}