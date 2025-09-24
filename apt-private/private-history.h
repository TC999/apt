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

APT_PUBLIC bool DoHistoryList(CommandLine &Cmd);
APT_PUBLIC bool DoHistoryInfo(CommandLine &Cmd);
#endif
