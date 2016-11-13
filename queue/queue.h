#ifndef __QUEUE_H__
#define __QUEUE_H__

#if __STDC_VERSION__ > 201112 && __STDC_NO_THREADS__ != 1
#include <threads.h>
#else
#include "../thread_t/threads.h"
#endif

typedef struct _q_node {
  void*  data;
  size_t size;
  struct _q_node* next;
} queue_node_t;

typedef struct _queue {
  queue_node_t* nodes;
  size_t length;
  mtx_t mod_lock, read_lock;
} queue_t;

queue_t* queue_init();
void queue_add(queue_t*, void*, size_t);
queue_node_t* queue_get(queue_t*);

#endif // __QUEUE_H__
