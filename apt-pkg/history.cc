// -*- mode: cpp; mode: fold -*-
// Description								/*{{{*/
/* ######################################################################
   Set of functions for parsing the history log
   ##################################################################### */
									/*}}}*/
// Include Files							/*{{{*/

#include <config.h>

#include <apt-pkg/configuration.h>
#include <apt-pkg/error.h>
#include <apt-pkg/fileutl.h>
#include <apt-pkg/history.h>
#include <apt-pkg/tagfile.h>

#include <regex>
#include <unordered_map>

#include <glob.h>
#include <apti18n.h> // for coloring

namespace APT::History
{

// Check if the string is a version
inline bool is_version(const std::string &s)
{
   static const std::regex version_regex(R"(\d[\w\.\+\-\~:]*)");
   return std::regex_match(s, version_regex);
}

static Change ParsePackageEvent(const std::string &event)
{
   Change change{};
   auto openParen = event.find(" (");
   if (openParen == std::string::npos)
      return change;

   change.package = event.substr(0, openParen);

   auto closeParen = event.find(')', openParen);
   if (closeParen == std::string::npos)
      return change;

   std::string versionStr = event.substr(openParen + 2, closeParen - openParen - 2);

   auto commaPos = versionStr.find(", ");
   if (commaPos == std::string::npos)
   {
      change.currentVersion = versionStr;
      return change;
   }
   change.currentVersion = versionStr.substr(0, commaPos);
   std::string candidate = versionStr.substr(commaPos + 2);
   if (is_version(candidate))
      change.candidateVersion = candidate;
   else
      change.automatic = true;

   return change;
}

static std::vector<std::string> SplitPackagesInContent(const std::string &content)
{
   std::vector<std::string> result;
   std::size_t start = 0;

   if (content.length() == 0)
      return result;

   // Split at "), " but keep the parenthesis.
   while (true)
   {
      std::size_t pos = content.find("), ", start);
      if (pos == std::string::npos)
      {
	 result.push_back(content.substr(start));
	 break;
      }
      result.push_back(content.substr(start, pos - start + 1));
      // Increment by 3 since 3 == len("), ")
      start = pos + 3;
   }
   return result;
}

Entry ParseSection(
   const pkgTagSection &section)
{
   Entry entry{};
   entry.startDate = section.Find("Start-Date");
   entry.endDate = section.Find("End-Date");
   entry.cmdLine = section.Find("Commandline");
   entry.requestingUser = section.Find("Requested-By");
   entry.comment = section.Find("Comment");
   entry.error = section.Find("Error");

   std::string content = "";
   for (const auto &mapping : KIND_MAPPINGS)
   {
      content = section.Find(mapping.name);
      if (content.empty())
	 continue;

      std::vector<std::string> package_events = SplitPackagesInContent(content);
      std::vector<Change> changes = {};
      for (auto event : package_events)
      {
	 Change change = ParsePackageEvent(event);
	 change.kind = mapping.kind;
	 changes.push_back(change);
      }
      // Changed packages should be in order
      std::sort(changes.begin(), changes.end(), [](const Change &a, const Change &b)
		{ return a.package < b.package; });
      entry.changeMap[mapping.kind] = changes;
   }

   return entry;
}

bool ParseFile(FileFd &fd, HistoryBuffer &buf)
{
   pkgTagFile file(&fd, FileFd::ReadOnly);
   pkgTagSection tmpSection;
   while (file.Step(tmpSection))
   {
      buf.push_back(ParseSection(tmpSection));
   }
   return true;
}

bool ParseLogDir(HistoryBuffer &buf)
{
   std::string files = _config->FindFile("Dir::Log::History") + "*";
   const char *pattern = files.data();
   glob_t result;

   int ret = glob(pattern, GLOB_TILDE, nullptr, &result);
   if (ret != 0)
   {
      return false;
   }

   for (size_t i = 0; i < result.gl_pathc; ++i)
   {
      FileFd fd;
      if (not fd.Open(result.gl_pathv[i], FileFd::ReadOnly, FileFd::Extension))
	 return _error->Error(_("Could not open file %s"), result.gl_pathv[i]);
      if (not ParseFile(fd, buf))
	 return _error->Error(_("Could parse file %s"), result.gl_pathv[i]);
      if (not fd.Close())
	 return _error->Error(_("Could close file %s"), result.gl_pathv[i]);
   }

   // Sort entries by time
   std::sort(buf.begin(), buf.end(),
	     [](const Entry &a, const Entry &b)
	     {
		return a.startDate < b.startDate;
	     });

   return true;
}
} // namespace APT::History
