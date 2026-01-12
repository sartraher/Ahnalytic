#include "DeltaModifier.hpp"

DeltaModifier::DeltaModifier()
{
}

CompressData DeltaModifier::modify(const CompressData& data)
{
  std::vector<char> inData = data.getCharData();
  std::vector<char> ret = data.getCharData();
  ret[0] = inData[0];
  for (size_t index = 1; index < inData.size(); index++)
    ret[index] = inData[index] - inData[index - 1];
  return ret;
}

CompressData DeltaModifier::unmodify(const CompressData& data)
{
  std::vector<char> inData = data.getCharData();
  std::vector<char> ret = data.getCharData();
  ret[0] = inData[0];
  for (size_t index = 1; index < inData.size(); index++)
    ret[index] = inData[index] + ret[index - 1];
  return ret;
}

std::string DeltaModifier::getId()
{
  return "delta";
}