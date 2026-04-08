#include "UpdateRepoFile.hpp"

#include <fstream>
#include <iostream>

std::string UpdateRepoFile::getString(const nlohmann::json& data, const std::string& name)
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

UpdateRepoFile::UpdateRepoFile()
{
}

UpdateRepoFile::~UpdateRepoFile()
{
}

void UpdateRepoFile::read(const std::filesystem::path& path)
{
  if (std::filesystem::exists(path))
  {
    std::ifstream file(path);
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    readBuffer(content);
  }
}

void UpdateRepoFile::write(const std::filesystem::path& path)
{
  nlohmann::json statusData;

  for (const UpdateRepoData& repoInfo : updateRepoData)
  {
    nlohmann::json infoData;
    infoData["Licence"] = repoInfo.licence;
    infoData["Name"] = repoInfo.name;
    infoData["Url"] = repoInfo.url;
    infoData["Language"] = repoInfo.language;
    infoData["Type"] = repoInfo.type;
    infoData["Version"] = repoInfo.version;

    for (const UpdateRepoTagData& tag : repoInfo.tags)
    {
      nlohmann::json infoTagData;
      infoTagData["TagName"] = tag.tagName;
      infoTagData["Sha"] = tag.sha;
      infoData["Tags"].push_back(infoTagData);
    }

    statusData.push_back(infoData);
  }

  std::ofstream ofs(path.string(), std::ios::out | std::ios::trunc);
  ofs << statusData.dump(2);
}

void UpdateRepoFile::readBuffer(const std::string& buffer)
{
  updateRepoData.clear();

  try
  {
    nlohmann::json repoData = nlohmann::json::parse(buffer);

    updateRepoData.reserve(repoData.size());

    for (int index = 0; index < repoData.size(); index++)
    {
      UpdateRepoData repoInfo;

      repoInfo.licence = getString(repoData[index], "Licence");
      repoInfo.name = getString(repoData[index], "Name");
      repoInfo.url = getString(repoData[index], "Url");
      repoInfo.language = getString(repoData[index], "Language");
      repoInfo.type = getString(repoData[index], "Type");
      repoInfo.version = getString(repoData[index], "Version");

      for (int tagIndex = 0; tagIndex < repoData[index]["Tags"].size(); tagIndex++)
      {
        UpdateRepoTagData tagData;

        tagData.tagName = getString(repoData[index]["Tags"][tagIndex], "TagName");
        tagData.sha = getString(repoData[index]["Tags"][tagIndex], "Sha");

        repoInfo.tags.push_back(tagData);
      }

      updateRepoData.push_back(repoInfo);
    }
  }
  catch (const std::exception& e)
  {
    std::cerr << "JSON parse error: " << e.what() << "\n";
  }
}