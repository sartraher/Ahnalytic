
#include <mimalloc.h>
#include <new>

#include "AhnalyticBase/helper/Enviroment.hpp"
#include "AhnalyticBase/server/ScanServer.hpp"
#include "AhnalyticBase/stackexchange/StackOverflow.hpp"

#include "args/args.hxx"

#include <iostream>

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

  if (input)
  {
    std::string inPath = args::get(input);
    EnviromentC env;

    std::string stackDbPath = (env.dbFolder / "base/stackexchange/stackoverflow.db").string();
    std::string outDbPath = (env.dbFolder / "CPP/stackexchange/stackoverflow").string();

    StackOverflowHandler handler;
    handler.convertDataDump(inPath, stackDbPath);
    handler.importData(stackDbPath, outDbPath);
  }

  return 1;
}