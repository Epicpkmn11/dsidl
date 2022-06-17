#include "script.h"

#include "cJSON.h"
#include "download.h"
#include "fileBrowse.h"
#include "menu.h"

#include <nds.h>

#define SCRIPT_VERSION 1

static void downloadItem(std::string url) {
	Menu::print("================================");
	u16 key = Menu::prompt(KEY_A | KEY_B, "Download file?\nSource:\n%s\n\n(<A> Yes, <B> No)\n", url.c_str());

	if(key & KEY_A) {
		// Trim URL to just file name
		std::string fileName = url.substr(url.find_last_of('/') + 1);
		fileName = fileName.substr(0, fileName.find('?'));
		fileName = fileName.substr(0, fileName.find('#'));

		// Ask user to pick output
		std::string path = selectFile(fileName);
		if(path != "") {
			int ret = download(url.c_str(), path.c_str());
			Menu::printDelay(60, "\n%s\n", ret >= 0 ? "Download successful!" : "Download failed.");
		}
	}
}

static void runScriptInternal(const cJSON *json) {
	if(cJSON_IsArray(json)) {
		cJSON *item;
		cJSON_ArrayForEach(item, json) {
			if(cJSON_IsString(item)) {
				if(strncmp(item->valuestring, "http", 4) == 0)
					downloadItem(item->valuestring);
				else
					Menu::print("================================%s\n", item->valuestring);
			} else {
				Menu::printDelay(60, "Invalid JSON item.\n");
			}
		}
	} else if(cJSON_IsObject(json)) {
		cJSON *item;
		cJSON_ArrayForEach(item, json) {
			if(cJSON_IsString(item)) {
				if(strncmp(item->valuestring, "http", 4) == 0) {
					Menu::print("================================");
					u16 key = Menu::prompt(KEY_A | KEY_B, "Download %s?\nSource:\n%s\n\n(<A> Yes, <B> No)\n", item->string, item->valuestring);

					if(key & KEY_A) {
						if(item->string[0] != '/' && strncmp(item->string, "sd:/", 4) != 0)
							selectDir();

						int ret = download(item->valuestring, item->string);
						Menu::printDelay(60, "\n%s\n", ret >= 0 ? "Download successful!" : "Download failed.");
					}
				} else {
					Menu::print("================================%s\n", item->valuestring);
				}
			} else {
				Menu::printDelay(60, "Invalid JSON item:\n%s\n\n", item->string);
			}
		}
	} else {
		Menu::printDelay(60, "Invalid JSON.\n");
	}
}

void runScript(const std::string &payload) {
	cJSON *json = cJSON_Parse(payload.c_str());
	if(json) {
		Menu::print("Processing as JSON script...\n");

		// Check for external script reference
		if(cJSON_IsObject(json)) {
			cJSON *src = cJSON_GetObjectItemCaseSensitive(json, "dsidl:src");
			if(src) {
				if(cJSON_IsString(src) && strncmp(src->valuestring, "http", 4) == 0) {
					char *jsonBuffer = new char[1 << 20];
					int ret = downloadBuffer(src->valuestring, jsonBuffer, 1 << 20);
					if(ret == 0) {
						cJSON_Delete(json);
						json = cJSON_Parse(jsonBuffer);
						if(json) {
							cJSON *item = cJSON_GetObjectItemCaseSensitive(json, "dsidl");
							if(item && cJSON_IsNumber(item) && item->valueint == SCRIPT_VERSION) {
								item = cJSON_GetObjectItemCaseSensitive(json, "script");
								if(item)
									runScriptInternal(item);
							} else {
								Menu::printDelay(60, "Invalid script.\n");
							}
						} else {
							Menu::printDelay(60, "Failed to parse JSON.\n");
						}
					}

					delete[] jsonBuffer;
				} else {
					Menu::printDelay(60, "Invalid script source.\n");
				}

				cJSON_Delete(json);
				return;
			}
		}

		runScriptInternal(json);
		cJSON_Delete(json);
	} else if(payload.substr(0, 4) == "http") {
		downloadItem(payload.c_str());
	} else {
		Menu::print("QR content:\n%s\n", payload.c_str());
	}
}
