#ifndef noneModifier_hpp__
#define noneModifier_hpp__

#include "modifier.hpp"

class NoneModifier : public ModifierI
{
public:
  NoneModifier() = default;
  virtual CompressData modify(const CompressData& data)
  {
    return std::move(data);
  }
  virtual CompressData unmodify(const CompressData& data)
  {
    return std::move(data);
  }
  virtual std::string getId()
  {
    return "none";
  }
};

#endif