#include "camera.h"
#include "download.h"
#include "fileBrowse.h"
#include "quirc.h"
#include "version.h"
#include "wifi.h"

#include <dirent.h>
#include <fat.h>
#include <math.h>
#include <nds.h>
#include <stdio.h>

int main(int argc, char **argv) {
	consoleDemoInit();
	vramSetBankA(VRAM_A_MAIN_BG);
	videoSetMode(MODE_5_2D);
	int bg3Main = bgInit(3, BgType_Bmp16, BgSize_B16_256x256, 0, 0);

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

			std::string url;

			if(quirc_count(q) > 0) {
				struct quirc_code code;
				struct quirc_data data;
				quirc_extract(q, 0, &code);
				if(!quirc_decode(&code, &data)) {
					url = (const char *)data.payload;
				}
			}

			quirc_destroy(q);
			free(buffer);
			iprintf("%s!\n", url == "" ? "Not found" : "Done");

			if(url != "") {
				// Trim URL to just file name
				std::string fileName = url.substr(url.find_last_of('/') + 1);
				fileName = fileName.substr(0, fileName.find('?'));
				fileName = fileName.substr(0, fileName.find('#'));

				std::string path = selectFile(fileName);

				if(path != "|cancel|") {
					int ret = download(url.c_str(), path.c_str(), verbose);
					if(ret >= 0)
						iprintf("%s\ndownloaded successfully!\n", path.c_str());
					else
						iprintf("Download failed.\n");

					iprintf("================================");
					iprintf("dsidl " VER_NUMBER "\n");
					iprintf("\nA to swap, L/R to scan QR\n");
				}
			}
		} else if(pressed & KEY_START) {
			// Disable camera so the light turns off
			cameraDeactivate(camera);

			return 0;
		}
	}
}
