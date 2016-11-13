#include <stdio.h>
#include <openssl/md4.h>
#include "queue_t/queue.h"

#define CHUNK_SIZE 9728000
#define BUF_SIZE   8000
#define THREADS    4

void ed2k(void* arg) {
  queue_t* q = (queue_t*)arg;
  queue_node_t* qn;
  while (q->nodes) {
    qn = queue_get(q);
    char* fn = (char*)qn->data;

    FILE* fh   = fopen(fn, "rb");
    if (fh) {
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
      printf("%s|%ld|%s\n", fn, ftell(fh), result);

      free(result);
      fclose(fh);
    }
    thrd_yield();
    free(qn);
  }
  thrd_exit(0);
}

int main (int argc, const char *argv[]) {
  queue_t* q = queue_init();
  for (int i = 1; i < argc; ++i)
    queue_add(q, (void*)argv[i], sizeof(argv[i]));

  thrd_t* t = malloc(THREADS * sizeof(thrd_t*));
  for (int i = 0; i < THREADS; ++i)
    thrd_create(&t[i], ed2k, (void*)q);
  for (int i = 0; i < THREADS; ++i)
    thrd_join(t[i], NULL);

  return 0;
}
