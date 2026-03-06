#include "GitCliHelper.hpp"
#include "DataHelper.hpp"

#include <filesystem>
#include <fstream>
#include <ranges>
#include <thread>
#include <unordered_map>

std::vector<GitTagData> GitCliHelperC::parseTags(const std::string& lines, bool isHead)
{
  std::vector<GitTagData> ret;
  std::unordered_map<std::string, size_t> lookUp;

  for (auto part : lines | std::views::split('\n'))
  {
    std::string line(part.begin(), part.end());

    std::vector<std::string> segments;
    for (auto segment : line | std::views::split('\t'))
      segments.push_back(std::string(segment.begin(), segment.end()));

    if (segments.size() == 2)
    {
      std::string sha = segments[0];

      std::string name;
      if (isHead)
        name = "HEAD";
      else
        name = segments[1].substr(10); // +10:refs/tags/

      GitTagData tagData;
      tagData.sha = segments[0];
      if (name.ends_with("^{}"))
      {
        tagData.name = name.substr(0, name.size() - 3);

        auto iter = lookUp.find(tagData.name);
        if (iter != lookUp.end())
          ret[iter->second] = std::move(tagData);
        else
        {
          lookUp[name] = ret.size();
          ret.push_back(std::move(tagData));
        }
      }
      else
      {
        tagData.name = name;
        lookUp[name] = ret.size();
        ret.push_back(std::move(tagData));
      }
    }
  }

  return ret;
}

std::vector<GitTagData> GitCliHelperC::getGitTagData(const std::string& url, const std::string& tempPath)
{
  std::vector<GitTagData> ret;

  std::string cloneCmd = "git ls-remote --tags " + url;
  ExecResult result = DataHelperC::execAndCapture(cloneCmd, tempPath);
  if (result.exitCode != 0)
    return ret;

  if (result.stdoutText.size() > 0)
    ret = parseTags(result.stdoutText, false);

  return ret;
}

std::string GitCliHelperC::getHeadSha(const std::string& url, const std::string& tempPath)
{
  std::string ret;

  std::string cloneCmd = "git ls-remote " + url + " HEAD";
  ExecResult result = DataHelperC::execAndCapture(cloneCmd, tempPath);
  if (result.exitCode != 0)
    return ret;

  if (result.stdoutText.size() > 0)
  {
    std::vector<GitTagData> tags = parseTags(result.stdoutText, true);
    if (tags.size() > 0)
      ret = tags[0].sha;
  }

  return ret;
}

std::string GitCliHelperC::getGitCloneShallow(const std::string& name, const std::string& url, const std::string& tempPath)
{
  std::filesystem::path repoPath = std::filesystem::path(tempPath) / name;
  return getGitCloneShallow(repoPath, url, tempPath);
}

std::string GitCliHelperC::getGitCloneShallow(const std::filesystem::path& repoPath, const std::string& url, const std::string& tempPath)
{
  std::string cloneCmd = "git clone --depth=1 --filter=blob:none --no-checkout " + url + " \"" + repoPath.string() + "\"";
  ExecResult result = DataHelperC::execAndCapture(cloneCmd, tempPath);

  return repoPath.string();
}

std::string GitCliHelperC::getGitClone(const std::string& name, const std::string& url, const std::string& tempPath)
{
  std::filesystem::path repoPath = std::filesystem::path(tempPath) / name;
  return getGitClone(repoPath, url, tempPath);
}

std::string GitCliHelperC::getGitClone(const std::filesystem::path& repoPath, const std::string& url, const std::string& tempPath)
{
  std::string cloneCmd = "git clone --no-checkout " + url + " \"" + repoPath.string() + "\"";
  ExecResult result = DataHelperC::execAndCapture(cloneCmd, tempPath);

  return repoPath.string();
}

void GitCliHelperC::fetchTag(const std::string& repoPath, const std::string& sha, const std::string& tempPath)
{
  std::string cloneCmd = "git -C \"" + repoPath + "\" fetch --filter=blob:none origin " + sha;
  ExecResult result = DataHelperC::execAndCapture(cloneCmd, tempPath);
}

std::vector<std::string> GitCliHelperC::getGitFiles(const std::string& name, const std::string& url, const std::string& tempPath)
{
  std::vector<std::string> ret;

  std::filesystem::path repoPath = std::filesystem::path(tempPath) / name;

  std::string getFilesCmd = "git -C \"" + repoPath.string() + "\" ls-tree -r --name-only HEAD";
  ExecResult result2 = DataHelperC::execAndCapture(getFilesCmd, tempPath);
  if (result2.exitCode != 0)
    return ret;

  for (auto part : result2.stdoutText | std::views::split('\n'))
  {
    std::string line(part.begin(), part.end());
    ret.push_back(std::move(line));
  }

  return ret;
}

std::vector<std::string> GitCliHelperC::getGitFiles(const std::string& name, const std::string& url, const std::string& sha, const std::string& tempPath)
{
  std::vector<std::string> ret;

  std::filesystem::path repoPath = std::filesystem::path(tempPath) / name;

  std::string getFilesCmd = "git -C \"" + repoPath.string() + "\" ls-tree -r --name-only " + sha;
  ExecResult result2 = DataHelperC::execAndCapture(getFilesCmd, tempPath);
  if (result2.exitCode != 0)
    return ret;

  for (auto part : result2.stdoutText | std::views::split('\n'))
  {
    std::string line(part.begin(), part.end());
    ret.push_back(std::move(line));
  }

  return ret;
}

std::vector<std::string> GitCliHelperC::getGitFiles(const std::string& name, const std::string& url, const std::string& sha, const std::string& lastSha,
                                                    const std::string& tempPath)
{
  std::vector<std::string> ret;

  std::filesystem::path repoPath = std::filesystem::path(tempPath) / name;

  std::string getFilesCmd = "git -C \"" + repoPath.string() + "\" diff --diff-filter=AMR --name-only " + lastSha + " " + sha;
  ExecResult result2 = DataHelperC::execAndCapture(getFilesCmd, tempPath);
  if (result2.exitCode != 0)
    return ret;

  for (auto part : result2.stdoutText | std::views::split('\n'))
  {
    std::string line(part.begin(), part.end());
    ret.push_back(std::move(line));
  }

  return ret;
}

std::unordered_map<std::string, std::string> GitCliHelperC::getFilesWithContent(const std::string& repoPath, const std::string& sha,
                                                                                const std::vector<std::string>& files)
{
  std::unordered_map<std::string, std::string> ret;

  std::filesystem::path workPath = std::filesystem::path(repoPath) / "work";
  std::filesystem::create_directories(workPath);

  std::filesystem::path filterPath = std::filesystem::path(workPath) / "files.txt";
  std::ofstream out(filterPath);
  for (const auto& line : files)
    out << line << '\n';
  out.close();

  //std::string getFilesCmd = "git -C \"" + repoPath + "\" --work-tree=work restore --source=" + sha + " --pathspec-from-file=" + filterPath.string() + " --worktree";

  std::string getFilesCmd = "git -C \"" + repoPath + "\" --work-tree=work restore --source=" + sha + " --pathspec-from-file=" + filterPath.string();
  //std::string getFilesCmd = "git -C \"" + repoPath + "\" archive " + sha + " --pathspec-from-file=" + filterPath.string() + " | tar -x -C work";

  ExecResult result2 = DataHelperC::execAndCapture(getFilesCmd, workPath.string());
  if (result2.exitCode != 0)
    return ret;

  for (const auto& file : files)
  {
    std::filesystem::path filePath = workPath / file;
    if (std::filesystem::exists(filePath))
    {
      std::ifstream in(filePath);
      if (in.good())
      {
        std::ostringstream ss;
        ss << in.rdbuf();
        ret[file] = ss.str();
      }
    }
  }

  std::error_code remove_ec;
  std::filesystem::remove_all(workPath, remove_ec);
  return ret;
}

std::string GitCliHelperC::getCreationDate(const std::string& url, const std::string& tempPath)
{
  std::string cloneCmd = "git -C " + url + " log --max-parents=0 --format=%aI";
  ExecResult result = DataHelperC::execAndCapture(cloneCmd, tempPath);
  return result.stdoutText;
}