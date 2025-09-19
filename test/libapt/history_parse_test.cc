#include <config.h>

#include <apt-pkg/configuration.h>
#include <apt-pkg/fileutl.h>
#include <apt-pkg/history.h>
#include <apt-pkg/indexcopy.h>
#include <apt-pkg/tagfile.h>

#include "common.h"

#include "file-helpers.h"

TEST(HistoryParseTest, SectionToEntry)
{
   FileFd fd;
   const char *entry_section =
      "Start-Date: 2025-09-01  15:22:56\n"
      "Commandline: apt install rust-coreutils\n"
      "Requested-By: user (1000)\n"
      "Error: An error occurred\n"
      "Comment: This is a comment\n"
      "Install: rust-coreutils:amd64 (0.1.0+git20250813.4af2a84-0ubuntu2)\n"
      "End-Date: 2025-09-01  15:22:57";

   openTemporaryFile("packagesection", fd, entry_section);

   pkgTagFile tfile(&fd);
   pkgTagSection section;
   ASSERT_TRUE(tfile.Step(section));

   History::HistoryEntry entry = History::ParseSection(section);
   EXPECT_EQ("2025-09-01  15:22:56", entry.start_date);
   EXPECT_EQ("2025-09-01  15:22:57", entry.end_date);
   EXPECT_EQ("apt install rust-coreutils", entry.cmd_line);
   EXPECT_EQ("user (1000)", entry.requesting_user);
   EXPECT_EQ("An error occurred", entry.error);
   EXPECT_EQ("This is a comment", entry.comment);
   EXPECT_EQ("rust-coreutils:amd64 (0.1.0+git20250813.4af2a84-0ubuntu2)",
	     entry.action_content_map[History::HistoryAction::INSTALL]);
}

TEST(HistoryParseTest, EmptyOptionalFields)
{
   FileFd fd;
   const char *entry_section =
      "Start-Date: 2025-09-01  15:22:56\n"
      "Commandline: apt install rust-coreutils\n"
      "Requested-By: user (1000)\n"
      "Install: rust-coreutils:amd64 (0.1.0+git20250813.4af2a84-0ubuntu2)\n"
      "End-Date: 2025-09-01  15:22:57";
   openTemporaryFile("packagesection", fd, entry_section);
   pkgTagFile tfile(&fd);
   pkgTagSection section;
   ASSERT_TRUE(tfile.Step(section));

   History::HistoryEntry entry = History::ParseSection(section);
   EXPECT_EQ("", entry.error);
   EXPECT_EQ("", entry.comment);
}

TEST(HistoryParseTest, MultipleActions)
{
   FileFd fd;
   const char *entry_section =
      "Start-Date: 2025-09-01  15:22:56\n"
      "Commandline: apt install rust-coreutils\n"
      "Requested-By: user (1000)\n"
      "Error: An error occurred\n"
      "Comment: This is a comment\n"
      "Install: rust-coreutils:amd64 (0.1.0+git20250813.4af2a84-0ubuntu2)\n"
      "Remove: rust-coreutils:amd64 (0.1.0+git20250813.4af2a84-0ubuntu2)\n"
      "Downgrade: rust-coreutils:amd64 (0.1.0, 0.0.0)\n"
      "Reinstall: rust-coreutils:amd64 (0.1.0+git20250813.4af2a84-0ubuntu2)\n"
      "Upgrade: rust-coreutils:amd64 (0.1.0, 0.2.0)\n"
      "Purge: rust-coreutils:amd64 (0.1.0+git20250813.4af2a84-0ubuntu2)\n"
      "End-Date: 2025-09-01  15:22:57";

   openTemporaryFile("packagesection", fd, entry_section);

   pkgTagFile tfile(&fd);
   pkgTagSection section;
   ASSERT_TRUE(tfile.Step(section));

   History::HistoryEntry entry = History::ParseSection(section);
   EXPECT_EQ("rust-coreutils:amd64 (0.1.0+git20250813.4af2a84-0ubuntu2)",
	     entry.action_content_map[History::HistoryAction::INSTALL]);
   EXPECT_EQ("rust-coreutils:amd64 (0.1.0+git20250813.4af2a84-0ubuntu2)",
	     entry.action_content_map[History::HistoryAction::REMOVE]);
   EXPECT_EQ("rust-coreutils:amd64 (0.1.0, 0.0.0)",
	     entry.action_content_map[History::HistoryAction::DOWNGRADE]);
   EXPECT_EQ("rust-coreutils:amd64 (0.1.0+git20250813.4af2a84-0ubuntu2)",
	     entry.action_content_map[History::HistoryAction::REINSTALL]);
   EXPECT_EQ("rust-coreutils:amd64 (0.1.0, 0.2.0)",
	     entry.action_content_map[History::HistoryAction::UPGRADE]);
   EXPECT_EQ("rust-coreutils:amd64 (0.1.0+git20250813.4af2a84-0ubuntu2)",
	     entry.action_content_map[History::HistoryAction::PURGE]);
}
