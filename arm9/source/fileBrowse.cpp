#include "fileBrowse.h"

#include "keyboard.h"

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
		iprintf("Unable to open directory.\n");
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

	dirContents.insert(dirContents.begin(), "<Current Directory>");
	dirContents.insert(dirContents.begin(), "<Back>");

	return dirContents;
}

std::string selectDir() {
	std::vector<std::string> dirContents = getDirectoryContents();

	int cursorPos = 0, scrollPos = 0;

	while(1) {
		consoleClear();
		iprintf("Select a directory to save to.\n");
		iprintf("================================");
		for(int i = scrollPos; i < (int)dirContents.size() && i - scrollPos < ENTRIES_PER_SCREEN; i++)
			iprintf("\x1B[%d;0H%c %.29s", 2 + i - scrollPos, i == cursorPos ? '>' : ' ', dirContents[i].c_str());

		u16 pressed, held;
		do {
			swiWaitForVBlank();
			scanKeys();
			pressed = keysDown();
			held = keysDownRepeat();
		} while(!((held & (KEY_UP | KEY_DOWN | KEY_LEFT | KEY_RIGHT)) || (pressed & (KEY_A | KEY_B | KEY_START))));

		if((pressed & KEY_A) && cursorPos > CURDIR_POS) {
			chdir(dirContents[cursorPos].c_str());
			dirContents = getDirectoryContents();
			cursorPos = 0;
			scrollPos = 0;
		} else if((pressed & KEY_B) || ((pressed & KEY_A) && cursorPos == BACK_POS)) {
			char cwd[PATH_MAX];
			getcwd(cwd, PATH_MAX);
			if(strlen(strchr(cwd, '/')) == 1) { // in root, cancel
				consoleClear();
				return "|cancel|";
			}

			chdir("..");
			dirContents = getDirectoryContents();
			cursorPos = 0;
			scrollPos = 0;
		} else if((pressed & KEY_START) || ((pressed & KEY_A) && cursorPos == CURDIR_POS)) {
			char cwd[PATH_MAX];
			getcwd(cwd, PATH_MAX);
			consoleClear();
			return cwd;
		} else if(held & KEY_UP) {
			cursorPos--;
			if(cursorPos < 0)
				cursorPos = (int)dirContents.size() - 1;
		} else if(held & KEY_DOWN) {
			cursorPos++;
			if(cursorPos >= (int)dirContents.size())
				cursorPos = 0;
		} else if(held & KEY_LEFT) {
			cursorPos -= ENTRIES_PER_SCREEN;
			if(cursorPos < 0)
				cursorPos = 0;
		} else if(held & KEY_RIGHT) {
			cursorPos += ENTRIES_PER_SCREEN;
			if(cursorPos <= (int)dirContents.size())
				cursorPos = dirContents.size() - 1;
		}

		if(cursorPos < scrollPos)
			scrollPos = cursorPos;
		else if(cursorPos - ENTRIES_PER_SCREEN > scrollPos)
			scrollPos = cursorPos - ENTRIES_PER_SCREEN;
	}
}

std::string selectFile(const std::string &fileName) {
	std::string dir = selectDir();
	if(dir == "|cancel|")
		return dir;

	std::string str = kbdGetString("Enter a file name to save as.", 255, fileName);
	if(str == "|cancel|" || str == "")
		return "|cancel|";

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
