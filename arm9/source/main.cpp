#include "camera.h"
#include "download.h"
#include "quirc.h"
#include "script.h"
#include "version.h"
#include "wifi.h"

#include "overlay.h"

#include <dirent.h>
#include <fat.h>
#include <math.h>
#include <nds.h>
#include <stdio.h>

int main(int argc, char **argv) {
	consoleDemoInit();
	vramSetBankA(VRAM_A_MAIN_BG);
	videoSetMode(MODE_5_2D);
	int bg0Main = bgInit(0, BgType_Text4bpp, BgSize_T_256x256, 7, 0);
	int bg3Main = bgInit(3, BgType_Bmp16, BgSize_B16_256x256, 1, 0);

	dmaCopy(overlayPal, BG_PALETTE, overlayPalLen);
	decompress(overlayTiles, bgGetGfxPtr(bg0Main), LZ77Vram);
	decompress(overlayMap, bgGetMapPtr(bg0Main), LZ77Vram);
	REG_BLDCNT = BLEND_SRC_BG0 | BLEND_ALPHA | BLEND_DST_BG3;
	REG_BLDALPHA = 0x0A00;

	iprintf("dsidl " VER_NUMBER "\n");

	keysSetRepeat(25, 5);
	scanKeys();
	bool verbose = keysHeld() & KEY_SELECT;

	bool fatInited = fatInitDefault();
	if(!fatInited) {
		iprintf("FAT init failed.\n");
		while(1)
			swiWaitForVBlank();
	};

	// If not using no$gba, initialize Wi-Fi
	if(strncmp((const char *)0x4FFFA00, "no$gba", 6) != 0) {
		iprintf("Connecting to Wi-Fi...\n");
		iprintf("If this doesn't connect, ensure\nyour Wi-Fi is working in System\nSettings.\n\nIf it still doesn't work your\nrouter isn't compatible yet.\n\nHold SELECT while loading for\nverbose logging.\n\n");

		wifiInit(verbose);
		uint i = 0;
		while(!wifiConnected()) {
			swiWaitForVBlank();
			const char spinner[] = "-\\|/";
			iprintf("\x1B[23;31H%c", spinner[(i++ / 6) % sizeof(spinner)]);
		}
		consoleClear();
	}

	iprintf("Starting camera...\n");
	cameraInit();

	Camera camera = CAM_OUTER;
	cameraActivate(camera);

	consoleClear();
	iprintf("dsidl " VER_NUMBER "\n");
	iprintf("\nA to swap, L/R to scan QR\n");

	while(1) {
		u16 pressed;
		do {
			swiWaitForVBlank();
			if(!cameraTransferActive())
				cameraTransferStart(bgGetGfxPtr(bg3Main), CAPTURE_MODE_PREVIEW);
			scanKeys();
			pressed = keysDown();
		} while(!pressed);

		if(pressed & KEY_A) {
			// Wait for previous transfer to finish
			while(cameraTransferActive())
				swiWaitForVBlank();
			cameraTransferStop();

			// Switch camera
			camera = camera == CAM_INNER ? CAM_OUTER : CAM_INNER;
			cameraActivate(camera);

			iprintf("Swapped to %s camera\n", camera == CAM_INNER ? "inner" : "outer");
		} else if(pressed & (KEY_L | KEY_R)) {
			iprintf("Scanning for QR... ");

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
			iprintf("%s!\n", payload == "" ? "Not found" : "Done");

			if(payload != "") {
				runScript(payload, verbose);

				iprintf("================================");
				iprintf("dsidl " VER_NUMBER "\n");
				iprintf("\nA to swap, L/R to scan QR\n");
			}
		} else if(pressed & KEY_START) {
			// Disable camera so the light turns off
			cameraDeactivate(camera);

			return 0;
		}
	}
}
