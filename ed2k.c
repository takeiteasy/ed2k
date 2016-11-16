#include <stdio.h>
#include <math.h>
#include <string.h>
#include <openssl/md4.h>
#include <pthread.h>

#define CHUNK_SIZE 9728000
#define BUF_SIZE   8000

const char** files;
int next_file   = 0,
    total_files = 0;

void* ed2k() {
  int this_file;
  while (next_file < total_files) {
    this_file  = next_file;
    next_file += 1;
    FILE* fh   = fopen(files[this_file], "rb");

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
      printf("%s|%ld|%s\n", files[this_file], ftell(fh), result);

      free(result);
      fclose(fh);
    }
  }
  pthread_exit(NULL);
}

int main (int argc, const char *argv[]) {
  int total_threads = 8, i = 0, j = 0;
  const char** tmp_args = malloc(argc * sizeof(char*));
  for (i = 1, j = 0; i < argc; ++i) {
    if (argv[i][0] == '-' && argv[i][1] == '-') {
      if (!strcmp("--threads", argv[i])) {
        if ((i + 1) > (argc - 1))
          continue;

        total_threads = (unsigned)atoi(argv[++i]);
        if (total_threads <= 1)
          total_threads = 1;
      }
    } else {
      tmp_args[j] = malloc(strlen(argv[i]) * sizeof(char*));
      strcpy(tmp_args[j], argv[i]);
      j++;
    }
  }

  total_files = j;
  if (total_files < total_threads)
    total_threads = total_files;
  files       = malloc(j * sizeof(char*));
  memcpy(files, tmp_args, j * sizeof(char*));
  free(tmp_args);

  pthread_t threads[total_threads];
  for (i = 0; i < total_threads; ++i)
    pthread_create(&threads[i], NULL, ed2k, NULL);
  for (i = 0; i < total_threads; ++i)
    pthread_join(threads[i], NULL);

  return 0;
}
