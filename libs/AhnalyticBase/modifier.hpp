#ifndef modifier_hpp__
#define modifier_hpp__

#include "compressData.hpp"
#include "export.hpp"

#include <string>
#include <vector>

class DLLEXPORT ModifierI
{
public:
  virtual ~ModifierI() {};
  virtual CompressData modify(const CompressData& data) = 0;
  virtual CompressData unmodify(const CompressData& data) = 0;
  virtual std::string getId() = 0;

private:
protected:
};

#endif