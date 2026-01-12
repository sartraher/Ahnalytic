#include "CompressData.hpp"

#include <cstring> 

CompressData::CompressData(const std::vector<uint32_t>& inData, bool withHeader)
{
  if (withHeader)
  {
    int headerSize = sizeof(CompressDataHeader) / sizeof(uint32_t);
    data.resize(inData.size() - headerSize);
    charSize = data.size() * sizeof(uint32_t);
    memcpy(&headers, inData.data(), headerSize * sizeof(uint32_t));
    memcpy(data.data(), inData.data() + headerSize, charSize);
  }
  else
  {
    data.resize(inData.size());
    charSize = inData.size() * sizeof(uint32_t);
    memcpy(data.data(), inData.data(), charSize);
  }
}

CompressData::CompressData(const std::vector<char>& inData, bool withHeader)
{
  if (withHeader)
  {
    int headerSize = sizeof(CompressDataHeader) / sizeof(uint32_t);
    int resInSize = inData.size() - sizeof(CompressDataHeader);
    size_t resSize = resInSize / sizeof(uint32_t) + ((resInSize % sizeof(uint32_t)) > 0 ? 1 : 0);
    data.resize(resSize);
    charSize = resInSize;
    memcpy(&headers, inData.data(), headerSize * sizeof(uint32_t));
    memcpy(data.data(), inData.data() + sizeof(CompressDataHeader), charSize);
  }
  else
  {
    size_t resSize = inData.size() / sizeof(uint32_t) + ((inData.size() % sizeof(uint32_t)) > 0 ? 1 : 0);
    data.resize(resSize);
    charSize = inData.size();
    memcpy(data.data(), inData.data(), charSize);
  }
}

size_t CompressData::getCharSize() const
{
  return charSize;
}

size_t CompressData::getUint32Size() const
{
  return data.size();
}

std::vector<uint32_t> CompressData::getUint32Data() const
{
  std::vector<uint32_t> ret;

  if (headers.algo == CompressionAlgosE::None)
  {
    ret.resize(data.size());
    memcpy(ret.data(), data.data(), charSize);
  }
  else
  {
    ret.resize(data.size() + sizeof(CompressDataHeader) / sizeof(uint32_t));
    memcpy(ret.data(), &headers, sizeof(CompressDataHeader));
    memcpy(ret.data() + 3, data.data(), charSize);
  }

  return ret;
}

std::vector<char> CompressData::getCharData() const
{
  std::vector<char> ret;

  if (headers.algo == CompressionAlgosE::None)
  {
    ret.resize(charSize);
    memcpy(ret.data(), data.data(), charSize);
  }
  else
  {
    ret.resize(charSize + sizeof(CompressDataHeader));
    memcpy(ret.data(), &headers, sizeof(CompressDataHeader));
    memcpy(ret.data() + 3 * sizeof(uint32_t), data.data(), charSize);
  }
  return ret;
}

const CompressDataHeader& CompressData::getHeader() const
{
  return headers;
}

void CompressData::setHeader(const CompressDataHeader& header)
{
  headers = header;
}