#include "DataDump.hpp"

#include <fstream>
#include <iostream>
#include <map>
#include <regex>
#include <sstream>
#include <string>

#include <expat.h>

constexpr size_t bufferSize = 1024 * 1024;

struct DataDumpPrivate
{
  std::list<SnippedData> snippets;
  XML_Parser parser;
  FILE* file = nullptr;

  char* buffer;
};

DataDump::DataDump() : privateData(new DataDumpPrivate())
{
  privateData->buffer = new char[bufferSize];
}

DataDump::~DataDump()
{
  XML_ParserFree(privateData->parser);
  fclose(privateData->file);

  delete privateData->buffer;
  delete privateData;
}

void DataDump::parseXMLFile(const char* fileName)
{
  errno_t error = fopen_s(&privateData->file, fileName, "r");

  if (error == 0)
  {
    if (!privateData->file)
    {
      std::cerr << "Error opening file." << std::endl;
      return;
    }

    privateData->parser = XML_ParserCreate(nullptr);
    // CodeSnippetHandler handler;
    XML_SetUserData(privateData->parser, this);

    // Set up event handlers
    XML_SetElementHandler(privateData->parser, [](void* data, const XML_Char* name, const XML_Char** atts)
    { static_cast<DataDump*>(data)->startElement(name, atts); }, [](void* data, const XML_Char* name) { static_cast<DataDump*>(data)->endElement(name); });

    XML_SetCharacterDataHandler(privateData->parser, [](void* data, const XML_Char* s, int len) { static_cast<DataDump*>(data)->characterData(s, len); });
  }
}

bool DataDump::hasNext()
{
  if (privateData->snippets.size() == 0)
    readNext();

  return privateData->snippets.size() > 0;
}

SnippedData DataDump::next()
{
  if (privateData->snippets.size() == 0)
    readNext();

  SnippedData ret = privateData->snippets.front();
  privateData->snippets.pop_front();
  return ret;
}

void DataDump::readNext()
{
  // char buffer[1024];
  int bytesRead = 0;
  while ((bytesRead = fread(privateData->buffer, 1, bufferSize, privateData->file)) > 0)
  {
    if (XML_Parse(privateData->parser, privateData->buffer, bytesRead, bytesRead == 0) == XML_STATUS_ERROR)
    {
      std::cerr << "XML Parse Error: " << XML_ErrorString(XML_GetErrorCode(privateData->parser)) << std::endl;
      break;
    }

    if (privateData->snippets.size() > 0)
      break;
  }
}

void DataDump::startElement(const XML_Char* name, const XML_Char** atts)
{
  if (strcmp(name, "row") == 0)
  {
    int index = 0;

    SnippedData data;

    std::vector<std::string> codes;

    while (atts[index] != nullptr)
    {
      if (strcmp(atts[index], "Body") == 0)
      {
        std::string body = atts[index + 1];
        std::list<std::string> codesAll = parseBody(body);
        for (const std::string& code : codesAll)
          if (code.size() > 200)
            codes.push_back(decodeXML(code));
        // snippets.push_back(code);
      }
      else if (strcmp(atts[index], "Id") == 0)
      {
        data.id = atts[index + 1];
      }
      else if (strcmp(atts[index], "ContentLicense") == 0)
      {
        data.licence = atts[index + 1];
      }
      else if (strcmp(atts[index], "CreationDate") == 0)
      {
        data.date = atts[index + 1];
      }

      index += 2;
    }

    for (const std::string& code : codes)
    {
      data.code = code;
      privateData->snippets.push_back(data);
    }
  }
}

void DataDump::endElement(const XML_Char*)
{
}

void DataDump::characterData(const XML_Char*, int)
{
}

/*
std::list<std::string> DataDump::parseBody(const std::string& inputData)
{
  std::list<std::string> codeBlocks;

  // Regular expression to match text between <code> and </code>, handling newlines using [\s\S]
  std::regex codeBlockPattern(R"(<code>([\s\S]*?)</code>)", std::regex::icase);

  // Iterator to go through all matches in the input data
  auto words_begin = std::sregex_iterator(inputData.begin(), inputData.end(), codeBlockPattern);
  auto words_end = std::sregex_iterator();

  for (std::sregex_iterator i = words_begin; i != words_end; ++i)
  {
    // Get the matched text (group 1 is the text inside <code>...</code>)
    codeBlocks.push_back((*i)[1].str());
  }

  return codeBlocks;
}*/

std::list<std::string> DataDump::parseBody(const std::string& inputData)
{
  std::list<std::string> codeBlocks;

  const std::string startTag = "<code>";
  const std::string endTag = "</code>";

  size_t pos = 0;

  while (true)
  {
    // Find start tag
    size_t start = inputData.find(startTag, pos);
    if (start == std::string::npos)
      break; // No more blocks

    start += startTag.length();

    // Find end tag
    size_t end = inputData.find(endTag, start);
    if (end == std::string::npos)
      break; // Malformed input, stop parsing

    codeBlocks.emplace_back(inputData.substr(start, end - start));

    pos = end + endTag.length();
  }

  return codeBlocks;
}

std::string DataDump::decodeXML(const std::string& input)
{
  // Map for XML escape sequences to their corresponding characters
  std::map<std::string, char> xml_entities = {{"amp", '&'}, {"lt", '<'}, {"gt", '>'}, {"quot", '\"'}, {"apos", '\''}, {"copy", '©'}, {"reg", '®'}};

  std::string decoded;
  size_t i = 0;
  while (i < input.size())
  {
    if (input[i] == '&')
    {
      // Try to find the end of the entity
      size_t semicolon_pos = input.find(';', i);
      if (semicolon_pos != std::string::npos)
      {
        // Extract the entity inside the & and ;
        std::string entity = input.substr(i + 1, semicolon_pos - i - 1);

        // Check if the entity is in the map
        auto it = xml_entities.find(entity);
        if (it != xml_entities.end())
        {
          // Append the decoded character
          decoded += it->second;
        }
        else
        {
          // If it's not a valid entity, keep it as-is (including '&' and ';')
          decoded += '&' + entity + ';';
        }

        // Move the index past the semicolon
        i = semicolon_pos + 1;
      }
      else
      {
        // If there's no semicolon, it's a malformed entity; add the '&' as-is
        decoded += '&';
        ++i;
      }
    }
    else
    {
      // If it's not an entity, just append the character
      decoded += input[i];
      ++i;
    }
  }

  return decoded;
}