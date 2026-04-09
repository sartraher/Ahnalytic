#include "AhnalyticBase/helper/Enviroment.hpp"
#include "AhnalyticBase/helper/SignHelper.hpp"

#include "args/args.hxx"

#include <filesystem>
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
    EnviromentC env;

    std::string inPath = args::get(input);

    std::filesystem::path signPath = inPath;
    signPath = signPath.concat(".sig");
    SignHelper::signFile(inPath, env.privatePath.string(), signPath.string());
  }

  return 1;
}