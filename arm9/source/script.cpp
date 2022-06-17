#include "script.h"

#include "cJSON.h"
#include "download.h"
#include "fileBrowse.h"
#include "menu.h"

#include <nds.h>

#define SCRIPT_VERSION 1

static void downloadItem(std::string url, std::string fileName = "") {
	Menu::print("================================");
	u16 key = Menu::prompt(KEY_A | KEY_B, "Download %s?\nSource:\n%s\n\n(<A> Yes, <B> No)\n", fileName == "" ? "file" : fileName.c_str(), url.c_str());

	if(key & KEY_A) {
		std::string path;
		if(fileName == "") {
			// Trim URL to just file name
			std::string fileName = url.substr(url.find_last_of('/') + 1);
			fileName = fileName.substr(0, fileName.find('?'));
			fileName = fileName.substr(0, fileName.find('#'));

			// Ask user to pick folder and ensure name is good
			path = selectDir();
			if(path == "")
				return;
			fileName = selectFile(fileName);
			if(fileName == "")
				return;
			path += fileName;
		} else if(!(fileName[0] == '/' || fileName.substr(0, 4) == "sd:/")) {
			path = selectDir();
			if(path == "")
				return;
			path += fileName;
		} else {
			path = fileName;
		}

		if(path != "") {
			int ret = download(url.c_str(), path.c_str());
			Menu::printDelay(60, "\n%s\n", ret >= 0 ? "Download successful!" : "Download failed.");
		}
	}
}

static void runScriptInternal(const cJSON *json) {
	if(cJSON_IsArray(json) || cJSON_IsObject(json)) {
		cJSON *item;
		cJSON_ArrayForEach(item, json) {
			if(cJSON_IsString(item)) {
				if(strncmp(item->valuestring, "http", 4) == 0)
					downloadItem(item->valuestring, item->string ? : "");
				else
					Menu::print("================================%s\n", item->valuestring);
			} else if(cJSON_IsObject(item)) {
				cJSON *msg = cJSON_GetObjectItemCaseSensitive(item, "msg");
				if(msg && cJSON_IsString(msg))
					Menu::print("================================%s\n", msg->valuestring);

				cJSON *overwrite = cJSON_GetObjectItemCaseSensitive(item, "overwrite");
				if(overwrite && cJSON_IsFalse(overwrite) && item->string && access(item->string, F_OK) == 0) {
					Menu::print("================================");
					Menu::printDelay(30, "Skipping %s,\nfile exists and set to not\noverwrite.\n", item->string);
					continue;
				}

				cJSON *url = cJSON_GetObjectItemCaseSensitive(item, "url");
				if(url && cJSON_IsString(url) && strncmp(url->valuestring, "http", 4) == 0)
					downloadItem(url->valuestring, item->string ? : "");
			} else {
				Menu::printDelay(60, "Invalid JSON item.\n");
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
		downloadItem(payload);
	} else {
		Menu::print("QR content:\n%s\n", payload.c_str());
	}
}
