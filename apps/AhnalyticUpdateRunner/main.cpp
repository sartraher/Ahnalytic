
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

  std::string workPath = "R:/";
  std::string dbPath = "D:/source/git/Ahnalytic/db";

  if (input)
  {
    std::string path = args::get(input);

    RepoInfo info;
    info.deserialize(nlohmann::json::parse(std::ifstream(path)));

    std::string cleanName = GitHubHandler::cleanFileName(info.fullName);

    std::filesystem::path workResPath = workPath;
    workResPath = workResPath.concat(cleanName);

    GitHubHandler handler(dbPath, workResPath.string());
    handler.scanRepo(info);
  }
  else
  {
    // GitHubHandler handler("D:/source/Ahnalytic/db", "D:/work");

    BS::thread_pool pool(4);

    std::vector<std::string> langFilter{"C", "C++"};

    GitHubRepoDatabase db(DBType::SQLite, dbPath + "/base/github/github.db");
    db.processRepos(langFilter, true, false, [workPath, &pool, &argv, dbPath](RepoInfo info)
    {
      if (stopGracefully)
        return;

      // if (info.language == "C" || info.language == "C++")
      {
        std::string cleanName = GitHubHandler::cleanFileName(info.fullName);

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

        //  pool.detach_task([&handler, info]() { handler.scanRepo(info); });

        while (pool.get_tasks_total() > 1000)
          pool.wait_for(std::chrono::minutes(5));
      }
    });

    pool.wait();
  }
}