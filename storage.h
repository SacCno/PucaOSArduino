#ifndef STORAGE_H
#define STORAGE_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

  bool storage_write(const char *path, const void *data, size_t size);
  bool storage_read(const char *path, void *data, size_t size);

#ifdef __cplusplus
}
#endif

#endif