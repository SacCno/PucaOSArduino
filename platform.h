#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

  bool pucaos_platform_init(void);
  void printSerial(const char *fmt, ...);
  bool password_reader(const char *path, char *output, size_t output_size);

#ifdef __cplusplus
}
#endif

#endif