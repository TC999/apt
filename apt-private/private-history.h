#ifndef APT_PRIVATE_HISTORY_H
#define APT_PRIVATE_HISTORY_H

#include <apt-pkg/cmndline.h>
#include <apt-pkg/history.h>
#include <apt-pkg/macros.h>
#include <apt-pkg/tagfile.h>

#include <array>
#include <map>
#include <optional>
#include <string_view>
#include <vector>

using namespace History;

APT_PUBLIC bool DoHistoryList(CommandLine &Cmd);
APT_PUBLIC bool DoHistoryInfo(CommandLine &Cmd);

// FillHistoryBuffer - Read the history log directory and fill the given
// buffer with all entries. Entries are sorted by date (early to later).
// Return true if successful.
bool FillHistoryBuffer(HistoryBuffer &history_buf);

// ShortenCommand - Take a command and shorten it such that it adheres
// to the given maximum length.
std::string ShortenCommand(const std::string &cmd, const size_t max_len);

// GetActionString - Take a history entry and construct the string
// corresponding to the actions performed. Multpile actions are
// alphabetically grouped such as:
// Install, Remove, and Reinstall -> I,R,rI
//
// Shorthand legend:
// Install   -> I
// Reinstall -> rI
// Upgrade   -> U
// Downgrade -> D
// Remove    -> R
// Purge     -> P
//
std::string GetActionString(const HistoryEntry &entry);

// SplitPackagesInContent - Take the content of an action and split the
// assiocated packages as an array of strings.
std::vector<std::string> SplitPackagesInContent(const std::string &content);

// PrintHistory - Take a history vector and print it.
void PrintHistoryVector(const HistoryBuffer history_buf, int column_width = 15);

// PrintPackageEvent - Take a package and action string and print the
// event for that package.
// Example:
//  "package (0.1.0, 0.2.0)" and "Upgrade" -> "Upgrade package (0.1.0 -> 0.2.0)"
//  "package (0.1.0)" and "Install" -> "Install package (0.1.0)"
void PrintPackageEvent(const std::string &package,
		       const std::string &action_string);

// PrintDetailedEntry - Take a history buffer and print the detailed
// transaction details for the given transaction id.
void PrintDetailedEntry(const HistoryBuffer &history_buf, const size_t id);
#endif
