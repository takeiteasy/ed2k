#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <openssl/md4.h>
#include "threads/threads.h"

#define CHUNK_SIZE 9728000
#define BUF_SIZE   8000

void* ed2k() {
}

int main (int argc, const char *argv[]) {
  long nprocessors = sysconf(_SC_NPROCESSORS_ONLN);

  thrd_t* t = malloc(nprocessors * sizeof(thrd_t*));
  return 0;
}
