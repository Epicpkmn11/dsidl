#include "wifi.h"

#include <dsiwifi9.h>
#include <lwip/sockets.h>

static void logPrintf(const char *str) { iprintf("\x1B[40m%s\x1B[47m", str); }

static void logNull(const char *str) { (void)str; }

void wifiInit(bool verbose) {
	DSiWifi_SetLogHandler(verbose ? logPrintf : logNull);
	DSiWifi_InitDefault(true);
}

bool wifiConnected() {
	u8 ipByte = DSiWifi_GetIP() >> 0x18;
	return ipByte != 0x00 && ipByte != 0xFF;
}
