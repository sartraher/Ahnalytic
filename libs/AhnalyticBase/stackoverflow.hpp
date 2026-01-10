#ifndef stackoverflow_hpp__
#define stackoverflow_hpp__

#include "AhnalyticBase/export.hpp"

#include <string>

class DLLEXPORT StackOverflowHandler
{
public:
  StackOverflowHandler();

  void convertDataDump(const std::string& commentsXmlPath, const std::string& dbPath);
  void importData(const std::string& stackDb, const std::string& outDb);

private:
protected:
};

#endif