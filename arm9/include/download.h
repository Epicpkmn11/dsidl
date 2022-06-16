#ifndef DOWNLOAD_H
#define DOWNLOAD_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

int download(const char *url, const char *path, bool verbose);
int downloadBuffer(const char *url, void *retBuffer, unsigned int size, bool verbose);

#ifdef __cplusplus
}
#endif

#endif // DOWNLOAD_H
