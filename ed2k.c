#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <openssl/md4.h>
#include <pthread.h>

#define CHUNK_SIZE 9728000
#define BUF_SIZE   8000
#if !defined(THREADS)
#define THREADS 8
#endif

typedef struct _q_node {
  void*  data;
  size_t size;
  struct _q_node* next;
} queue_node_t;

typedef struct _queue {
  queue_node_t* nodes;
  size_t length;
  pthread_mutex_t mod_lock, read_lock;
} queue_t;

void queue_init(queue_t* q) {
  q->length = 0;
  q->nodes  = NULL;

  
  pthread_mutex_init(&q->mod_lock, NULL);
  pthread_mutex_init(&q->read_lock, NULL);
  pthread_mutex_lock(&q->read_lock);
}

void queue_add(queue_t* q, void* _data, size_t _size) {
  queue_node_t* qn = calloc(1, sizeof(queue_node_t));
  if (!qn)
    return;

  qn->data = _data;
  qn->size = _size;

  pthread_mutex_lock(&q->mod_lock);
  qn->next = q->nodes;
  q->nodes = qn;
  q->length++;

  pthread_mutex_unlock(&q->mod_lock);
  pthread_mutex_unlock(&q->read_lock);
}

queue_node_t* queue_get(queue_t* q) {
  queue_node_t *qn, *pqn;

  pthread_mutex_lock(&q->mod_lock);
  pthread_mutex_lock(&q->read_lock);

  if (!(qn = pqn = q->nodes)) {
    pthread_mutex_unlock(&q->mod_lock);
    return (queue_node_t*)NULL;
  }

  qn = pqn = q->nodes;
  while ((qn->next)) {
    pqn = qn;
    qn  = qn->next;
  }

  pqn->next = NULL;
  if (q->length <= 1)
    q->nodes = NULL;
  q->length--;

  if (q->length > 0)
    pthread_mutex_unlock(&q->read_lock);
  pthread_mutex_unlock(&q->mod_lock);

  return qn;
}

void ed2k(void* _q) {
  queue_t* q = (queue_t*)_q;
  queue_node_t* qn;
  while(q->nodes) {
    qn = queue_get(q);
    char* fn = (char*)qn->data;
    FILE* fh = fopen(fn, "rb");
    if (!fh) {
      printf("ERROR: Failed to open \"%s\"\n", fn);
      free(qn);
      continue;
    }

    fseek(fh, 0L, SEEK_END);
    size_t fh_size = ftell(fh);
    rewind(fh);
    int too_small  = (fh_size < CHUNK_SIZE);

    unsigned char buf[BUF_SIZE];
    unsigned char md [MD4_DIGEST_LENGTH];
    MD4_CTX root;
    MD4_CTX chunk;
    MD4_Init(&root);
    MD4_Init(&chunk);

    size_t cur_len   = 0,
           len       = 0,
           cur_chunk = 0,
           cur_buf   = 0;
    while (fread(buf, sizeof(*buf), BUF_SIZE, fh) > 0) {
      len        = ftell(fh);
      cur_buf    = len - cur_len;
      MD4_Update(&chunk, buf, cur_buf);
      cur_len    = len;
      cur_chunk += BUF_SIZE;

      if (cur_chunk == CHUNK_SIZE && cur_buf == BUF_SIZE) {
        cur_chunk = 0;
        MD4_Final(md, &chunk);
        MD4_Init(&chunk);
        MD4_Update(&root, md, MD4_DIGEST_LENGTH);
      }
    }
    MD4_Final(md, &chunk);

    if (!too_small) {
      MD4_Update(&root, md, MD4_DIGEST_LENGTH);
      MD4_Final(md, &root);
    }

    char* result = malloc(32 * sizeof(char*));
    int i        = 0;
    for(; i < MD4_DIGEST_LENGTH; ++i)
      sprintf(&result[i * 2], "%02x", (unsigned int)md[i]);
    printf("ed2k://|file|%s|%lu|%s|\n", fn, ftell(fh), result);

    free(result);
    fclose(fh);
    free(qn);
  }
}

int main (int argc, const char *argv[]) {
  int i, total_threads = THREADS, total_files = argc - 1;
  if (!total_files) {
    printf("Nothing to do!\n");
    return 1;
  }
  if (total_files < total_threads)
    total_threads = total_files;
  queue_t q;

  queue_init(&q);
  for (i = 1; i < argc; ++i)
    queue_add(&q, (void*)argv[i], sizeof(argv[i]));
  
  pthread_t threads[total_threads];
  for (i = 0; i < total_threads; ++i)
    pthread_create(&threads[i], NULL, ed2k, (void*)&q);
  for (i = 0; i < total_threads; ++i)
    pthread_join(threads[i], NULL);

  free(q.nodes);
  return 0;
}
