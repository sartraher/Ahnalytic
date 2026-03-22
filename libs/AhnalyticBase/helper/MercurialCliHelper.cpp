#include "MercurialCliHelper.hpp"
#include "DataHelper.hpp"

#include <ranges>
#include <algorithm>

std::string MercurialCliHelperC::getHeadChangeSet(const std::string& url, const std::string& tempPath)
{
  std::string cmd = "hg identify -i " + url;
  ExecResult result = DataHelperC::execAndCapture(cmd, tempPath);

  if (result.exitCode != 0 || result.stdoutText.empty())
    return "";

  std::string out = result.stdoutText;
  out.erase(std::remove(out.begin(), out.end(), '\n'), out.end());
  return out;
}

std::vector<TagData> MercurialCliHelperC::getMercurialTagData(const std::string& url, const std::string& tempPath)
{
  std::vector<TagData> ret;

  std::string cmd = "hg tags " + url;
  ExecResult result = DataHelperC::execAndCapture(cmd, tempPath);

  if (result.exitCode != 0)
    return ret;

  for (auto part : result.stdoutText | std::views::split('\n'))
  {
    std::string line(part.begin(), part.end());
    if (line.empty())
      continue;

    // format: name  rev:hash
    auto pos = line.find(' ');
    if (pos == std::string::npos)
      continue;

    std::string name = line.substr(0, pos);
    if (name == "tip")
      continue;

    auto colon = line.find(':', pos);
    if (colon == std::string::npos)
      continue;

    std::string hash = line.substr(colon + 1);

    ret.push_back({name, hash});
  }

  return ret;
}

std::string MercurialCliHelperC::getMercurialCloneShallow(const std::string& name, const std::string& url, const std::string& tempPath)
{
  std::filesystem::path repoPath = std::filesystem::path(tempPath) / name;
  return getMercurialCloneShallow(repoPath, url, tempPath);
}

std::string MercurialCliHelperC::getMercurialCloneShallow(const std::filesystem::path& repoPath, const std::string& url, const std::string& tempPath)
{
  std::string cmd = "hg clone --noupdate " + url + " \"" + repoPath.string() + "\"";
  DataHelperC::execAndCapture(cmd, tempPath);
  return repoPath.string();
}

std::vector<std::string> MercurialCliHelperC::getMercurialFiles(const std::string& name, const std::string& url, const std::string& tempPath)
{
  std::filesystem::path repoPath = std::filesystem::path(tempPath) / name;

  std::string cmd = "hg -R \"" + repoPath.string() + "\" manifest";
  ExecResult result = DataHelperC::execAndCapture(cmd, tempPath);

  std::vector<std::string> ret;
  if (result.exitCode != 0)
    return ret;

  for (auto part : result.stdoutText | std::views::split('\n'))
    ret.emplace_back(part.begin(), part.end());

  return ret;
}

std::vector<std::string> MercurialCliHelperC::getMercurialFiles(const std::string& name, const std::string& url, const std::string& sha,
                                                                const std::string& tempPath)
{
  std::filesystem::path repoPath = std::filesystem::path(tempPath) / name;

  std::string cmd = "hg -R \"" + repoPath.string() + "\" manifest -r " + sha;
  ExecResult result = DataHelperC::execAndCapture(cmd, tempPath);

  std::vector<std::string> ret;
  if (result.exitCode != 0)
    return ret;

  for (auto part : result.stdoutText | std::views::split('\n'))
    ret.emplace_back(part.begin(), part.end());

  return ret;
}

std::vector<std::string> MercurialCliHelperC::getMercurialFiles(const std::string& name, const std::string& url, const std::string& sha,
                                                                const std::string& lastSha, const std::string& tempPath)
{
  std::filesystem::path repoPath = std::filesystem::path(tempPath) / name;

  std::string cmd = "hg -R \"" + repoPath.string() + "\" status --rev " + lastSha + " --rev " + sha + " -man";
  ExecResult result = DataHelperC::execAndCapture(cmd, tempPath);

  std::vector<std::string> ret;
  if (result.exitCode != 0)
    return ret;

  for (auto part : result.stdoutText | std::views::split('\n'))
  {
    std::string line(part.begin(), part.end());
    if (line.size() > 2)
      ret.push_back(line.substr(2));
  }

  return ret;
}

std::string MercurialCliHelperC::getCreationDate(const std::string& url, const std::string& tempPath)
{
  std::string cmd = "hg log -r 0 --template \"{date|isodate}\" " + url;
  ExecResult result = DataHelperC::execAndCapture(cmd, tempPath);
  return result.stdoutText;
}

std::unordered_map<std::string, std::string> MercurialCliHelperC::getFilesWithContent(const std::string& repoPath, const std::string& sha,
                                                                                      const std::vector<std::string>& files)
{
  std::unordered_map<std::string, std::string> ret;

  for (const auto& file : files)
  {
    std::string cmd = "hg -R \"" + repoPath + "\" cat -r " + sha + " \"" + file + "\"";
    ExecResult result = DataHelperC::execAndCapture(cmd, repoPath);

    if (result.exitCode == 0)
      ret[file] = result.stdoutText;
  }

  return ret;
}
