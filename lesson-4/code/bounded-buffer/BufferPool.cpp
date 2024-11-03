#include "BufferPool.h"
#include "fifo.h"
#include "thread.h"
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>

static size_t saved_size;

void init_Buffer(SharedRegion *buffer, size_t size) {
  saved_size = size;
  fifoInit(&(buffer->freeBuffers));
  fifoInit(&(buffer->pendingRequests));

  buffer->pool = (BUFFER *)malloc(sizeof(BUFFER) * size);

  /* init access mutex */
  Item item;
  for (size--; size > 0; size--) {
    cond_init(&(buffer->pool[size].conds), NULL);
    mutex_init(&(buffer->pool[size].access), NULL);
    item.id = size;
    fifoInsert(&(buffer->freeBuffers), item);
  }
}
void destroy_Buffer(SharedRegion *buffer) {
  for (size_t size = saved_size; size > 0; size--) {
    cond_destroy(&(buffer->pool[size].conds));
  }
  free(&(buffer->pool));
  saved_size = 0;
}
////////////////////////////////////////////////////
// Client Interface
uint32_t getFreeBuffer(SharedRegion *buffer) {
  Item i = fifoRetrieve(&buffer->freeBuffers);
  return i.id;
}

void putRequestData(SharedRegion *buffer, const char *data, uint32_t id) {
  mutex_lock(&buffer->pool[id].access);
  memcpy(buffer->pool[id].req_data, data, MAX_LEN);
  mutex_unlock(&buffer->pool[id].access);
}

void submitRequest(SharedRegion *buffer, uint32_t id) {
  Item i;
  i.id = id;
  fifoInsert(&buffer->pendingRequests, i);
  buffer->pool[id].isDone = 0;
}

void waitForResponse(SharedRegion *buffer, uint32_t id) {
  mutex_lock(&buffer->pool[id].access);
  printf("ola\n");
  while (buffer->pool[id].isDone != 1) {
    cond_wait(&buffer->pool[id].conds, &buffer->pool[id].access);
  }
  printf("xau\n");
  mutex_unlock(&buffer->pool[id].access);
}

STATISTICS getResponseData(SharedRegion *buffer, uint32_t id) {
  printf("Hello getResponseData\n");
  mutex_lock(&buffer->pool[id].access);
  printf("Got Lock \n");
  STATISTICS stats = buffer->pool[id].resp_data;
  mutex_unlock(&buffer->pool[id].access);
  return stats;
}

void releaseBuffer(SharedRegion *buffer, uint32_t id) {
  mutex_lock(&buffer->pool[id].access);
  Item item;
  item.id = id;
  fifoInsert(&buffer->freeBuffers, item);
  mutex_unlock(&buffer->pool[id].access);
}

////////////////////////////////////////////////////
// Server Interface

uint32_t getPendingRequest(SharedRegion *buffer) {
  Item item;
  item = fifoRetrieve(&buffer->pendingRequests);
  return item.id;
}

char *getRequestData(SharedRegion *buffer, uint32_t id) {
  mutex_lock(&buffer->pool[id].access);
  char *req = buffer->pool[id].req_data;
  mutex_unlock(&buffer->pool[id].access);
  return req;
}

STATISTICS produceResponse(SharedRegion *buffer, const char *req) {
  STATISTICS s;
  s.n_chars = 100;
  s.n_digits = 101;
  s.n_letters = 102;
  return s;
}

void putResponseData(SharedRegion *buffer, STATISTICS *resp, uint32_t id) {
  mutex_lock(&buffer->pool[id].access);
  buffer->pool[id].resp_data = *resp;
  mutex_unlock(&buffer->pool[id].access);
}

void notifyClient(SharedRegion *buffer, uint32_t id) {
  mutex_lock(&buffer->pool[id].access);
  printf("Notified %d\n", id);
  buffer->pool[id].isDone = 1;
  mutex_unlock(&buffer->pool[id].access);
  cond_broadcast(&buffer->pool[id].conds);
}
