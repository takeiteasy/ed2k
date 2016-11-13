#include "queue.h"

queue_t* queue_init() {
  queue_t* ret = (queue_t*)calloc(1, sizeof(queue_t));
  if (!ret)
    return NULL;

  mtx_init(&ret->mod_lock, mtx_plain);
  mtx_init(&ret->read_lock, mtx_plain);
  mtx_lock(&ret->read_lock);

  return ret;
}

void queue_add(queue_t* q, void* _data, size_t _size) {
  queue_node_t* qi = calloc(1, sizeof(queue_node_t));
  if (!qi)
    return;

  qi->data = _data;
  qi->size = _size;

  mtx_lock(&q->mod_lock);
  qi->next = q->nodes;
	q->nodes = qi;
  q->length++;

  mtx_unlock(&q->mod_lock);
  mtx_unlock(&q->read_lock);
}

queue_node_t* queue_get(queue_t* q) {
  queue_node_t *qi, *pqi;

  mtx_lock(&q->mod_lock);
  mtx_lock(&q->read_lock);

  if (!(qi = pqi = q->nodes)) {
    mtx_unlock(&q->mod_lock);
    return (queue_node_t*)NULL;
  }

  qi = pqi = q->nodes;
	while ((qi->next)) {
    pqi = qi;
		qi  = qi->next;
  }

  pqi->next = NULL;
	if (q->length <= 1)
		q->nodes = NULL;
	q->length--;

	if (q->length > 0)
		mtx_unlock(&q->read_lock);
	mtx_unlock(&q->mod_lock);

  return qi;
}
