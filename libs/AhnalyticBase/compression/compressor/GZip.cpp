#include "GZip.hpp"

/*
#include "deflate.h"
#include "zlib.h"

void compress_memory(void* in_data, size_t in_data_size, std::vector<char>& out_data)
{
  std::vector<char> buffer;

  const size_t BUFSIZE = 128 * 1024;
  uint8_t temp_buffer[BUFSIZE];

  z_stream strm;
  strm.zalloc = 0;
  strm.zfree = 0;
  strm.next_in = reinterpret_cast<uint8_t*>(in_data);
  strm.avail_in = in_data_size;
  strm.next_out = temp_buffer;
  strm.avail_out = BUFSIZE;

  deflateInit(&strm, Z_BEST_COMPRESSION);

  while (strm.avail_in != 0)
  {
    int res = deflate(&strm, Z_NO_FLUSH);
    if (strm.avail_out == 0)
    {
      buffer.insert(buffer.end(), temp_buffer, temp_buffer + BUFSIZE);
      strm.next_out = temp_buffer;
      strm.avail_out = BUFSIZE;
    }
  }

  int deflate_res = Z_OK;
  while (deflate_res == Z_OK)
  {
    if (strm.avail_out == 0)
    {
      buffer.insert(buffer.end(), temp_buffer, temp_buffer + BUFSIZE);
      strm.next_out = temp_buffer;
      strm.avail_out = BUFSIZE;
    }
    deflate_res = deflate(&strm, Z_FINISH);
  }

  buffer.insert(buffer.end(), temp_buffer, temp_buffer + BUFSIZE - strm.avail_out);
  deflateEnd(&strm);

  out_data.swap(buffer);
}

void decompress_gzip(const std::vector<char>& in_data, std::vector<char>& out_data)
{
  if (in_data.empty())
  {
    out_data.clear();
    return;
  }

  z_stream strm{};
  strm.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(in_data.data()));
  strm.avail_in = in_data.size();

  if (inflateInit2(&strm, 15 + 16) != Z_OK)
  {
  }

  const size_t CHUNK = 128 * 1024;
  std::vector<char> buffer(CHUNK);

  while (true)
  {
    strm.next_out = reinterpret_cast<Bytef*>(buffer.data());
    strm.avail_out = buffer.size();

    int ret = inflate(&strm, Z_NO_FLUSH);
    if (ret == Z_STREAM_ERROR || ret == Z_DATA_ERROR)
    {
    }

    size_t have = buffer.size() - strm.avail_out;
    out_data.insert(out_data.end(), buffer.data(), buffer.data() + have);

    if (ret == Z_STREAM_END)
      break;
  }

  inflateEnd(&strm);
}

GZipCompressor::GZipCompressor()
{
}

CompressData GZipCompressor::compress(const CompressData& data)
{
  std::vector<char> ret;
  std::vector<char> inData = data.getCharData();
  compress_memory(inData.data(), inData.size(), ret);
  return ret;
}

CompressData GZipCompressor::decompress(const CompressData& data)
{
  std::vector<char> ret;
  decompress_gzip(data.getCharData(), ret);
  return ret;
}

std::string GZipCompressor::getId()
{
  return "gzip";
}
*/