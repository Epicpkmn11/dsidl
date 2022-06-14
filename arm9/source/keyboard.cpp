// Based on GodMode9i
// https://github.com/DS-Homebrew/GodMode9i/blob/cce31e15f201dc6d00c3437c4dbe6b2ca2f0d527/arm9/source/keyboard.cpp

#include "keyboard.h"

#include <nds.h>
#include <string.h>

#define SCREEN_COLS 32

std::string kbdGetString(std::string label, int maxSize, std::string oldStr) {
	keyboardDemoInit();
	BG_PALETTE_SUB[0] = 0x0000;
	BG_PALETTE_SUB[1] = 0x7FFF;
	keyboardShow();

	std::string output(oldStr);

	int stringPosition = output.size(), scrollPosition = stringPosition;
	for(int i = 0; i < SCREEN_COLS - 4 && scrollPosition > 0; i++) {
		scrollPosition--;
		while((output[scrollPosition] & 0xC0) == 0x80) // UTF-8
			scrollPosition--;
	}

	u16 pressed;
	int key;
	bool done = false;
	while(!done) {
		consoleClear();
		iprintf("%s\n", label.c_str());
		iprintf("================================");

		iprintf(">%c%-29.29s%c\n", (scrollPosition > 0) ? '\x1B' : ' ', output.c_str() + scrollPosition, (scrollPosition < (int)output.size() - (SCREEN_COLS - 3)) ? '\x1A' : ' ');

		int charLen = 1;
		while((output[stringPosition + charLen] & 0xC0) == 0x80)
			charLen++;
		int cursorPosition = 0;
		for(int i = 0; scrollPosition + i < stringPosition; i++) {
			if((output[scrollPosition + i] & 0xC0) != 0x80)
				cursorPosition++;
		}
		iprintf("\x1B[42m\x1B[2;%dH%s\x1B[47m\n\n", 2 + cursorPosition, stringPosition < (int)output.size() ? output.substr(stringPosition, charLen).c_str() : "_");

		do {
			scanKeys();
			pressed = keysDownRepeat();
			key = keyboardUpdate();
			swiWaitForVBlank();
		} while (!((pressed & (KEY_LEFT | KEY_RIGHT | KEY_B | KEY_START)) || (key != -1)));

		switch(key) {
			case NOKEY:
			case DVK_MENU:
			case DVK_CAPS: // Caps
			case DVK_SHIFT: // Shift
			case DVK_CTRL: // Ctrl
			case DVK_UP: // Up
			case DVK_DOWN: // Down
			case DVK_ALT: // Alt
			case DVK_TAB: // tab
				break;
			case DVK_RIGHT: // Right
				pressed |= KEY_RIGHT;
				break;
			case DVK_LEFT: // Left
				pressed |= KEY_LEFT;
				break;
			case DVK_FOLD: // (using as esc)
				output = "|cancel|";
				done = true;
				break;
			case DVK_BACKSPACE: // Backspace
				pressed |= KEY_B;
				break;
			case DVK_ENTER: // Return
				done = true;
				break;
			default: // Letter
				if(output.size() < (uint)maxSize) {
					output.insert(output.begin() + stringPosition, key);
					stringPosition++;

					if(cursorPosition + 1 >= (SCREEN_COLS - 3) && stringPosition <= (int)output.size()) {
						scrollPosition++;
						while((output[scrollPosition] & 0xC0) == 0x80) // UTF-8
							scrollPosition++;
					}
				}
				break;
		}

		if(pressed & KEY_LEFT) {
			if(stringPosition > 0) {
				stringPosition--;
				while((output[stringPosition] & 0xC0) == 0x80) // UTF-8
					stringPosition--;

				if(cursorPosition - 1 < 0) {
					scrollPosition--;
					while((output[scrollPosition] & 0xC0) == 0x80) // UTF-8
						scrollPosition--;
				}
			}
		} else if(pressed & KEY_RIGHT) {
			if(stringPosition < (int)output.size()) {
				stringPosition++;
				while((output[stringPosition] & 0xC0) == 0x80) // UTF-8
					stringPosition++;

				if(cursorPosition + 1 >= (SCREEN_COLS - 3)) {
					scrollPosition++;
					while((output[scrollPosition] & 0xC0) == 0x80) // UTF-8
						scrollPosition++;
				}
			}
		} else if(pressed & KEY_B) {
			if(stringPosition > 0) {
				stringPosition--;
				while((output[stringPosition] & 0xC0) == 0x80) {
					output.erase(output.begin() + stringPosition);
					stringPosition--;
				}
				output.erase(output.begin() + stringPosition);
			}
		} else if(pressed & KEY_START) {
			done = true;
		}
	}
	keyboardHide();

#ifdef SCREENSWAP
	screenSwapped ? lcdMainOnBottom() : lcdMainOnTop();
#endif

	consoleClear();
	return output;
}
