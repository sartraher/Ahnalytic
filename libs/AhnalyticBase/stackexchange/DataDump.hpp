#ifndef datadump_hpp__
#define datadump_hpp__

#include "AhnalyticBase/Export.hpp"

#include <list>
#include <string>

typedef char XML_Char;

struct SnippedData
{
  int id;
  std::string code;
  std::string licence;
  std::string date;
};

class DLLEXPORT DataDump
{
public:
  DataDump();
  ~DataDump();

  // std::list<SnippedData> parseXMLFile(const char* fileName);
  void parseXMLFile(const char* fileName);

  bool hasNext();
  SnippedData next();

private:
  struct DataDumpPrivate* privateData = nullptr;

protected:
  void readNext();

  void startElement(const XML_Char* name, const XML_Char** atts);
  void endElement(const XML_Char* name);
  void characterData(const XML_Char* s, int len);

  std::list<std::string> parseBody(const std::string& inputData);

  std::string decodeXML(const std::string& input);
};

#endif