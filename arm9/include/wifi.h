#ifndef WIFI_H
#define WIFI_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

void wifiInit(bool verbose);
bool wifiConnected(void);

#ifdef __cplusplus
}
#endif

#endif // WIFI_H
