#ifndef DOWNLOAD_H
#define DOWNLOAD_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

int download(const char *url, const char *path, bool verbose);

#ifdef __cplusplus
}
#endif

#endif // DOWNLOAD_H
