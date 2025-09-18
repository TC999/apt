// -*- mode: cpp; mode: fold -*-
// Description								/*{{{*/
/* ######################################################################

   History - Parse history logs to structured data.

   ##################################################################### */
									/*}}}*/
#ifndef APTPKG_HISTORY_H
#define APTPKG_HISTORY_H

#include <apt-pkg/macros.h>
#include <apt-pkg/tagfile.h>

#include <array>
#include <map>
#include <string>
#include <vector>

namespace History
{
struct HistoryEntry
{
   // Strings instead of string_view to avoid reference errors
   std::string start_date;
   std::string end_date;
   std::string cmd_line;
   std::string comment;
   std::string error;
   std::string requesting_user;
   std::map<std::string, std::string> action_content_map;
};
// History is defined as the collection of entries in the history log(s).
typedef std::vector<HistoryEntry> HistoryBuffer;

enum class HistoryAction
{
   INSTALL,
   REINSTALL,
   UPGRADE,
   DOWNGRADE,
   REMOVE,
   PURGE,
   FILLER // helper for iteration, always last
};

struct ActionMapping
{
   HistoryAction action;
   std::string_view name;
};

// faster than a map, but order has to be equal to enum definition order
static constexpr std::array<ActionMapping, static_cast<size_t>(HistoryAction::FILLER)> HISTORY_ACTION_MAPPINGS = {{{HistoryAction::INSTALL, "Install"}, {HistoryAction::REINSTALL, "Reinstall"}, {HistoryAction::UPGRADE, "Upgrade"}, {HistoryAction::DOWNGRADE, "Downgrade"}, {HistoryAction::REMOVE, "Remove"}, {HistoryAction::PURGE, "Purge"}}};

static constexpr std::string_view action_to_string(const HistoryAction &action)
{
   size_t idx = static_cast<size_t>(action);
   return idx < HISTORY_ACTION_MAPPINGS.size() ? HISTORY_ACTION_MAPPINGS[idx].name : "Undefined";
}

static constexpr HistoryAction string_to_action(const std::string &s)
{
   for (const auto &mapping : HISTORY_ACTION_MAPPINGS)
      if (mapping.name == s)
	 return mapping.action;

   // this is equivalent to an invalid string, consider optional type
   return HistoryAction::FILLER;
}

APT_PUBLIC HistoryEntry ParseSection(const pkgTagSection &section);
APT_PUBLIC bool TryParseFile(const std::string &filename, HistoryBuffer &buf);
} // namespace History

#endif
