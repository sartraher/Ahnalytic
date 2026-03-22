#include "Logger.hpp"

#include "AhnalyticBase/helper/Enviroment.hpp"

#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

class LoggerPrivateC
{
public:
  const int maxSize = 1048576 * 5;
  const int maxFiles = 3;

  std::shared_ptr<spdlog::logger> consoleLogger;
  std::shared_ptr<spdlog::logger> fileLogger;

private:
protected:
};

LoggerC::LoggerC(const EnviromentC& env)
{
  priv = new LoggerPrivateC();

  priv->consoleLogger = spdlog::stdout_color_mt("console");
  priv->fileLogger = spdlog::rotating_logger_mt("fileLogger", (env.dataFolder / "log.txt").string(), priv->maxSize, priv->maxFiles);

  spdlog::set_pattern("[%H:%M:%S %z] [%n] [%^---%L---%$] [thread %t] %v");

  spdlog::set_level(spdlog::level::info);
}

LoggerC::~LoggerC()
{
  delete priv;
}

void LoggerC::LogDebug(const std::string& msg)
{
  spdlog::get("console")->debug(msg);
  spdlog::get("fileLogger")->debug(msg);
}

void LoggerC::LogInfo(const std::string& msg)
{
  spdlog::get("console")->info(msg);
  spdlog::get("fileLogger")->info(msg);
}

void LoggerC::LogWarn(const std::string& msg)
{
  spdlog::get("console")->warn(msg);
  spdlog::get("fileLogger")->warn(msg);
}

void LoggerC::LogError(const std::string& msg)
{
  spdlog::get("console")->error(msg);
  spdlog::get("fileLogger")->error(msg);
}
