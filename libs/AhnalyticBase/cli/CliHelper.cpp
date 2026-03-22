#include "CliHelper.hpp"

#include "AhnalyticBase/cli/GitCliHelper.hpp"
#include "AhnalyticBase/cli/SvnCliHelper.hpp"
#include "AhnalyticBase/cli/MercurialCliHelper.hpp"

CliHelperWrapper::CliHelperWrapper(const std::string& typeName)
{
  if (typeName == "git")
    type = Git;
  else if (typeName == "svn")
    type = Svn;
  else if (typeName == "hg")
    type = Mercurial;
}

std::string CliHelperWrapper::getHeadId(const std::string& url, const std::string& tempPath)
{
  std::string ret;

  switch (type)
  {
  case Git:
    ret = GitCliHelperC::getHeadSha(url, tempPath);
    break;
  case Svn:
    ret = SvnCliHelperC::getHeadRevision(url, tempPath);
    break;
  case Mercurial:
    ret = MercurialCliHelperC::getHeadChangeSet(url, tempPath);
    break;
  }

  return ret;
}

std::vector<TagData> CliHelperWrapper::getTagData(const std::string& url, const std::string& tempPath)
{
  std::vector<TagData> ret;

  switch (type)
  {
  case Git:
    ret = GitCliHelperC::getGitTagData(url, tempPath);
    break;
  case Svn:
    ret = SvnCliHelperC::getSvnTagData(url, tempPath);
    break;
  case Mercurial:
    ret = MercurialCliHelperC::getMercurialTagData(url, tempPath);
    break;
  }

  return ret;
}

std::vector<std::string> CliHelperWrapper::getFiles(const std::string& name, const std::string& url, const std::string& tempPath)
{
  std::vector<std::string> ret;

  switch (type)
  {
  case Git:

    if (repoPath.size() == 0)
      repoPath = GitCliHelperC::getGitCloneShallow(name, url, tempPath);
    ret = GitCliHelperC::getGitFiles(name, url, tempPath);
    break;
  case Svn:
    //if (repoPath.size() == 0)
      //repoPath = SvnCliHelperC::getSvnCloneShallow(name, url, tempPath);
    ret = SvnCliHelperC::getSvnFiles(name, url, tempPath);
    break;
  case Mercurial:
    if (repoPath.size() == 0)
      repoPath = MercurialCliHelperC::getMercurialCloneShallow(name, url, tempPath);
    ret = MercurialCliHelperC::getMercurialFiles(name, url, tempPath);
    break;
  }

  return ret;
}

std::string CliHelperWrapper::getCreationDate(const std::string& url, const std::string& tempPath)
{
  std::string ret;

  switch (type)
  {
  case Git:
    ret = GitCliHelperC::getCreationDate(url, tempPath);
    break;
  case Svn:
    ret = SvnCliHelperC::getCreationDate(url, tempPath);
    break;
  case Mercurial:
    ret = MercurialCliHelperC::getCreationDate(url, tempPath);
    break;
  }

  return ret;
}

std::unordered_map<std::string, std::string> CliHelperWrapper::getFilesWithContent(const std::string& repoPath, const std::string& repoUrl,
                                                                                   const std::string& sha, const std::vector<std::string>& files,
                                                                                   const std::string& tempPath)
{
  std::unordered_map<std::string, std::string> ret;

  switch (type)
  {
  case Git:
    ret = GitCliHelperC::getFilesWithContent(repoPath, sha, files);
    break;
  case Svn:
    ret = SvnCliHelperC::getFilesWithContent(repoUrl, sha, files, tempPath);
    break;
  case Mercurial:
    ret = MercurialCliHelperC::getFilesWithContent(repoPath, sha, files);
    break;
  }

  return ret;
}
