#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <openssl/md4.h>
#include "queue_t/queue.h"
#include "../vector_t/vector.h"

#define CHUNK_SIZE 9728000
#define BUF_SIZE   4096
#define THREADS    4

// 4c565269678b60f1206151f130e647b9
// 2423660392

typedef struct {
  int       id, fh;
  size_t    fs;
  queue_t  *q1, *q2;
} hasher_arg;

typedef struct {
  int*     finished;
  queue_t* q;
} ed2k_arg;

typedef struct {
  size_t offset;
  int    id;
} chunk_t;

typedef struct {
  unsigned char* hash;
  int            id;
} hash_t;

void hasher(void* arg) {
  hasher_arg* a = (hasher_arg*)arg;
  queue_node_t* qn;
  while (a->q1->nodes) {
    qn = queue_get(a->q1);
    chunk_t* chunk = (chunk_t*)qn->data;
    size_t data_size = (chunk->offset + CHUNK_SIZE > a->fs ? a->fs - chunk->offset : CHUNK_SIZE);
    char* data = mmap(0, data_size, PROT_READ, MAP_SHARED, a->fh, chunk->offset);
    if (data == MAP_FAILED)
      exit(-1);

    unsigned char* md = (unsigned char*)malloc(MD4_DIGEST_LENGTH * sizeof(unsigned char));
    MD4_CTX root;
    MD4_Init(&root);
    MD4_Update(&root, data, data_size);
    MD4_Final(md, &root);

    hash_t* h = (hash_t*)malloc(sizeof(hash_t));
    h->hash   = md;
    h->id     = chunk->id;
    queue_add(a->q2, (void*)h, sizeof(h));

    thrd_yield();
    free(qn);
    free(chunk);
  }
  thrd_exit(0);
}

void ed2k(void* arg) {
  ed2k_arg* e = (ed2k_arg*)arg;
  queue_t* q = (queue_t*)e->q;
  queue_node_t* qn;
  vector_t* v = vector_init();
  int next = 0;
  MD4_CTX root;
  MD4_Init(&root);

  while (*e->finished || v->length) {
    while (q->nodes) {
      qn = queue_get(q);
      hash_t* h = (hash_t*)qn->data;
      if (h->id == next) {
        MD4_Update(&root, h->hash, MD4_DIGEST_LENGTH);
        printf("%d\n", h->id);
        ++next;
        free(h);
        free(qn);
      } else {
        vector_push(v, (void*)h);
      }
    }

    vector_for_each(v) {
      hash_t* tmp = (hash_t*)vector_get(v, i);
      if (tmp->id == next) {
        MD4_Update(&root, tmp->hash, MD4_DIGEST_LENGTH);
        printf("%d\n", tmp->id);
        ++next;
        vector_del(v, i);
      }
    }
  }

  unsigned char md [MD4_DIGEST_LENGTH];
  MD4_Final(md, &root);
  char* result = malloc(32 * sizeof(char*));
  int i        = 0;
  for(; i < MD4_DIGEST_LENGTH; ++i)
    sprintf(&result[i * 2], "%02x", (unsigned int)md[i]);
  printf("%s\n", result);

  vector_free(v);
  free(result);
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
  size_t f_size = s.st_size;

  queue_t* q1 = queue_init();
  size_t   i  = 0;
  int      j  = 0;
  for (; i < f_size; i += CHUNK_SIZE) {
    chunk_t* chunk = (chunk_t*)malloc(sizeof(chunk_t));
    chunk->offset  = i;
    chunk->id      = j++;
    queue_add(q1, (void*)chunk, sizeof(i));
  }

  // queue_node_t* qn;
  // while (q1->nodes) {
  //   qn = queue_get(q1);
  //   chunk_t* c = (chunk_t*)qn->data;
  //   printf("%lu -- %d\n", c->offset, c->id);
  //   free(qn);
  // }
  // exit(1);

  int running = 1;
  queue_t* q2  = queue_init();
  ed2k_arg* e  = (ed2k_arg*)malloc(sizeof(ed2k_arg));
  e->q         = q2;
  e->finished  = &running;
  thrd_t t1    = (thrd_t)malloc(sizeof(thrd_t));
  thrd_create(&t1, ed2k, (void*)e);
  thrd_t* t2  = (thrd_t*)malloc(THREADS * sizeof(thrd_t));

  for (int i = 0; i < THREADS; ++i) {
    hasher_arg* ha = (hasher_arg*)malloc(sizeof(hasher_arg));
    ha->id         = i;
    ha->fh         = fh;
    ha->fs         = f_size;
    ha->q1         = q1;
    ha->q2         = q2;
    thrd_create(&t2[i], hasher, (void*)ha);
  }
  for (int i = 0; i < THREADS; ++i)
    thrd_join(t2[i], NULL);
  running = 0;
  thrd_join(t1, NULL);

  free(t2);
  free(q1);
  free(q2);
  free(e);

  return 0;
}
