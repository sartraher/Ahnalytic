#ifndef vsencodeCompressor_hpp__
#define vsencodeCompressor_hpp__

#include "compressor.hpp"

class VSEncodeCompressor : public CompressorI
{
public:
  VSEncodeCompressor();

  virtual CompressData compress(const CompressData& data);
  virtual CompressData decompress(const CompressData& data);
  virtual std::string getId();
};

#endif