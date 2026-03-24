#ifndef compresssionManager_hpp__
#define compresssionManager_hpp__

#include "AhnalyticBase/Export.hpp"

#include "AhnalyticBase/compression/CompressData.hpp"
#include "AhnalyticBase/compression/Compressor.hpp"
#include "AhnalyticBase/compression/Modifier.hpp"

#include <string>
#include <unordered_map>
#include <vector>

class Diagnostic;

class DLLEXPORT CompressionManager
{
public:
  CompressionManager();
  ~CompressionManager();

  CompressData compress(const CompressData& data, Diagnostic* dia = nullptr, ahn::vector<ModAlgosE> modFilter = ahn::vector<ModAlgosE>(),
                        ahn::vector<CompressionAlgosE> cmpFilter = ahn::vector<CompressionAlgosE>(), bool force = false);

  CompressData decompress(const CompressData& input, Diagnostic* dia = nullptr);

  std::string getModifierName(ModAlgosE id);
  std::string getCompressorName(CompressionAlgosE id);

private:
  ahn::map<ModAlgosE, ModifierI*> modifiers;
  ahn::map<CompressionAlgosE, CompressorI*> compressors;

protected:
  void registerModifier(ModAlgosE algo, ModifierI* modifier);
  void registerCompressor(CompressionAlgosE algo, CompressorI* modifier);
};

#endif