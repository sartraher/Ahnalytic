#include "AhnalyticFile.hpp"

#include "AhnalyticBase/file/IniReader.hpp"

#include "magic_enum/magic_enum.hpp"

struct AhnalyticFilePrivate
{
  IniReader* iniReader = nullptr;

  AhnalyticFileTypeE type = AhnalyticFileTypeE::Content;

  // ThirdParty
  ThirdPartyConfig thirdPartyConfig;

  std::vector<CVEConfig> cveConfigs;

  // Content
  std::vector<ResultFilter> resultFilter;
};

AhnalyticFile::AhnalyticFile(const std::string& path) : priv(new AhnalyticFilePrivate())
{
  priv->iniReader = new IniReader(path);
  load();
}

AhnalyticFile::~AhnalyticFile()
{
  delete priv->iniReader;
}

void AhnalyticFile::load()
{
  std::vector<std::string> blocks = priv->iniReader->getBlocks();
  for (const std::string& block : blocks)
  {
    std::string type = priv->iniReader->getValue("type", block, "Content");

    if (type == "Content")
    {
      priv->type = AhnalyticFileTypeE::Content;

      ResultFilter filter;
      filter.dbFile = priv->iniReader->getValue("dbFile", block, "");
      filter.searchFile = priv->iniReader->getValue("searchFile", block, "");
      filter.reason = ResultFilter::FalsePositive;

      std::optional<ResultFilter::FilterReasonE> optionalReason =
          magic_enum::enum_cast<ResultFilter::FilterReasonE>(priv->iniReader->getValue("reason", block, "FalsePositive"));

      if (optionalReason.has_value())
        filter.reason = optionalReason.value();

      filter.comment = priv->iniReader->getValue("comment", block, "");

      priv->resultFilter.push_back(filter);
    }
    else if (type == "3rdParty")
    {
      priv->type = AhnalyticFileTypeE::ThirdParty;

      priv->thirdPartyConfig.vendor = priv->iniReader->getValue("vendor", block, "");
      priv->thirdPartyConfig.product = priv->iniReader->getValue("product", block, "");
      priv->thirdPartyConfig.copyright = priv->iniReader->getValue("copyright", block, "");
      priv->thirdPartyConfig.version = priv->iniReader->getValue("version", block, "");
      priv->thirdPartyConfig.displayName = priv->iniReader->getValue("displayName", block, "");
      priv->thirdPartyConfig.displayVersion = priv->iniReader->getValue("displayVersion", block, "");
      priv->thirdPartyConfig.url = priv->iniReader->getValue("url", block, "");
      priv->thirdPartyConfig.date = priv->iniReader->getValue("date", block, "");
    }
    else if (type == "CVE")
    {
      CVEConfig cveConfig;

      cveConfig.id = priv->iniReader->getValue("id", block, "");
      cveConfig.comment = priv->iniReader->getValue("comment", block, "");

      cveConfig.status = CVEConfig::Open;
      std::optional<CVEConfig::CVEStatusE> optionalStatus = magic_enum::enum_cast<CVEConfig::CVEStatusE>(priv->iniReader->getValue("status", block, "Open"));

      if (optionalStatus.has_value())
        cveConfig.status = optionalStatus.value();

      priv->cveConfigs.push_back(cveConfig);
    }
    else if (type == "Ignore")
    {
      priv->type = AhnalyticFileTypeE::Ignore;
    }
  }
}

AhnalyticFileTypeE AhnalyticFile::getType() const
{
  return priv->type;
}

ThirdPartyConfig AhnalyticFile::getThirdPartyConfig() const
{
  return priv->thirdPartyConfig;
}

std::vector<ResultFilter> AhnalyticFile::getResultFilters() const
{
  return priv->resultFilter;
}

std::vector<CVEConfig> AhnalyticFile::getCVEConfigs() const
{
  return priv->cveConfigs;
}