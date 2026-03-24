
#include "AhnalyticBase/database/FileDatabase.hpp"
#include "AhnalyticBase/database/SnippedDatabase.hpp"
#include "AhnalyticBase/database/StackExchangeExtractDatabase.hpp"
#include "AhnalyticBase/github/Github.hpp"
#include "AhnalyticBase/github/GithubCrawler.hpp"
#include "AhnalyticBase/helper/ArchiveHelper.hpp"
#include "AhnalyticBase/helper/Enviroment.hpp"
#include "AhnalyticBase/helper/SignHelper.hpp"
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
  args::ValueFlag<std::string> output(arguments, "output", "", {"output"});
  args::ValueFlag<std::string> type(arguments, "type", "", {"type"});

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

  if (input && output && type)
  {
    std::string inPath = args::get(input);
    std::filesystem::path outPath = args::get(output);
    std::string typeStr = args::get(type);

    if (typeStr == "github")
    {
      nlohmann::json outJson;
      nlohmann::json outJsonFull;

      BS::thread_pool pool;
      std::mutex mutex;

      for (const auto& entry : std::filesystem::directory_iterator(inPath))
      {
        if (entry.is_regular_file() && entry.path().extension() == ".db")
        {
          std::filesystem::path inPath = entry.path();
          std::filesystem::path outPathSub = outPath / entry.path().stem();
          std::filesystem::path tagFile = outPathSub / "tags.json";
          std::filesystem::path repoFile = outPathSub / "repo.json";
          pool.detach_task([outPathSub, inPath, &outJson, &outJsonFull, &mutex, tagFile, repoFile]()
          {
            if (!std::filesystem::exists(tagFile))
            {
              FileDatabase inDb(DBType::SQLite, inPath.string());

              std::filesystem::create_directories(outPathSub);

              std::filesystem::path outPathSubCpy = outPathSub;
              inDb.exportData(outPathSubCpy);
              std::cout << inPath << '\n';
            }

            if (std::filesystem::exists(tagFile))
            {
              std::ifstream tagStream(tagFile.string());
              nlohmann::json tagData;
              tagStream >> tagData;

              std::ifstream repoStream(repoFile.string());
              nlohmann::json repoData;
              repoStream >> repoData;

              repoData["type"] = "github";
              repoData["language"] = "CPP";
              repoData["version"] = "1";

              repoData["tags"] = tagData;

              {
                std::lock_guard<std::mutex> lock(mutex);
                outJsonFull.push_back(repoData);
              }

              if (tagData.size() > 0)
              {
                std::string sha = tagData.at(tagData.size() - 1)["Sha"].get<std::string>();

                nlohmann::json entryJson;
                entryJson["name"] = inPath.stem().string();
                entryJson["sha"] = sha;
                entryJson["type"] = "github";
                entryJson["language"] = "CPP";
                entryJson["version"] = "1";

                std::lock_guard<std::mutex> lock(mutex);
                outJson.push_back(entryJson);
              }
            }
          });
        }
      }

      pool.wait();

      std::filesystem::path statusPath = outPath / "status.json";
      std::ofstream repoOut(statusPath.native());
      repoOut << outJson.dump(2);
      repoOut.close();

      std::filesystem::path statusFullPath = outPath / "statusFull.json";
      std::ofstream fullOut(statusFullPath.native());
      fullOut << outJsonFull.dump(2);
      fullOut.close();
    }
    if (typeStr == "stackexchnage_base")
    {
      EnviromentC env;
      StackExchangeExtractDatabase db(DBType::SQLite, inPath);
      db.splitDatabase(outPath.string(), "stackoverflow");

      for (const auto& entry : std::filesystem::directory_iterator(outPath))
      {
        if (entry.is_regular_file() && entry.path().extension() == ".db")
        {
          std::filesystem::path tarGzPath = entry.path();
          tarGzPath = tarGzPath.concat(".tar.gz");
          ArchiveHelper::createTarGz(entry.path(), tarGzPath);

          std::filesystem::path signPath = tarGzPath;
          signPath = signPath.concat(".sig");
          SignHelper::signFile(entry.path().string(), env.privatePath.string(), signPath.string());

          std::error_code ec;
          std::filesystem::remove_all(entry.path(), ec);
        }
      }
    }
  }
}