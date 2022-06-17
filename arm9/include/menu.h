#ifndef MENU_H
#define MENU_H

#include <functional>
#include <nds.h>
#include <string>
#include <vector>

#define KEY_NONE 0

class Menu;

struct MenuItem {
	std::string title;
	u16 hotkey;
	std::function<void(Menu &)> callback;

	MenuItem(std::string title, u16 hotkey, std::function<void(Menu &)> callback) : title(title), hotkey(hotkey), callback(callback) { }
};

class Menu {
	std::string _title, _result;
	std::vector<MenuItem> _items;
	int _curPos = 0, _scrollPos = 0;
	bool _exit = false;
	std::function<void(Menu &)> _vblank;
	u16 _keymask = KEY_UP | KEY_DOWN | KEY_LEFT | KEY_RIGHT | KEY_A;

	void draw(bool clear);

public:
	static void print(const char *format, ...);
	static void printDelay(int delay, const char *format, ...);
	static u16 prompt(u16 keymask, const char *format, ...);

	Menu(const std::string &title, std::function<void(Menu &)> vblank = nullptr) : _title(title), _vblank(vblank) { }

	Menu &addItem(const MenuItem &item);
	Menu &clear(void);
	Menu &exit(const std::string &result = "") { _exit = true; _result = result; return *this; }

	int curPos(void) const { return _curPos; }
	int scrollPos(void) const { return _scrollPos; }
	const MenuItem &current(void) const { return _items[_curPos]; }
	const std::string &result(void) const { return _result; }

	void run(void);
};

#endif // MENU_H
