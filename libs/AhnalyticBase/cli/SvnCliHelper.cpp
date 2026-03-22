#include "SvnCliHelper.hpp"
#include "AhnalyticBase/helper/DataHelper.hpp"

#include <ranges>

inline void trimLineEnd(std::string& line)
{
  while (!line.empty() && (line.back() == '\n' || line.back() == '\r'))
    line.pop_back();
}

std::string SvnCliHelperC::getHeadRevision(const std::string& url, const std::string& tempPath)
{
  std::string cmd = "svn info " + url;
  ExecResult result = DataHelperC::execAndCapture(cmd, tempPath);

  if (result.exitCode != 0)
    return "";

  std::istringstream ss(result.stdoutText);
  std::string line;

  while (std::getline(ss, line))
  {
    trimLineEnd(line);

    if (line.starts_with("Revision:"))
    {
      std::string rev = line.substr(10);
      trimLineEnd(rev);
      return rev;
    }
  }

  return "";
}

std::vector<TagData> SvnCliHelperC::getSvnTagData(const std::string& url, const std::string& tempPath)
{
  std::vector<TagData> ret;

  std::string tagsUrl = url + "/tags/";
  std::string cmd = "svn list " + tagsUrl;

  ExecResult result = DataHelperC::execAndCapture(cmd, tempPath);
  if (result.exitCode != 0)
    return ret;

  for (auto part : result.stdoutText | std::views::split('\n'))
  {
    std::string tag(part.begin(), part.end());
    trimLineEnd(tag);

    if (tag.empty())
      continue;

    if (!tag.empty() && tag.back() == '/')
      tag.pop_back();

    std::string revCmd = "svn info " + tagsUrl + tag;
    ExecResult revRes = DataHelperC::execAndCapture(revCmd, tempPath);

    std::istringstream ss(revRes.stdoutText);
    std::string line;

    while (std::getline(ss, line))
    {
      trimLineEnd(line);

      if (line.starts_with("Revision:"))
      {
        std::string rev = line.substr(10);
        trimLineEnd(rev);
        ret.push_back({tag, rev});
        break;
      }
    }
  }

  return ret;
}

std::string SvnCliHelperC::getSvnCloneShallow(const std::string& name, const std::string& url, const std::string& tempPath)
{
  std::filesystem::path repoPath = std::filesystem::path(tempPath) / name;
  return getSvnCloneShallow(repoPath, url, tempPath);
}

std::string SvnCliHelperC::getSvnCloneShallow(const std::filesystem::path& repoPath, const std::string& url, const std::string& tempPath)
{
  std::string cmd = "svn checkout --depth=empty " + url + " \"" + repoPath.string() + "\"";
  DataHelperC::execAndCapture(cmd, tempPath);
  return repoPath.string();
}

std::vector<std::string> SvnCliHelperC::getSvnFiles(const std::string&, const std::string& url, const std::string& tempPath)
{
  std::string cmd = "svn list -R " + url;
  ExecResult result = DataHelperC::execAndCapture(cmd, tempPath);

  std::vector<std::string> ret;
  if (result.exitCode != 0)
    return ret;

  for (auto part : result.stdoutText | std::views::split('\n'))
  {
    std::string line(part.begin(), part.end());
    trimLineEnd(line);

    if (!line.empty())
      ret.push_back(std::move(line));
  }

  return ret;
}

std::vector<std::string> SvnCliHelperC::getSvnFiles(const std::string&, const std::string& url, const std::string& sha, const std::string& tempPath)
{
  std::string cmd = "svn list -R -r " + sha + " " + url;
  ExecResult result = DataHelperC::execAndCapture(cmd, tempPath);

  std::vector<std::string> ret;
  if (result.exitCode != 0)
    return ret;

  for (auto part : result.stdoutText | std::views::split('\n'))
  {
    std::string line(part.begin(), part.end());
    trimLineEnd(line);

    if (!line.empty())
      ret.push_back(std::move(line));
  }

  return ret;
}

std::vector<std::string> SvnCliHelperC::getSvnFiles(const std::string&, const std::string& url, const std::string& sha, const std::string& lastSha,
                                                    const std::string& tempPath)
{
  std::string cmd = "svn diff --summarize -r " + lastSha + ":" + sha + " " + url;
  ExecResult result = DataHelperC::execAndCapture(cmd, tempPath);

  std::vector<std::string> ret;
  if (result.exitCode != 0)
    return ret;

  for (auto part : result.stdoutText | std::views::split('\n'))
  {
    std::string line(part.begin(), part.end());
    trimLineEnd(line);

    if (line.size() > 8)
    {
      std::string file = line.substr(8);
      trimLineEnd(file);
      ret.push_back(std::move(file));
    }
  }

  return ret;
}

std::string SvnCliHelperC::getCreationDate(const std::string& url, const std::string& tempPath)
{
  std::string cmd = "svn log -r 1 --quiet " + url;
  ExecResult result = DataHelperC::execAndCapture(cmd, tempPath);

  std::string out = result.stdoutText;
  trimLineEnd(out);
  return out;
}

std::unordered_map<std::string, std::string> SvnCliHelperC::getFilesWithContent(const std::string& repoUrl, const std::string& sha,
                                                                                const std::vector<std::string>& files, const std::string& tempPath)
{
  std::unordered_map<std::string, std::string> ret;

  for (const auto& file : files)
  {
    std::string cmd = "svn cat -r " + sha + " \"" + repoUrl + "/" + file + "\"";
    ExecResult result = DataHelperC::execAndCapture(cmd, tempPath);

    if (result.exitCode == 0)
      ret[file] = result.stdoutText; // keep raw content
  }

  return ret;
}