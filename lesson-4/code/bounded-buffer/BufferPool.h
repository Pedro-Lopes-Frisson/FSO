#ifndef BUFFER_POOL_H_2024
#define BUFFER_POOL_H_2024

#include "fifo.h"
#include <sys/types.h>

#define MAX_LEN 100

struct STATISTICS {
  uint32_t n_chars;
  uint32_t n_digits;
  uint32_t n_letters;
};
struct BUFFER {
  char req_data[MAX_LEN];
  STATISTICS resp_data;
  int isDone;
  pthread_mutex_t access;
  pthread_cond_t conds;
};

struct SharedRegion {
  Fifo freeBuffers;
  Fifo pendingRequests;
  BUFFER *pool;
};

void init_Buffer(SharedRegion *buffer, size_t size);
void destroy_Buffer(SharedRegion *buffer);

////////////////////////////////////////////////////
// Client Interface
uint32_t getFreeBuffer(SharedRegion *buffer);

void putRequestData(SharedRegion *buffer, const char *data, uint32_t id);

void submitRequest(SharedRegion *buffer, uint32_t id);

void waitForResponse(SharedRegion *buffer, uint32_t id);

STATISTICS getResponseData(SharedRegion *buffer, uint32_t id);

void releaseBuffer(SharedRegion *buffer, uint32_t id);

////////////////////////////////////////////////////
// Server Interface

uint32_t getPendingRequest(SharedRegion *buffer);

char *getRequestData(SharedRegion *buffer, uint32_t id);

STATISTICS produceResponse(SharedRegion *buffer, const char *req);

void putResponseData(SharedRegion *buffer, STATISTICS *resp, uint32_t id);

void notifyClient(SharedRegion *buffer, uint32_t id);

//

#endif // DEBUG
