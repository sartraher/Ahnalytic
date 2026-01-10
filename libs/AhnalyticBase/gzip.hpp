#ifndef gzipCompressor_hpp__
#define gzipCompressor_hpp__

#include "compressor.hpp"

class GZipCompressor : public CompressorI
{
public:
  GZipCompressor();

  virtual CompressData compress(const CompressData& data);
  virtual CompressData decompress(const CompressData& data);
  virtual std::string getId();
};

#endif