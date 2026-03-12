#include "UpdateStatusFile.hpp"

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include <fstream>
#include <iostream>

std::string UpdateStatusFile::getString(const nlohmann::json& data, const std::string& name)
try
{
  if (data.contains(name))
  {
    const auto& value = data[name];
    if (value.is_string())
      return value.get<std::string>();
  }

  return "";
}
catch (const std::exception&)
{
  return "";
}

UpdateStatusFile::UpdateStatusFile()
{
}

UpdateStatusFile::~UpdateStatusFile()
{
}

void UpdateStatusFile::read(const std::filesystem::path& path)
{
  if (std::filesystem::exists(path))
  {
    std::ifstream file(path);
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    readBuffer(content);
  }
}

void UpdateStatusFile::write(const std::filesystem::path& path)
{
}

void UpdateStatusFile::readBuffer(const std::string& buffer)
{
  infos.clear();

  try
  {
    nlohmann::json statusData = nlohmann::json::parse(buffer);

    infos.reserve(statusData.size());

    for (int index = 0; index < statusData.size(); index++)
    {
      UpdateInfo info;
      info.name = getString(statusData[index], "name");
      info.baseName = getString(statusData[index], "baseName");
      info.sha = getString(statusData[index], "sha");
      info.maxVersion = getString(statusData[index], "maxVersion");
      info.type = getString(statusData[index], "type");
      info.language = getString(statusData[index], "language");

      infos.push_back(info);

      // if (std::find(installedUpdates.begin(), installedUpdates.end(), info) == installedUpdates.end())
      // ret.push_back(info);
    }
  }
  catch (const std::exception& e)
  {
    std::cerr << "JSON parse error: " << e.what() << "\n";
  }
}