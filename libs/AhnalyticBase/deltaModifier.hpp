#ifndef deltaModifier_hpp__
#define deltaModifier_hpp__

#include "modifier.hpp"

class DeltaModifier : public ModifierI
{
public:
  DeltaModifier();
  virtual ~DeltaModifier() = default;
  virtual CompressData modify(const CompressData& data);
  virtual CompressData unmodify(const CompressData& data);
  virtual std::string getId();
};

#endif