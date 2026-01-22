#ifndef AhnalyticFile_hpp__
#define AhnalyticFile_hpp__

#include "AhnalyticBase/Export.hpp"

#include <string>
#include <vector>

struct AhnalyticFilePrivate;

struct CVEConfig
{
  enum CVEStatusE
  {
    Close,
    Open
  };

  std::string id;
  std::string comment;
  CVEStatusE status = CVEStatusE::Open;
};

struct ResultFilter
{
  enum FilterReasonE
  {
    FalsePositive,
    NoCreativeValue,
    AllowedByAuthor,
    Other
  };

  std::string dbFile;
  std::string searchFile;

  FilterReasonE reason = FilterReasonE::FalsePositive;
  std::string comment;
};

struct ThirdPartyConfig
{
  std::string vendor;
  std::string product;
  std::string copyright;
  std::string version;
  std::string displayName;
  std::string displayVersion;
  std::string url;
  std::string date;
};

enum class AhnalyticFileTypeE
{
  Ignore,
  ThirdParty,
  Content
};

class DLLEXPORT AhnalyticFile
{
public:
  AhnalyticFile(const std::string& path);
  ~AhnalyticFile();

  AhnalyticFileTypeE getType() const;
  ThirdPartyConfig getThirdPartyConfig() const;
  std::vector<ResultFilter> getResultFilters() const;
  std::vector<CVEConfig> getCVEConfigs() const;

private:
  AhnalyticFilePrivate* priv = nullptr;

protected:
  void load();
};

#endif