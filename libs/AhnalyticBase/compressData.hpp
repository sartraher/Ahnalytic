#ifndef compressdata_hpp__
#define compressdata_hpp__

#include "export.hpp"

#include <vector>

enum class DLLEXPORT CompressionAlgosE : uint32_t
{
  None = 0,
  // GZip = 1,
  VSEncoding = 2,
  SIMDoptpFor = 3,
  LZMA = 4,
  ZStd = 5,
  BSC = 6,
  LZ4 = 7,
  Test = 8
};

enum class DLLEXPORT ModAlgosE : uint32_t
{
  None = 0,
  Delta = 1
};

struct DLLEXPORT CompressDataHeader
{
  CompressionAlgosE algo = CompressionAlgosE::None;
  ModAlgosE modifier = ModAlgosE::None;
  uint32_t originalSize = 0;
};

class DLLEXPORT CompressData
{
public:
  CompressData(const std::vector<uint32_t>& inData, bool withHeader = false);
  CompressData(const std::vector<char>& inData, bool withHeader = false);

  size_t getCharSize() const;
  size_t getUint32Size() const;

  std::vector<uint32_t> getUint32Data() const;
  std::vector<char> getCharData() const;

  const CompressDataHeader& getHeader() const;
  void setHeader(const CompressDataHeader& header);

private:
protected:
  std::vector<uint32_t> data;
  size_t charSize;

  CompressDataHeader headers;
};

#endif