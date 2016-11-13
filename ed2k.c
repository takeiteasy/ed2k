#include <stdio.h>
#include <string.h>
#include <openssl/md4.h>
#include "queue_t/queue.h"

#define CHUNK_SIZE 9728000
#define BUF_SIZE   8000
#define THREADS    4

void* ed2k(void* arg) {
  queue_t* q = (queue_t*)arg;
  queue_node_t* qn;
  while (q->nodes) {
    qn = queue_get(q);
    printf("%s\n", (char*)qn->data);
    thrd_yield();
    free(qn);
  }
  thrd_exit(0);
}

int main (int argc, const char *argv[]) {
  queue_t* q = queue_init();
  for (int i = 1; i < argc; ++i)
    queue_add(q, argv[i], sizeof(argv[i]));

  thrd_t* t = malloc(THREADS * sizeof(thrd_t*));
  for (int i = 0; i < THREADS; ++i)
    thrd_create(&t[i], ed2k, (void*)q);
  for (int i = 0; i < THREADS; ++i)
    thrd_join(t[i], NULL);

  return 0;
}
