#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <openssl/md4.h>
#include "queue_t/queue.h"

#define CHUNK_SIZE 9728000
#define BUF_SIZE   8000
#define THREADS    4

// 4c565269678b60f1206151f130e647b9
// 2423660392

typedef struct {
  int id, fh;
  queue_t* q;
} worker_arg;

typedef struct {
  size_t offset;
  int    id;
} chunk_t;

void worker(void* arg) {
  worker_arg* a = (worker_arg*)arg;
  queue_node_t* qn;
  while (a->q->nodes) {
    qn = queue_get(a->q);
    chunk_t* chunk = (chunk_t*)qn->data;
    char* data = mmap(0, CHUNK_SIZE, PROT_READ, MAP_SHARED, a->fh, chunk->offset);
    if (data == MAP_FAILED)
      exit(-1);

    unsigned char md[MD4_DIGEST_LENGTH];
    MD4_CTX root;
    MD4_Init(&root);
    MD4_Update(&root, data, CHUNK_SIZE);
    MD4_Final(md, &root);

    char* result = malloc(32 * sizeof(char*));
    for (int i = 0; i < MD4_DIGEST_LENGTH; ++i)
      sprintf(&result[i * 2], "%02x", (unsigned int)md[i]);
    printf("%s %d %d\n", result, a->id, chunk->id);

    thrd_yield();
    free(qn);
    free(chunk);
  }
  thrd_exit(0);
}

int main() {
  char* test = "/Users/rusty/Downloads/DANDY-510.avi.mp4";
  int fh     = open(test, O_RDONLY);
  if (fh < 0)
    return -1;

  struct stat s;
  int status = fstat (fh, &s);
  if (status < 0)
    return -1;
  size_t size = s.st_size;

  queue_t* q = queue_init();
  size_t   i = 0;
  int      j = 0;
  for (; i < size; i += CHUNK_SIZE) {
    chunk_t* chunk = (chunk_t*)malloc(sizeof(chunk_t));
    chunk->offset = i;
    chunk->id     = j++;
    queue_add(q, (void*)chunk, sizeof(i));
  }

  thrd_t* t = (thrd_t*)malloc(THREADS * sizeof(thrd_t));
  for (int i = 0; i < THREADS; ++i) {
    worker_arg* arg = (worker_arg*)malloc(sizeof(worker_arg));
    arg->id = i;
    arg->fh = fh;
    arg->q  = q;
    thrd_create(&t[i], worker, (void*)arg);
  }
  for (int i = 0; i < THREADS; ++i)
    thrd_join(t[i], NULL);

  return 0;
}
