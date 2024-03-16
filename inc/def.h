#ifndef DEF_H_
#define DEF_H_

#include <stdint.h>

typedef enum {
  FREE = 0,
  BUSY = 1
} access_t;

typedef enum {
  OK = 0,
  ERR = 1,
  SKIP = 2
} status_t;

#endif
