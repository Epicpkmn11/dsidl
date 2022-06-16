#include "script.h"

#include "cJSON.h"
#include "download.h"
#include "fileBrowse.h"

#include <nds.h>

static void downloadItem(std::string url, bool verbose) {
	iprintf("================================");
	iprintf("Download file?\nSource:\n%s\n\n(<A> Yes, <B> No)\n", url.c_str());
	u16 down;
	do {
		swiWaitForVBlank();
		scanKeys();
		down = keysDown();
	} while(!(down & (KEY_A | KEY_B)));

	if(down & KEY_A) {
		// Trim URL to just file name
		std::string fileName = url.substr(url.find_last_of('/') + 1);
		fileName = fileName.substr(0, fileName.find('?'));
		fileName = fileName.substr(0, fileName.find('#'));

		// Ask user to pick output
		std::string path = selectFile(fileName);
		if(path != "|cancel|") {
			int ret = download(url.c_str(), path.c_str(), verbose);
			if(ret >= 0)
				iprintf("%s\ndownloaded successfully!\n", path.c_str());
			else
				iprintf("Download failed.\n");
			for(int i = 0; i < 60; i++)
				swiWaitForVBlank();
		}
	}
}

void runScript(const std::string &payload, bool verbose) {
	cJSON *json = cJSON_Parse(payload.c_str());
	if(json) {
		iprintf("Processing as JSON script...\n");
		if(cJSON_IsArray(json)) {
			cJSON *item;
			cJSON_ArrayForEach(item, json) {
				if(cJSON_IsString(item)) {
					if(strncmp(item->valuestring, "http", 4) == 0)
						downloadItem(item->valuestring, verbose);
					else
						iprintf("================================%s\n", item->valuestring);
				} else {
					iprintf("Invalid JSON item.\n");
					for(int i = 0; i < 60; i++)
						swiWaitForVBlank();
				}
			}
		} else if(cJSON_IsObject(json)) {
			cJSON *item;
			cJSON_ArrayForEach(item, json) {
				if(cJSON_IsString(item)) {
					if(strncmp(item->valuestring, "http", 4) == 0) {
						iprintf("================================");
						iprintf("Download %s?\nSource:\n%s\n\n(<A> Yes, <B> No)\n", item->string, item->valuestring);
						u16 down;
						do {
							swiWaitForVBlank();
							scanKeys();
							down = keysDown();
						} while(!(down & (KEY_A | KEY_B)));

						if(down & KEY_A) {
							int ret = download(item->valuestring, item->string, verbose);
							if(ret >= 0)
								iprintf("%s\ndownloaded successfully!\n", item->string);
							else
								iprintf("Download failed.\n");
							for(int i = 0; i < 60; i++)
								swiWaitForVBlank();
						}
					} else {
						iprintf("================================%s\n", item->valuestring);
					}
				} else {
					iprintf("Invalid JSON item:\n%s\n\n", item->string);
					for(int i = 0; i < 60; i++)
						swiWaitForVBlank();
				}
			}
		} else {
			iprintf("Invalid JSON.\n");
			for(int i = 0; i < 60; i++)
				swiWaitForVBlank();
		}

		cJSON_Delete(json);
	} else if(payload.substr(0, 4) == "http") {
		downloadItem(payload.c_str(), verbose);
	} else {
		iprintf("QR content:\n%s\n", payload.c_str());
	}
}