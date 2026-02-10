
#include "AhnalyticBase/database/FileDatabase.hpp"
#include "AhnalyticBase/database/SnippedDatabase.hpp"
#include "AhnalyticBase/github/Github.hpp"
#include "AhnalyticBase/github/GithubCrawler.hpp"
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
  std::string workPathSmall = "S:/work";
  std::string dbPath = "D:/source/git/Ahnalytic/db";

  if (input)
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
    db.processRepos(langFilter, true, false, [workPathBig, workPathSmall, &poolBig, &poolSmall, &argv, dbPath](RepoInfo info)
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