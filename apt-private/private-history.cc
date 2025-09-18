// Include files
#include <config.h>

#include <apt-pkg/cmndline.h>
#include <apt-pkg/configuration.h>
#include <apt-pkg/error.h>
#include <apt-pkg/fileutl.h>
#include <apt-pkg/history.h>
#include <apt-pkg/tagfile.h>

#include <apt-private/private-history.h>

#include <iomanip>
#include <iostream>
#include <regex>
#include <string>

#include <glob.h>
#include <apti18n.h> // for coloring
		     /*}}}*/

using namespace History;

std::string ShortenCommand(const std::string &cmd, const std::size_t max_len)
{
   std::string shortened_cmd = cmd;
   if (shortened_cmd.starts_with("apt "))
      shortened_cmd = shortened_cmd.substr(4);
   if (shortened_cmd.length() > max_len - 3)
      return shortened_cmd.substr(0, max_len - 4) + "...";
   return shortened_cmd;
}

std::string GetActionString(const HistoryEntry &entry)
{
   if (entry.action_content_map.size() == 1)
      return entry.action_content_map.begin()->first;

   std::string action_group = "";
   for (const auto &[action_string, _] : entry.action_content_map)
   {
      switch (string_to_action(action_string))
      {
      case HistoryAction::INSTALL:
	 action_group += "I";
	 break;
      case HistoryAction::REINSTALL:
	 action_group += "rI";
	 break;
      case HistoryAction::UPGRADE:
	 action_group += "U";
	 break;
      case HistoryAction::DOWNGRADE:
	 action_group += "D";
	 break;
      case HistoryAction::REMOVE:
	 action_group += "R";
	 break;
      case HistoryAction::PURGE:
	 action_group += "P";
	 break;
      default:
	 action_group += "INVALID";
      }
      action_group += ",";
   }
   // remove trailing ","
   action_group.pop_back();
   return action_group;
}

std::vector<std::string> SplitPackagesInContent(const std::string &content)
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

void PrintHistoryVector(const HistoryBuffer buf, int column_width)
{
   // Calculate the width of the ID column, esentially the number of digits
   int id = 0;
   size_t size_frac = buf.size();
   int id_width = 2;
   while (size_frac)
   {
      size_frac /= 10;
      id_width++;
   }

// Print headers
#define WRITE_ENTRY(ENTRY, WIDTH)                          \
   {                                                       \
      std::cout << std::left << std::setw(WIDTH) << ENTRY; \
   }
   WRITE_ENTRY("ID", id_width);
   WRITE_ENTRY("Command line", column_width);
   WRITE_ENTRY("Date and Time", 23); // dates are 20 in length
   WRITE_ENTRY("Action", 10);	     // 9 chars longest action type
   WRITE_ENTRY("Changes", column_width);
   std::cout << "\n\n";

   // Each entry corresponds to a row
   for (auto entry : buf)
   {
      WRITE_ENTRY(id, id_width);
      WRITE_ENTRY(ShortenCommand(entry.cmd_line, column_width), column_width);
      WRITE_ENTRY(entry.start_date, 23);
      // If there are multiple actions, we want to group them
      WRITE_ENTRY(GetActionString(entry), 10);

      // Count the number of packages changed
      size_t changes = 0;
      for (const auto &[_, content] : entry.action_content_map)
	 changes += SplitPackagesInContent(content).size();
      WRITE_ENTRY(changes, column_width);
      std::cout << "\n";
      id++;
   }
#undef WRITE_ENTRY
}

bool FillHistoryBuffer(HistoryBuffer &buf)
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
      if (!TryParseFile(result.gl_pathv[i], buf))
	 return false;
   }

   // Sort entries by time
   std::sort(buf.begin(), buf.end(),
	     [](const HistoryEntry &a,
		const HistoryEntry &b)
	     {
		return a.start_date < b.start_date;
	     });

   return true;
}

// Check if the string is a version
inline bool is_version(const std::string &s)
{
   static const std::regex version_regex(R"(\d[\w\.\+\-\~:]*)");
   return std::regex_match(s, version_regex);
}

void PrintPackageEvent(const std::string &package, const std::string &action_string)
{
   // Pattern to match package:arch (version)/(version,automatic)/(v1,v2)
   std::regex pattern(R"(([\w\-\.:]+) \(([\w\.\+\-\~:]+)(?:, ([\w\.\+\-\~:]+))?\))");
   std::smatch matches;

   if (std::regex_search(package, matches, pattern))
   {
      std::string package = matches[1];
      std::string v1 = matches[2];
      std::string v2;

      if (matches[3].matched && is_version(matches[3]))
	 v2 = matches[3];

      std::cout << "    " << action_string << " " << package << " (" << v1;
      if (!v2.empty())
	 std::cout << " -> " << v2;
      std::cout << ")";
   }
}

void PrintDetailedEntry(const HistoryBuffer &buf, const size_t id)
{
   HistoryEntry entry = buf[id];
   std::cout << "Transaction ID : " << id << "\n";
   std::cout << "Start time     : " << entry.start_date << "\n";
   std::cout << "End time       : " << entry.end_date << "\n";
   std::cout << "Requested by   : " << entry.requesting_user << "\n";
   std::cout << "Command line   : " << entry.cmd_line << "\n";
   if (!entry.error.empty())
   {
      std::cout << _config->Find("APT::Color::Red") << _("Error          : ");
      std::cout << entry.error << _config->Find("APT::Color::Neutral") << "\n";
   }
   if (!entry.comment.empty())
      std::cout << "Comment        : " << entry.comment << "\n";

   // For each performed action, print what it did to each package
   std::cout << "Packages changed: " << "\n";
   for (const auto &[action_string, content] : entry.action_content_map)
   {
      std::vector<std::string> package_events = SplitPackagesInContent(content);
      std::sort(package_events.begin(), package_events.end());
      for (const auto &package : package_events)
      {
	 PrintPackageEvent(package, action_string);
	 std::cout << "\n";
      }
      std::cout << "\n";
   }
}

bool DoHistoryList(CommandLine &Cmd)
{
   HistoryBuffer buf = {};

   if (!FillHistoryBuffer(buf))
      return false;
   PrintHistoryVector(buf, 25);

   return true;
}

bool DoHistoryInfo(CommandLine &Cmd)
{
   HistoryBuffer buf = {};
   if (!FillHistoryBuffer(buf))
      return false;

   // This is ugly, check that the correct number of arguments where supplied
   size_t id = std::stoi(*(Cmd.FileList + 1));
   if (buf.size() <= id)
   {
      std::cout << "Invalid transaction ID: " << id << ", when history has ";
      std::cout << buf.size();
      std::cout << (buf.size() == 1 ? " entry!" : " entries!") << std::endl;
      return false;
   }
   PrintDetailedEntry(buf, id);

   return true;
}
