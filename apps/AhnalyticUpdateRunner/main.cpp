
#include "AhnalyticBase/database/FileDatabase.hpp"
#include "AhnalyticBase/database/SnippedDatabase.hpp"
#include "AhnalyticBase/github/Github.hpp"
#include "AhnalyticBase/github/GithubCrawler.hpp"
#include "AhnalyticBase/helper/GitCliHelper.hpp"
#include "AhnalyticBase/stackexchange/DataDump.hpp"
#include "AhnalyticBase/tree/SourceScanner.hpp"

#include "BS_thread_pool.hpp"

#include "args/args.hxx"

#include <fstream>

static bool stopGracefully = false;

int main(int argc, char* argv[])
{
  args::ArgumentParser parser("", "");
  args::HelpFlag help(parser, "help", "Display this help menu", {'h', "help"});
  args::Group arguments(parser, "arguments", args::Group::Validators::DontCare, args::Options::Global);
  args::ValueFlag<std::string> input(arguments, "input", "", {"input"});
  args::Flag test(arguments, "test", "", {"test"});

  try
  {
    parser.ParseCLI(argc, argv);
  }
  catch (const args::Completion& e)
  {
    std::cout << e.what();
    return 0;
  }
  catch (const args::Help&)
  {
    std::cout << parser;
    return 0;
  }
  catch (const args::ParseError& e)
  {
    std::cerr << e.what() << std::endl;
    std::cerr << parser;
    return 1;
  }

  std::string workPathBig = "R:/";
  std::string workPathSmall = "R:/";
  std::string dbPath = "E:/db";

  if (test)
  {
    std::string testUri = "https://github.com/mongodb/mongo.git";

    std::filesystem::path tempPath = std::filesystem::path(workPathBig) / "mongotest";
    std::filesystem::path repoPath = tempPath / "mongotest";
    std::filesystem::path workPath = repoPath / "work";

    std::vector<GitTagData> tagData = GitCliHelperC::getGitTagData(testUri, tempPath.string());

    std::string lastSha = tagData.at(0).sha;
    std::string sha = tagData.at(1).sha;

    GitCliHelperC::getGitClone(repoPath, testUri, tempPath.string());
    GitCliHelperC::fetchTag(repoPath.string(), lastSha, tempPath.string());
    GitCliHelperC::fetchTag(repoPath.string(), sha, tempPath.string());
    std::vector<std::string> files = GitCliHelperC::getGitFiles("mongotest", testUri, sha, lastSha, tempPath.string());

    static const std::vector<std::string> denyDirs = {"third_party", "3rdparty", "vendor", "vendors",      "external",
                                                      "externals",   "deps",     "dep",    "node_modules", ".git"};

    auto hasSupportedExtension = [](const std::string& path, const std::list<std::string>& exts)
    {
      for (const auto& ext : exts)
      {
        if (path.size() >= ext.size() && path.compare(path.size() - ext.size(), ext.size(), ext) == 0)
          return true;
      }
      return false;
    };

    std::list<std::string> supportedExt = {".cpp", ".c", ".cxx", ".hpp", ".h", ".hxx"};

    std::vector<std::string> filesFilteres;
    for (const std::string& file : files)
    {
      if (hasSupportedExtension(file, supportedExt))
      {
        bool skip = false;
        for (const std::string& denyPath : denyDirs)
        {
          if (file.find(denyPath) != std::string_view::npos)
          {
            skip = true;
            break;
          }
        }

        if (!skip)
          filesFilteres.push_back(file);
      }
    }

    std::unordered_map<std::string, std::string> result = GitCliHelperC::getFilesWithContent(repoPath.string(), sha, filesFilteres);
  }
  else if (input)
  {
    std::string path = args::get(input);

    RepoInfo info;
    info.deserialize(nlohmann::json::parse(std::ifstream(path)));

    std::string cleanName = GitHubHandler::cleanFileName(info.fullName);

    if (info.tags.size() >= 5)
    {
      std::filesystem::path workResPathBig = workPathBig;
      workResPathBig = workResPathBig.concat(cleanName);

      GitHubHandler handler(dbPath, workResPathBig.string());
      handler.scanRepo(info);
    }
    else
    {
      std::filesystem::path workResPathSmall = workPathSmall;
      workResPathSmall = workResPathSmall.concat(cleanName);

      GitHubHandler handler(dbPath, workResPathSmall.string());
      handler.scanRepo(info);
    }
  }
  else
  {
    // GitHubHandler handler("D:/source/Ahnalytic/db", "D:/work");

    BS::thread_pool poolBig(4);
    BS::thread_pool poolSmall(4);

    std::vector<std::string> langFilter{"C", "C++"};

    GitHubRepoDatabase db(DBType::SQLite, dbPath + "/base/github/github.db");
    db.processRepos(langFilter, false, false, [workPathBig, workPathSmall, &poolBig, &poolSmall, &argv, dbPath, &db](RepoInfo info)
    {
      if (stopGracefully)
        return;

      std::string cleanName = GitHubHandler::cleanFileName(info.fullName);

      if (cleanName.find("linux") != std::string::npos || cleanName.find("kernel") != std::string::npos)
        return;

      std::filesystem::path resPath = dbPath;
      resPath = resPath.concat("/CPP").concat("/github/").concat(cleanName + "_CPP.db");

      std::filesystem::path lastPath = dbPath;
      lastPath = lastPath.concat("/CPP").concat("/github_last/").concat(cleanName + "_CPP.db");

      if (!std::filesystem::exists(resPath) && std::filesystem::exists(lastPath))
        std::filesystem::copy(lastPath, resPath);

      if (std::filesystem::exists(resPath))
        return;

      info.tags = db.loadTags(info.id);

      if (std::filesystem::exists(resPath))
      {
        FileDatabase* db = new FileDatabase(DBType::SQLite, resPath.string());

        std::unordered_map<std::string, std::string> tags = db->getTags();

        std::vector<TagInfo> resTags;
        info.tags.reserve(info.tags.size());
        for (const TagInfo& tag : info.tags)
          if (!tags.contains(tag.name))
            resTags.push_back(tag);
        info.tags = resTags;

        if (info.tags.size() == 0)
          return;
      }

      resPath = dbPath;
      resPath = resPath.concat("/CPP").concat("/github/").concat(cleanName + "_CPP.empty");

      if (std::filesystem::exists(resPath))
        return;

      // std::string workPath;
      // BS::thread_pool *pool;

      auto startJob = [cleanName, info, &argv](auto& pool, std::string workPath)
      {
        pool.detach_task([workPath, cleanName, info, &argv]()
        {
          std::filesystem::path workResPath = workPath;
          workResPath = workResPath.concat("/" + cleanName);
          std::filesystem::create_directories(workResPath);
          std::filesystem::path workResFile = workResPath;
          workResFile = workResFile.concat("/data.json");

          std::ofstream(workResFile.string()) << info.serialize().dump(2);

          std::string cmd = argv[0];
          cmd += " --input=" + workResFile.string();
          std::system(cmd.c_str());

          std::error_code ec; // non-throwing
          std::filesystem::remove_all(workResPath.string(), ec);
        });
      };

      if (info.tags.size() > 5)
        startJob(poolBig, workPathBig);
      else
        startJob(poolSmall, workPathSmall);

      //  pool.detach_task([&handler, info]() { handler.scanRepo(info); });

      while ((poolSmall.get_tasks_total() + poolBig.get_tasks_total()) > 100000)
        poolSmall.wait_for(std::chrono::minutes(5));
    });

    poolSmall.wait();
    poolBig.wait();
  }
}