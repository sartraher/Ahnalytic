#include "Diagnostic.hpp"

#include <iostream>
#include <sstream>

Diagnostic::Diagnostic(size_t origSize)
{
  fileSize = origSize;
}

Diagnostic::~Diagnostic()
{
}

void Diagnostic::setDupReduce(size_t size)
{
  dupReduceSize = size;
}

void Diagnostic::setLabel(const std::string& label)
{
  curLabel = label;
}

void Diagnostic::setCompression(size_t inSize, size_t cmpSize, const std::string& compression, const std::string& alternatives)
{
  std::stringstream stringStream;
  stringStream << curLabel << "(" << inSize << "/" << cmpSize << "->" << (double)cmpSize / inSize << "): " << compression << "(" << alternatives << ")";
  compressions.push_back(stringStream.str());
}

void Diagnostic::setResultSize(size_t size)
{
  resultSize = size;
}

void Diagnostic::write()
{
  std::ostream& stream = std::cout;

  stream << "-----------------------------------------------------------------" << std::endl;
  stream << "File size: " << fileSize << std::endl;
  stream << "Dup reduce size: " << dupReduceSize << std::endl;
  stream << "Compressions: " << std::endl;
  for (std::string compression : compressions)
    stream << compression << std::endl;
  stream << "Result size: " << resultSize << "->" << (double)resultSize / fileSize << "%" << std::endl;
  stream << "-----------------------------------------------------------------" << std::endl;
}