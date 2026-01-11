
#define HAVE_STDBOOL_H 1

// https://stackoverflow.com/questions/23232864/how-to-use-lzma-sdk-in-c
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <memory>
#include "lzma_encoder.h"
#include "lzma_decoder.h"
#include "lzma.h"


static SRes Encode(FILE* inFile, FILE* outFile, char* rs)
{
  CLzmaEncHandle enc;
  SRes res;
  CFileSeqInStream inStream;
  CFileSeqOutStream outStream;
  CLzmaEncProps props;

  enc = LzmaEnc_Create(&g_Alloc);
  if (enc == 0)
    return SZ_ERROR_MEM;

  inStream.funcTable.Read = MyRead;
  inStream.file = inFile;
  outStream.funcTable.Write = MyWrite;
  outStream.file = outFile;

  LzmaEncProps_Init(&props);
  res = LzmaEnc_SetProps(enc, &props);

  if (res == SZ_OK)
  {
    Byte header[LZMA_PROPS_SIZE + 8];
    size_t headerSize = LZMA_PROPS_SIZE;
    UInt64 fileSize;
    int i;

    res = LzmaEnc_WriteProperties(enc, header, &headerSize);
    fileSize = MyGetFileLength(inFile);
    for (i = 0; i < 8; i++)
      header[headerSize++] = (Byte)(fileSize >> (8 * i));
    if (!MyWriteFileAndCheck(outFile, header, headerSize))
      return PrintError(rs, "writing error");

    if (res == SZ_OK)
      res = LzmaEnc_Encode(enc, &outStream.funcTable, &inStream.funcTable,
        NULL, &g_Alloc, &g_Alloc);
  }
  LzmaEnc_Destroy(enc, &g_Alloc, &g_Alloc);
  return res;
}