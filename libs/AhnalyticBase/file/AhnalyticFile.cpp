#include "AhnalyticFile.hpp"

#include "AhnalyticBase/file/IniReader.hpp"

#include "magic_enum/magic_enum.hpp"

struct AhnalyticFilePrivate
{
  std::shared_ptr<IniReader> iniReader;

  AhnalyticFileTypeE type = AhnalyticFileTypeE::Content;

  std::string target;

  // ThirdParty
  ThirdPartyConfig thirdPartyConfig;

  ahn::vector<CVEConfig> cveConfigs;

  // Content
  ahn::vector<ResultFilter> resultFilter;
};

AhnalyticFile::AhnalyticFile() : priv(new AhnalyticFilePrivate())
{
}

AhnalyticFile::AhnalyticFile(const std::string& path) : priv(new AhnalyticFilePrivate())
{
  priv->iniReader = std::make_shared<IniReader>(path);
  load();
}

AhnalyticFile::AhnalyticFile(const AhnalyticFile& other)
{
  priv = new AhnalyticFilePrivate(*other.priv);
}

AhnalyticFile::~AhnalyticFile()
{
  delete priv;
}

void AhnalyticFile::load()
{
  ahn::vector<std::string> blocks = priv->iniReader->getBlocks();
  for (const std::string& block : blocks)
  {
    std::string type = priv->iniReader->getValue("type", block, "Content");

    if (type == "Common")
    {
      priv->target = priv->iniReader->getValue("target", block, "");
    }
    else if (type == "Content")
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

std::string AhnalyticFile::getTarget() const
{
  return priv->target;
}

AhnalyticFileTypeE AhnalyticFile::getType() const
{
  return priv->type;
}

ThirdPartyConfig AhnalyticFile::getThirdPartyConfig() const
{
  return priv->thirdPartyConfig;
}

ahn::vector<ResultFilter> AhnalyticFile::getResultFilters() const
{
  return priv->resultFilter;
}

ahn::vector<CVEConfig> AhnalyticFile::getCVEConfigs() const
{
  return priv->cveConfigs;
}

nlohmann::json AhnalyticFile::getJson() const
{
  nlohmann::json j;
  j["type"] = (priv->type == AhnalyticFileTypeE::Content ? "Content" : (priv->type == AhnalyticFileTypeE::ThirdParty ? "3rdParty" : "Ignore"));
  j["target"] = priv->target;

  // ThirdParty
  if (priv->type == AhnalyticFileTypeE::ThirdParty)
  {
    j["thirdParty"] = {{"vendor", priv->thirdPartyConfig.vendor},
                       {"product", priv->thirdPartyConfig.product},
                       {"copyright", priv->thirdPartyConfig.copyright},
                       {"version", priv->thirdPartyConfig.version},
                       {"displayName", priv->thirdPartyConfig.displayName},
                       {"displayVersion", priv->thirdPartyConfig.displayVersion},
                       {"url", priv->thirdPartyConfig.url},
                       {"date", priv->thirdPartyConfig.date}};
  }

  // Content
  if (priv->type == AhnalyticFileTypeE::Content)
  {
    for (const auto& filter : priv->resultFilter)
    {
      j["resultFilters"].push_back(
          {{"dbFile", filter.dbFile}, {"searchFile", filter.searchFile}, {"reason", magic_enum::enum_name(filter.reason)}, {"comment", filter.comment}});
    }
  }

  // CVE
  for (const auto& cve : priv->cveConfigs)
  {
    j["cveConfigs"].push_back({{"id", cve.id}, {"comment", cve.comment}, {"status", magic_enum::enum_name(cve.status)}});
  }

  return j;
}