// -*- mode: cpp; mode: fold -*-
// Description								/*{{{*/
/* ######################################################################

   History - Parse history logs to structured data.

   ##################################################################### */
									/*}}}*/
#ifndef APTPKG_HISTORY_H
#define APTPKG_HISTORY_H

#include <apt-pkg/fileutl.h>
#include <apt-pkg/macros.h>
#include <apt-pkg/tagfile.h>

#include <array>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace APT::History
{

enum class Kind
{
   INSTALL,
   REINSTALL,
   UPGRADE,
   DOWNGRADE,
   REMOVE,
   PURGE,
   FILLER // helper for iteration, always last
};

struct Change
{
   Kind kind;
   std::string package;
   std::string currentVersion;
   std::string candidateVersion;
   bool automatic = false;

   private:
   void *d; // pointer for future extension;
};

struct Entry
{
   // Strings instead of string_view to avoid reference errors
   std::string startDate;
   std::string endDate;
   std::string cmdLine;
   std::string comment;
   std::string error;
   std::string requestingUser;
   std::map<Kind, std::vector<Change>> changeMap;
};

// History is defined as the collection of entries in the history log(s).
typedef std::vector<Entry> HistoryBuffer;

struct KindMapping
{
   Kind kind;
   std::string_view name;
};

// faster than a map, but order has to be equal to enum definition order
static constexpr std::array<KindMapping, static_cast<size_t>(Kind::FILLER)> KIND_MAPPINGS = {{{Kind::INSTALL, "Install"}, {Kind::REINSTALL, "Reinstall"}, {Kind::UPGRADE, "Upgrade"}, {Kind::DOWNGRADE, "Downgrade"}, {Kind::REMOVE, "Remove"}, {Kind::PURGE, "Purge"}}};

static constexpr std::string_view kind_to_string(const Kind &kind)
{
   size_t idx = static_cast<size_t>(kind);
   return idx < KIND_MAPPINGS.size() ? KIND_MAPPINGS[idx].name : "Undefined";
}

static constexpr std::optional<Kind> string_to_kind(const std::string &s)
{
   for (const auto &mapping : KIND_MAPPINGS)
      if (mapping.name == s)
	 return mapping.kind;

   return std::nullopt;
}

// ParseSection - Take a tag section and parse it as a
// history log entry.
APT_PUBLIC Entry ParseSection(const pkgTagSection &section);
// ParseFile - Take a file descriptor and parse it as a history
// log to the given buffer.
//
// NOTE: Caller is responsible for closing the file descriptor.
APT_PUBLIC bool ParseFile(FileFd &fd, HistoryBuffer &buf);
// ParseLogDir - Parse the apt history log directory to the buffer.
APT_PUBLIC bool ParseLogDir(HistoryBuffer &buf);
} // namespace APT::History

#endif
