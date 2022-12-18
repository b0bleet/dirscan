#define _GNU_SOURCE
#define _POSIX_C_SOURCE
#include <curl/curl.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <sys/sysinfo.h>
#include "link.h"
#include <pthread.h>
#define ERR -1
#define OK 0
#define HTTP_OK 200UL
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

#define __maybe_unused __attribute__((unused))

typedef struct {
  char *memaddr;
  size_t size;
  unsigned long int st_code;
} CurlMem; 

typedef struct {
  char* path;
} thread_path; 
