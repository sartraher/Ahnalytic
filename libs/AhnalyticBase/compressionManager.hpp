#ifndef compresssionManager_hpp__
#define compresssionManager_hpp__

#include "export.hpp"

#include "compressData.hpp"
#include "compressor.hpp"
#include "modifier.hpp"

#include <string>
#include <unordered_map>
#include <vector>

class Diagnostic;

class DLLEXPORT CompressionManager
{
public:
  CompressionManager();
  ~CompressionManager();

  CompressData compress(const CompressData& data, Diagnostic* dia = nullptr, std::vector<ModAlgosE> modFilter = std::vector<ModAlgosE>(),
                        std::vector<CompressionAlgosE> cmpFilter = std::vector<CompressionAlgosE>(), bool force = false);
  // std::vector<uint32_t> compress(std::vector<uint32_t> data, Diagnostic* dia = nullptr, std::vector<ModAlgosE> modFilter = std::vector<ModAlgosE>(),
  // std::vector<CompressionAlgosE> cmpFilter = std::vector<CompressionAlgosE>()); std::vector<uint16_t> compress(std::vector<uint16_t> data, Diagnostic* dia =
  // nullptr, std::vector<ModAlgosE> modFilter = std::vector<ModAlgosE>(), std::vector<CompressionAlgosE> cmpFilter = std::vector<CompressionAlgosE>());

  CompressData decompress(const CompressData& input, Diagnostic* dia = nullptr);
  // std::vector<uint32_t> decompress(const std::vector<uint32_t>& input, Diagnostic* dia);
  // std::vector<uint16_t> decompress(const std::vector<uint16_t>& input, Diagnostic* dia);

  std::string getModifierName(ModAlgosE id);
  std::string getCompressorName(CompressionAlgosE id);

private:
  std::unordered_map<ModAlgosE, ModifierI*> modifiers;
  std::unordered_map<CompressionAlgosE, CompressorI*> compressors;

protected:
  void registerModifier(ModAlgosE algo, ModifierI* modifier);
  void registerCompressor(CompressionAlgosE algo, CompressorI* modifier);
};

#endif