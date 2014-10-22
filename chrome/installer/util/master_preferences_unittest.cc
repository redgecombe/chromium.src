// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Unit tests for master preferences related methods.

#include "base/files/file_util.h"
#include "base/memory/scoped_ptr.h"
#include "base/path_service.h"
#include "base/strings/stringprintf.h"
#include "base/values.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/pref_names.h"
#include "chrome/installer/util/master_preferences.h"
#include "chrome/installer/util/master_preferences_constants.h"
#include "chrome/installer/util/util_constants.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {
class MasterPreferencesTest : public testing::Test {
 protected:
  virtual void SetUp() {
    ASSERT_TRUE(base::CreateTemporaryFile(&prefs_file_));
  }

  virtual void TearDown() {
    EXPECT_TRUE(base::DeleteFile(prefs_file_, false));
  }

  const base::FilePath& prefs_file() const { return prefs_file_; }

 private:
  base::FilePath prefs_file_;
};

// Used to specify an expected value for a set boolean preference variable.
struct ExpectedBooleans {
  const char* name;
  bool expected_value;
};

}  // namespace

TEST_F(MasterPreferencesTest, NoFileToParse) {
  EXPECT_TRUE(base::DeleteFile(prefs_file(), false));
  installer::MasterPreferences prefs(prefs_file());
  EXPECT_FALSE(prefs.read_from_file());
}

TEST_F(MasterPreferencesTest, ParseDistroParams) {
  const char text[] =
    "{ \n"
    "  \"distribution\": { \n"
    "     \"show_welcome_page\": true,\n"
    "     \"import_search_engine\": true,\n"
    "     \"import_history\": true,\n"
    "     \"import_bookmarks\": true,\n"
    "     \"import_bookmarks_from_file\": \"c:\\\\foo\",\n"
    "     \"import_home_page\": true,\n"
    "     \"do_not_create_any_shortcuts\": true,\n"
    "     \"do_not_create_desktop_shortcut\": true,\n"
    "     \"do_not_create_quick_launch_shortcut\": true,\n"
    "     \"do_not_create_taskbar_shortcut\": true,\n"
    "     \"do_not_launch_chrome\": true,\n"
    "     \"make_chrome_default\": true,\n"
    "     \"make_chrome_default_for_user\": true,\n"
    "     \"system_level\": true,\n"
    "     \"verbose_logging\": true,\n"
    "     \"require_eula\": true,\n"
    "     \"alternate_shortcut_text\": true,\n"
    "     \"chrome_shortcut_icon_index\": 1,\n"
    "     \"ping_delay\": 40\n"
    "  },\n"
    "  \"blah\": {\n"
    "     \"import_history\": false\n"
    "  }\n"
    "} \n";

  EXPECT_TRUE(base::WriteFile(prefs_file(), text, strlen(text)));
  installer::MasterPreferences prefs(prefs_file());
  EXPECT_TRUE(prefs.read_from_file());

  const char* const expected_true[] = {
    installer::master_preferences::kDistroImportSearchPref,
    installer::master_preferences::kDistroImportHistoryPref,
    installer::master_preferences::kDistroImportBookmarksPref,
    installer::master_preferences::kDistroImportHomePagePref,
    installer::master_preferences::kDoNotCreateAnyShortcuts,
    installer::master_preferences::kDoNotCreateDesktopShortcut,
    installer::master_preferences::kDoNotCreateQuickLaunchShortcut,
    installer::master_preferences::kDoNotCreateTaskbarShortcut,
    installer::master_preferences::kDoNotLaunchChrome,
    installer::master_preferences::kMakeChromeDefault,
    installer::master_preferences::kMakeChromeDefaultForUser,
    installer::master_preferences::kSystemLevel,
    installer::master_preferences::kVerboseLogging,
    installer::master_preferences::kRequireEula,
    installer::master_preferences::kAltShortcutText,
  };

  for (int i = 0; i < arraysize(expected_true); ++i) {
    bool value = false;
    EXPECT_TRUE(prefs.GetBool(expected_true[i], &value));
    EXPECT_TRUE(value) << expected_true[i];
  }

  std::string str_value;
  EXPECT_TRUE(prefs.GetString(
      installer::master_preferences::kDistroImportBookmarksFromFilePref,
      &str_value));
  EXPECT_STREQ("c:\\foo", str_value.c_str());

  int icon_index = 0;
  EXPECT_TRUE(prefs.GetInt(
      installer::master_preferences::kChromeShortcutIconIndex,
      &icon_index));
  EXPECT_EQ(icon_index, 1);
  int ping_delay = 90;
  EXPECT_TRUE(prefs.GetInt(installer::master_preferences::kDistroPingDelay,
                           &ping_delay));
  EXPECT_EQ(ping_delay, 40);
}

TEST_F(MasterPreferencesTest, ParseMissingDistroParams) {
  const char text[] =
    "{ \n"
    "  \"distribution\": { \n"
    "     \"import_search_engine\": true,\n"
    "     \"import_bookmarks\": false,\n"
    "     \"import_bookmarks_from_file\": \"\",\n"
    "     \"do_not_create_desktop_shortcut\": true,\n"
    "     \"do_not_create_quick_launch_shortcut\": true,\n"
    "     \"do_not_launch_chrome\": true,\n"
    "     \"chrome_shortcut_icon_index\": \"bac\"\n"
    "  }\n"
    "} \n";

  EXPECT_TRUE(base::WriteFile(prefs_file(), text, strlen(text)));
  installer::MasterPreferences prefs(prefs_file());
  EXPECT_TRUE(prefs.read_from_file());

  ExpectedBooleans expected_bool[] = {
    { installer::master_preferences::kDistroImportSearchPref, true },
    { installer::master_preferences::kDistroImportBookmarksPref, false },
    { installer::master_preferences::kDoNotCreateDesktopShortcut, true },
    { installer::master_preferences::kDoNotCreateQuickLaunchShortcut, true },
    { installer::master_preferences::kDoNotLaunchChrome, true },
  };

  bool value = false;
  for (int i = 0; i < arraysize(expected_bool); ++i) {
    EXPECT_TRUE(prefs.GetBool(expected_bool[i].name, &value));
    EXPECT_EQ(value, expected_bool[i].expected_value) << expected_bool[i].name;
  }

  const char* const missing_bools[] = {
    installer::master_preferences::kDistroImportHomePagePref,
    installer::master_preferences::kDoNotRegisterForUpdateLaunch,
    installer::master_preferences::kMakeChromeDefault,
    installer::master_preferences::kMakeChromeDefaultForUser,
  };

  for (int i = 0; i < arraysize(missing_bools); ++i) {
    EXPECT_FALSE(prefs.GetBool(missing_bools[i], &value)) << missing_bools[i];
  }

  std::string str_value;
  EXPECT_FALSE(prefs.GetString(
      installer::master_preferences::kDistroImportBookmarksFromFilePref,
      &str_value));

  int icon_index = 0;
  EXPECT_FALSE(prefs.GetInt(
      installer::master_preferences::kChromeShortcutIconIndex,
      &icon_index));
  EXPECT_EQ(icon_index, 0);

  int ping_delay = 90;
  EXPECT_FALSE(prefs.GetInt(
      installer::master_preferences::kDistroPingDelay, &ping_delay));
  EXPECT_EQ(ping_delay, 90);
}

TEST_F(MasterPreferencesTest, FirstRunTabs) {
  const char text[] =
    "{ \n"
    "  \"distribution\": { \n"
    "     \"something here\": true\n"
    "  },\n"
    "  \"first_run_tabs\": [\n"
    "     \"http://google.com/f1\",\n"
    "     \"https://google.com/f2\",\n"
    "     \"new_tab_page\"\n"
    "  ]\n"
    "} \n";

  EXPECT_TRUE(base::WriteFile(prefs_file(), text, strlen(text)));
  installer::MasterPreferences prefs(prefs_file());
  typedef std::vector<std::string> TabsVector;
  TabsVector tabs = prefs.GetFirstRunTabs();
  ASSERT_EQ(3, tabs.size());
  EXPECT_EQ("http://google.com/f1", tabs[0]);
  EXPECT_EQ("https://google.com/f2", tabs[1]);
  EXPECT_EQ("new_tab_page", tabs[2]);
}

// In this test instead of using our synthetic json file, we use an
// actual test case from the extensions unittest. The hope here is that if
// they change something in the manifest this test will break, but in
// general it is expected the extension format to be backwards compatible.
TEST(MasterPrefsExtension, ValidateExtensionJSON) {
  base::FilePath prefs_path;
  ASSERT_TRUE(PathService::Get(chrome::DIR_TEST_DATA, &prefs_path));
  prefs_path = prefs_path.AppendASCII("extensions")
      .AppendASCII("good").AppendASCII("Preferences");

  installer::MasterPreferences prefs(prefs_path);
  base::DictionaryValue* extensions = NULL;
  EXPECT_TRUE(prefs.GetExtensionsBlock(&extensions));
  int location = 0;
  EXPECT_TRUE(extensions->GetInteger(
      "behllobkkfkfnphdnhnkndlbkcpglgmj.location", &location));
  int state = 0;
  EXPECT_TRUE(extensions->GetInteger(
      "behllobkkfkfnphdnhnkndlbkcpglgmj.state", &state));
  std::string path;
  EXPECT_TRUE(extensions->GetString(
      "behllobkkfkfnphdnhnkndlbkcpglgmj.path", &path));
  std::string key;
  EXPECT_TRUE(extensions->GetString(
      "behllobkkfkfnphdnhnkndlbkcpglgmj.manifest.key", &key));
  std::string name;
  EXPECT_TRUE(extensions->GetString(
      "behllobkkfkfnphdnhnkndlbkcpglgmj.manifest.name", &name));
  std::string version;
  EXPECT_TRUE(extensions->GetString(
      "behllobkkfkfnphdnhnkndlbkcpglgmj.manifest.version", &version));
}

// Test that we are parsing master preferences correctly.
TEST_F(MasterPreferencesTest, GetInstallPreferencesTest) {
  // Create a temporary prefs file.
  base::FilePath prefs_file;
  ASSERT_TRUE(base::CreateTemporaryFile(&prefs_file));
  const char text[] =
    "{ \n"
    "  \"distribution\": { \n"
    "     \"do_not_create_desktop_shortcut\": false,\n"
    "     \"do_not_create_quick_launch_shortcut\": false,\n"
    "     \"do_not_launch_chrome\": true,\n"
    "     \"system_level\": true,\n"
    "     \"verbose_logging\": false\n"
    "  }\n"
    "} \n";
  EXPECT_TRUE(base::WriteFile(prefs_file, text, strlen(text)));

  // Make sure command line values override the values in master preferences.
  std::wstring cmd_str(
      L"setup.exe --installerdata=\"" + prefs_file.value() + L"\"");
  cmd_str.append(L" --do-not-launch-chrome");
  CommandLine cmd_line = CommandLine::FromString(cmd_str);
  installer::MasterPreferences prefs(cmd_line);

  // Check prefs that do not have any equivalent command line option.
  ExpectedBooleans expected_bool[] = {
    { installer::master_preferences::kDoNotLaunchChrome, true },
    { installer::master_preferences::kSystemLevel, true },
    { installer::master_preferences::kVerboseLogging, false },
  };

  // Now check that prefs got merged correctly.
  bool value = false;
  for (int i = 0; i < arraysize(expected_bool); ++i) {
    EXPECT_TRUE(prefs.GetBool(expected_bool[i].name, &value));
    EXPECT_EQ(value, expected_bool[i].expected_value) << expected_bool[i].name;
  }

  // Delete temporary prefs file.
  EXPECT_TRUE(base::DeleteFile(prefs_file, false));

  // Check that if master prefs doesn't exist, we can still parse the common
  // prefs.
  cmd_str = L"setup.exe --do-not-launch-chrome";
  cmd_line.ParseFromString(cmd_str);
  installer::MasterPreferences prefs2(cmd_line);
  ExpectedBooleans expected_bool2[] = {
    { installer::master_preferences::kDoNotLaunchChrome, true },
  };

  for (int i = 0; i < arraysize(expected_bool2); ++i) {
    EXPECT_TRUE(prefs2.GetBool(expected_bool2[i].name, &value));
    EXPECT_EQ(value, expected_bool2[i].expected_value)
        << expected_bool2[i].name;
  }

  EXPECT_FALSE(prefs2.GetBool(
      installer::master_preferences::kSystemLevel, &value));
  EXPECT_FALSE(prefs2.GetBool(
      installer::master_preferences::kVerboseLogging, &value));
}

TEST_F(MasterPreferencesTest, TestDefaultInstallConfig) {
  std::wstringstream chrome_cmd;
  chrome_cmd << "setup.exe";

  CommandLine chrome_install(CommandLine::FromString(chrome_cmd.str()));

  installer::MasterPreferences pref_chrome(chrome_install);

  EXPECT_FALSE(pref_chrome.is_multi_install());
  EXPECT_TRUE(pref_chrome.install_chrome());
}

TEST_F(MasterPreferencesTest, TestMultiInstallConfig) {
  using installer::switches::kMultiInstall;
  using installer::switches::kChrome;

  std::wstringstream chrome_cmd, cf_cmd, chrome_cf_cmd;
  chrome_cmd << "setup.exe --" << kMultiInstall << " --" << kChrome;

  CommandLine chrome_install(CommandLine::FromString(chrome_cmd.str()));

  installer::MasterPreferences pref_chrome(chrome_install);

  EXPECT_TRUE(pref_chrome.is_multi_install());
  EXPECT_TRUE(pref_chrome.install_chrome());
}

TEST_F(MasterPreferencesTest, EnforceLegacyCreateAllShortcutsFalse) {
  static const char kCreateAllShortcutsFalsePrefs[] =
      "{"
      "  \"distribution\": {"
      "     \"create_all_shortcuts\": false"
      "  }"
      "}";

    installer::MasterPreferences prefs(kCreateAllShortcutsFalsePrefs);

    bool do_not_create_desktop_shortcut = false;
    bool do_not_create_quick_launch_shortcut = false;
    bool do_not_create_taskbar_shortcut = false;
    prefs.GetBool(
        installer::master_preferences::kDoNotCreateDesktopShortcut,
        &do_not_create_desktop_shortcut);
    prefs.GetBool(
        installer::master_preferences::kDoNotCreateQuickLaunchShortcut,
        &do_not_create_quick_launch_shortcut);
    prefs.GetBool(
        installer::master_preferences::kDoNotCreateTaskbarShortcut,
        &do_not_create_taskbar_shortcut);
    // create_all_shortcuts is a legacy preference that should only enforce
    // do_not_create_desktop_shortcut and do_not_create_quick_launch_shortcut
    // when set to false.
    EXPECT_TRUE(do_not_create_desktop_shortcut);
    EXPECT_TRUE(do_not_create_quick_launch_shortcut);
    EXPECT_FALSE(do_not_create_taskbar_shortcut);
}

TEST_F(MasterPreferencesTest, DontEnforceLegacyCreateAllShortcutsTrue) {
  static const char kCreateAllShortcutsFalsePrefs[] =
      "{"
      "  \"distribution\": {"
      "     \"create_all_shortcuts\": true"
      "  }"
      "}";

    installer::MasterPreferences prefs(kCreateAllShortcutsFalsePrefs);

    bool do_not_create_desktop_shortcut = false;
    bool do_not_create_quick_launch_shortcut = false;
    bool do_not_create_taskbar_shortcut = false;
    prefs.GetBool(
        installer::master_preferences::kDoNotCreateDesktopShortcut,
        &do_not_create_desktop_shortcut);
    prefs.GetBool(
        installer::master_preferences::kDoNotCreateQuickLaunchShortcut,
        &do_not_create_quick_launch_shortcut);
    prefs.GetBool(
        installer::master_preferences::kDoNotCreateTaskbarShortcut,
        &do_not_create_taskbar_shortcut);
    EXPECT_FALSE(do_not_create_desktop_shortcut);
    EXPECT_FALSE(do_not_create_quick_launch_shortcut);
    EXPECT_FALSE(do_not_create_taskbar_shortcut);
}

TEST_F(MasterPreferencesTest, DontEnforceLegacyCreateAllShortcutsNotSpecified) {
  static const char kCreateAllShortcutsFalsePrefs[] =
      "{"
      "  \"distribution\": {"
      "     \"some_other_pref\": true"
      "  }"
      "}";

    installer::MasterPreferences prefs(kCreateAllShortcutsFalsePrefs);

    bool do_not_create_desktop_shortcut = false;
    bool do_not_create_quick_launch_shortcut = false;
    bool do_not_create_taskbar_shortcut = false;
    prefs.GetBool(
        installer::master_preferences::kDoNotCreateDesktopShortcut,
        &do_not_create_desktop_shortcut);
    prefs.GetBool(
        installer::master_preferences::kDoNotCreateQuickLaunchShortcut,
        &do_not_create_quick_launch_shortcut);
    prefs.GetBool(
        installer::master_preferences::kDoNotCreateTaskbarShortcut,
        &do_not_create_taskbar_shortcut);
    EXPECT_FALSE(do_not_create_desktop_shortcut);
    EXPECT_FALSE(do_not_create_quick_launch_shortcut);
    EXPECT_FALSE(do_not_create_taskbar_shortcut);
}

TEST_F(MasterPreferencesTest, MigrateOldStartupUrlsPref) {
  static const char kOldMasterPrefs[] =
      "{ \n"
      "  \"distribution\": { \n"
      "     \"show_welcome_page\": true,\n"
      "     \"import_search_engine\": true,\n"
      "     \"import_history\": true,\n"
      "     \"import_bookmarks\": true\n"
      "  },\n"
      "  \"session\": {\n"
      "     \"urls_to_restore_on_startup\": [\"http://www.google.com\"]\n"
      "  }\n"
      "} \n";

  const installer::MasterPreferences prefs(kOldMasterPrefs);
  const base::DictionaryValue& master_dictionary =
      prefs.master_dictionary();

  const base::ListValue* old_startup_urls_list = NULL;
  EXPECT_TRUE(master_dictionary.GetList(prefs::kURLsToRestoreOnStartupOld,
                                        &old_startup_urls_list));
  EXPECT_TRUE(old_startup_urls_list != NULL);

  // The MasterPreferences dictionary should also conjure up the new setting
  // as per EnforceLegacyPreferences.
  const base::ListValue* new_startup_urls_list = NULL;
  EXPECT_TRUE(master_dictionary.GetList(prefs::kURLsToRestoreOnStartup,
                                        &new_startup_urls_list));
  EXPECT_TRUE(new_startup_urls_list != NULL);
}

TEST_F(MasterPreferencesTest, DontMigrateOldStartupUrlsPrefWhenNewExists) {
  static const char kOldAndNewMasterPrefs[] =
      "{ \n"
      "  \"distribution\": { \n"
      "     \"show_welcome_page\": true,\n"
      "     \"import_search_engine\": true,\n"
      "     \"import_history\": true,\n"
      "     \"import_bookmarks\": true\n"
      "  },\n"
      "  \"session\": {\n"
      "     \"urls_to_restore_on_startup\": [\"http://www.google.com\"],\n"
      "     \"startup_urls\": [\"http://www.example.com\"]\n"
      "  }\n"
      "} \n";

  const installer::MasterPreferences prefs(kOldAndNewMasterPrefs);
  const base::DictionaryValue& master_dictionary =
      prefs.master_dictionary();

  const base::ListValue* old_startup_urls_list = NULL;
  EXPECT_TRUE(master_dictionary.GetList(prefs::kURLsToRestoreOnStartupOld,
                                        &old_startup_urls_list));
  ASSERT_TRUE(old_startup_urls_list != NULL);
  std::string url_value;
  EXPECT_TRUE(old_startup_urls_list->GetString(0, &url_value));
  EXPECT_EQ("http://www.google.com", url_value);

  // The MasterPreferences dictionary should also conjure up the new setting
  // as per EnforceLegacyPreferences.
  const base::ListValue* new_startup_urls_list = NULL;
  EXPECT_TRUE(master_dictionary.GetList(prefs::kURLsToRestoreOnStartup,
                                        &new_startup_urls_list));
  ASSERT_TRUE(new_startup_urls_list != NULL);
  std::string new_url_value;
  EXPECT_TRUE(new_startup_urls_list->GetString(0, &new_url_value));
  EXPECT_EQ("http://www.example.com", new_url_value);
}
