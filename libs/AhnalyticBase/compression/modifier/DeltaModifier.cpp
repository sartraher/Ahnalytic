#include "DeltaModifier.hpp"

DeltaModifier::DeltaModifier()
{
}

CompressData DeltaModifier::modify(const CompressData& data)
{
  ahn::vector<char> inData = data.getCharData(CompressData::Auto);
  ahn::vector<char> ret = data.getCharData(CompressData::Auto);
  ret[0] = inData[0];
  for (size_t index = 1; index < inData.size(); index++)
    ret[index] = inData[index] - inData[index - 1];
  return ret;
}

CompressData DeltaModifier::unmodify(const CompressData& data)
{
  ahn::vector<char> inData = data.getCharData(CompressData::Auto);
  ahn::vector<char> ret = data.getCharData(CompressData::Auto);
  ret[0] = inData[0];
  for (size_t index = 1; index < inData.size(); index++)
    ret[index] = inData[index] + ret[index - 1];
  return ret;
}

std::string DeltaModifier::getId()
{
  return "delta";
}