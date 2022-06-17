#include "camera.h"
#include "download.h"
#include "quirc.h"
#include "script.h"
#include "version.h"
#include "menu.h"
#include "wifi.h"

#include "overlay.h"

#include <dirent.h>
#include <fat.h>
#include <math.h>
#include <nds.h>
#include <stdio.h>

bool verbose = false;

int main(int argc, char **argv) {
	consoleDemoInit();

	bool fatInited = fatInitDefault();
	if(!fatInited) {
		Menu::prompt(KEY_START, "FAT init failed.\n\nPress START to quit.");
		return 1;
	};

	vramSetBankA(VRAM_A_MAIN_BG);
	videoSetMode(MODE_5_2D);
	int bg0Main = bgInit(0, BgType_Text4bpp, BgSize_T_256x256, 7, 0);
	int bg3Main = bgInit(3, BgType_Bmp16, BgSize_B16_256x256, 1, 0);

	dmaCopy(overlayPal, BG_PALETTE, overlayPalLen);
	decompress(overlayTiles, bgGetGfxPtr(bg0Main), LZ77Vram);
	decompress(overlayMap, bgGetMapPtr(bg0Main), LZ77Vram);
	REG_BLDCNT = BLEND_SRC_BG0 | BLEND_ALPHA | BLEND_DST_BG3;
	REG_BLDALPHA = 0x0A00;

	keysSetRepeat(25, 5);
	scanKeys();
	verbose = keysHeld() & KEY_SELECT;

	// If not using no$gba, initialize Wi-Fi
	if(strncmp((const char *)0x4FFFA00, "no$gba", 6) != 0) {
		Menu::print("Connecting to Wi-Fi...\n");
		Menu::print("If this doesn't connect, ensure\nyour Wi-Fi is working in System\nSettings.\n\nIf it still doesn't work your\nrouter isn't compatible yet.\n\nHold SELECT while loading for\nverbose logging.\n\n");

		wifiInit();
		uint i = 0;
		while(!wifiConnected()) {
			swiWaitForVBlank();
			const char spinner[] = "-\\|/";
			Menu::print("\x1B[23;31H%c", spinner[(i++ / 6) % (sizeof(spinner) - 1)]);
		}
	}

	consoleClear();
	Menu::print("Starting camera...\n");
	cameraInit();

	Camera camera = CAM_OUTER;
	cameraActivate(camera);

	Menu mainMenu("dsidl " VER_NUMBER, [&bg3Main](Menu &) {
		if(!cameraTransferActive())
			cameraTransferStart(bgGetGfxPtr(bg3Main), CAPTURE_MODE_PREVIEW);
	});

	mainMenu.addItem({"Scan QR                    L/R", KEY_L | KEY_R, [](Menu &) {
		Menu::print("Scanning for QR... ");

		// Wait for previous transfer to finish
		while(cameraTransferActive())
			swiWaitForVBlank();

		// Get image
		u16 *buffer = (u16 *)malloc(640 * 480 * sizeof(u16));
		cameraTransferStart(buffer, CAPTURE_MODE_CAPTURE);
		while(cameraTransferActive())
			swiWaitForVBlank();
		cameraTransferStop();

		struct quirc *q = quirc_new();
		quirc_resize(q, 640, 480);
		uint8_t *qrbuffer = quirc_begin(q, nullptr, nullptr);
		// Copy Y values to qrbuffer
		for(int i = 0; i < 640 * 480; i++) {
			qrbuffer[i] = buffer[i] & 0xFF;
		}
		quirc_end(q);

		std::string payload;

		if(quirc_count(q) > 0) {
			struct quirc_code code;
			struct quirc_data data;
			quirc_extract(q, 0, &code);
			if(!quirc_decode(&code, &data)) {
				payload = (const char *)data.payload;
			}
		}

		quirc_destroy(q);
		free(buffer);
		Menu::print("%s!\n", payload == "" ? "Not found" : "Done");

		// If we found a QR, process its content
		if(payload != "")
			runScript(payload);
	}});

	mainMenu.addItem({"Swap camera                  Y", KEY_Y, [&camera](Menu &) {
		// Wait for previous transfer to finish
		while(cameraTransferActive())
			swiWaitForVBlank();
		cameraTransferStop();

		// Switch camera
		camera = camera == CAM_INNER ? CAM_OUTER : CAM_INNER;
		cameraActivate(camera);

		Menu::print("Swapped to %s camera\n", camera == CAM_INNER ? "inner" : "outer");
	}});

	mainMenu.addItem({"Exit                     START", KEY_START, [&camera](Menu &menu) {
		cameraDeactivate(camera);
		menu.exit();
	}});

	consoleClear();
	mainMenu.run();
}
