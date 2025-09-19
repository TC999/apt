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

namespace History
{
HistoryEntry ParseSection(
   const pkgTagSection &section)
{
   HistoryEntry entry{};
   entry.start_date = section.Find("Start-Date");
   entry.end_date = section.Find("End-Date");
   entry.cmd_line = section.Find("Commandline");
   entry.requesting_user = section.Find("Requested-By");
   entry.comment = section.Find("Comment");
   entry.error = section.Find("Error");

   std::string content = "";
   for (auto mapping : HISTORY_ACTION_MAPPINGS)
   {
      content = section.Find(mapping.name);
      // Add action content if it was performed
      if (!content.empty())
	 entry.action_content_map[mapping.action] = content;
   }

   return entry;
}

bool TryParseFile(const std::string &filename, HistoryBuffer &buf)
{
   // FileFd cannot be passed as an argument, so make it from here.
   FileFd log_fd;
   if (!log_fd.Open(filename, FileFd::ReadOnly, FileFd::Extension))
      return false;
   pkgTagFile file(&log_fd, FileFd::ReadOnly);

   pkgTagSection tmp_section;
   while (file.Step(tmp_section))
   {
      buf.push_back(ParseSection(tmp_section));
   }

   if (!log_fd.Close())
      return false;

   return true;
}
} // namespace History
