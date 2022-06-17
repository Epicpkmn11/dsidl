#include "fileBrowse.h"

#include "keyboard.h"
#include "menu.h"

#include <algorithm>
#include <dirent.h>
#include <nds.h>
#include <unistd.h>
#include <vector>

#define ENTRIES_PER_SCREEN (24 - 2)
#define BACK_POS 0
#define CURDIR_POS 1

static std::vector<std::string> getDirectoryContents() {
	std::vector<std::string> dirContents;

	DIR *pdir = opendir(".");
	if(pdir == nullptr) {
		Menu::print("Unable to open directory.\n");
		return {};
	} else {
		while(true) {
			dirent *pent = readdir(pdir);
			if(pent == nullptr)
				break;

			if(pent->d_type == DT_DIR && pent->d_name[0] != '.')
				dirContents.emplace_back(pent->d_name);
		}
		closedir(pdir);
	}

	// std::sort(dirContents.begin(), dirContents.end(), [](const std::string &a, const std::string &b) { return strcasecmp(a.c_str(), b.c_str()); });

	return dirContents;
}

static void backDir(Menu &menu);
static void exitDir(Menu &menu);
static void enterDir(Menu &menu);

static void backDir(Menu &menu) {
	char cwd[PATH_MAX];
	getcwd(cwd, PATH_MAX);
	if(strlen(strchr(cwd, '/')) == 1) { // in root, cancel
		menu.exit();
	} else {
		chdir("..");
		menu.clear();
		menu.addItem({"<Back>", KEY_B, std::function<void(Menu &)>(backDir)});
	menu.addItem({"<Current Directory>", KEY_START, std::function<void(Menu &)>(exitDir)});
		std::vector<std::string> dirContents = getDirectoryContents();
		for(const std::string &dir : dirContents)
			menu.addItem({dir, KEY_NONE, std::function<void(Menu &)>(enterDir)});
	}
	consoleClear();
}

static void exitDir(Menu &menu) {
	char cwd[PATH_MAX];
	getcwd(cwd, PATH_MAX);
	menu.exit(cwd);
}

static void enterDir(Menu &menu) {
	chdir(menu.current().title.c_str());

	menu.clear();
	menu.addItem({"<Back>", KEY_B, std::function<void(Menu &)>(backDir)});
	menu.addItem({"<Current Directory>", KEY_START, std::function<void(Menu &)>(exitDir)});
	std::vector<std::string> dirContents = getDirectoryContents();
	for(const std::string &dir : dirContents)
		menu.addItem({dir, KEY_NONE, std::function<void(Menu &)>(enterDir)});
	consoleClear();
}

std::string selectDir() {
	Menu menu("Select a directory to save to.");
	menu.addItem({"<Back>                       B", KEY_B, std::function<void(Menu &)>(backDir)});
	menu.addItem({"<Current Directory>      START", KEY_START, std::function<void(Menu &)>(exitDir)});

	{
		std::vector<std::string> dirContents = getDirectoryContents();
		for(const std::string &dir : dirContents)
			menu.addItem({dir, KEY_NONE, std::function<void(Menu &)>(enterDir)});
	}

	consoleClear();
	menu.run();

	return menu.result();
}

std::string selectFile(const std::string &fileName) {
	std::string dir = selectDir();
	if(dir == "")
		return dir;

	std::string str = kbdGetString("Enter a file name to save as.", 255, fileName);
	if(str == "")
		return str;

	// Ensure name is FAT safe
	for(uint i = 0; i < str.size(); i++) {
		switch(str[i]) {
			case '>':
			case '<':
			case ':':
			case '"':
			case '/':
			case '\\':
			case '|':
			case '?':
			case '*':
				str[i] = '_';
		}
	}

	return dir + str;
}
