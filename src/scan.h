#include <curl/curl.h>
#include <errno.h>
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

struct CurlMem {
  char *memaddr;
  size_t size;
  unsigned long int st_code;
} typedef CurlMem;

struct thread_path {
  char* path;
} typedef thread_path;
