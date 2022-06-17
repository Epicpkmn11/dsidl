#include "menu.h"

#include <nds.h>
#include <stdarg.h>

#define SCREEN_COLS 32
#define SCREEN_ROWS 24

void Menu::print(const char *format, ...) {
	va_list args;
	va_start(args, format);
	viprintf(format, args);
	va_end(args);
}

void Menu::printDelay(int delay, const char *format, ...) {
	va_list args;
	va_start(args, format);
	viprintf(format, args);
	va_end(args);

	for(int i = 0; i < delay; i++)
		swiWaitForVBlank();
}

u16 Menu::prompt(u16 keymask, const char *format, ...) {
	va_list args;
	va_start(args, format);
	viprintf(format, args);
	va_end(args);

	u16 down;
	do {
		swiWaitForVBlank();
		scanKeys();
		down = keysDown();
	} while(!(down & keymask));

	return down;
}

void Menu::draw(bool clear) {
	if(clear)
		consoleClear();

	printf("\x1B[0;0H%-32.32s================================", _title.c_str());

	for(int i = 0; i < (SCREEN_ROWS - 2) && (i + _scrollPos) < (int)_items.size(); i++) {
		printf("%c %-30.30s", (_scrollPos + i) == _curPos ? '>' : ' ', _items[_scrollPos + i].title.c_str());
	}

	if(_items.size() < (SCREEN_ROWS) - 2)
		printf("================================");
}

Menu &Menu::addItem(const MenuItem &item) {
	sassert((item.hotkey & _keymask) == 0, "Error in %s:\nKey already assigned\nOld 0x%04X\nNew: 0x%04X", item.title.c_str(), _keymask, item.hotkey);

	_keymask |= item.hotkey;
	_items.push_back(item);

	return *this;
}

Menu &Menu::clear(void) {
	_curPos = 0;
	_scrollPos = 0;
	_keymask = KEY_UP | KEY_DOWN | KEY_LEFT | KEY_RIGHT | KEY_A;
	_items.clear();
	
	return *this;
}

void Menu::run() {
	while(!_exit) {
		draw(false);

		u16 pressed, held;
		do {
			swiWaitForVBlank();
			scanKeys();
			held = keysDownRepeat();
			pressed = keysDown();
			
			if(_vblank)
				_vblank(*this);
		} while(!(held & (KEY_UP | KEY_DOWN | KEY_LEFT | KEY_RIGHT) || (pressed & _keymask)));

		// Process input
		if(pressed & KEY_A) {
			if(_items[_curPos].callback) {
				draw(true);
				_items[_curPos].callback(*this);
			}
		} else if(held & KEY_UP) {
			_curPos--;
			if(_curPos < 0)
				_curPos = (int)_items.size() - 1;
		} else if(held & KEY_DOWN) {
			_curPos++;
			if(_curPos >= (int)_items.size())
				_curPos = 0;
		} else if(held & KEY_LEFT) {
			_curPos -= (SCREEN_ROWS - 2);
			if(_curPos < 0)
				_curPos = 0;
		} else if(held & KEY_RIGHT) {
			_curPos += (SCREEN_ROWS - 2);
			if(_curPos >= (int)_items.size())
				_curPos = _items.size() - 1;
		} else {
			for(const MenuItem &item : _items) {
				if(pressed & item.hotkey) {
					if(item.callback) {
						draw(true);
						item.callback(*this);
					}
				}
			}
		}

		// Scroll if needed
		if(_curPos < _scrollPos)
			_scrollPos = _curPos;
		else if(_curPos - (SCREEN_ROWS - 3) > _scrollPos)
			_scrollPos = _curPos - (SCREEN_ROWS - 3);
	}
}