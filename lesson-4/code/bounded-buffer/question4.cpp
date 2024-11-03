/*
 * An implementation of the bounded-buffer problem
 *
 * NC producers and NC consumers communicate through a fifo.
 * The fifo has a fixed capacity.
 * NI items will be produced by the producers and consume by the consumers.
 * An item is composed of 2 equal integers, ranging from 1 to NI.
 */

#include <libgen.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "BufferPool.h"
#include "thread.h"
#include "utils.h"

bool verbose = false;
static void printUsage(FILE *fp, const char *cmd) {
  fprintf(fp,
          "Synopsis %s [options]\n"
          "\t----------+--------------------------------------------\n"
          "\t  Option  |          Description                       \n"
          "\t----------+--------------------------------------------\n"
          "\t -p num   | number of clients (dfl: 1)               \n"
          "\t -c num   | number of servers (dfl: 1)               \n"
          "\t -V       | verbose mode                               \n"
          "\t -h       | this help                                  \n"
          "\t----------+--------------------------------------------\n",
          cmd);
}

static SharedRegion *buffer;
struct ClientArgs {};
struct ServerArgs {};

void serverLifeCycle() {
  uint32_t id;
  char str[MAX_LEN];
  STATISTICS resp;
  while (true) {
    id = getPendingRequest(buffer);
    printf("id %d\n", id);
    resp = produceResponse(buffer, getRequestData(buffer, id));
    putResponseData(buffer, &resp, id);
    notifyClient(buffer, id);
  }
}

void *serverThread(void *arg) {
  serverLifeCycle();
  thread_exit(NULL);
  return NULL;
}

void clientLifeCycle() {
  uint32_t id;
  char str[MAX_LEN];
  str[0] = '0';
  str[1] = 'o';
  str[2] = 'l';
  str[3] = 'a';
  str[4] = '1';
  str[5] = '2';
  STATISTICS resp;
  while (true) {
    id = getFreeBuffer(buffer);
    printf("id %d\n", id);
    putRequestData(buffer, str, id);
    submitRequest(buffer, id);
    waitForResponse(buffer, id);
    printf("Reaching for the resposne");
    resp = getResponseData(buffer, id);
    printf("n_letters %d\n", resp.n_letters);
    releaseBuffer(buffer, id);
  }
}

void *clientThread(void *arg) {
  clientLifeCycle();
  thread_exit(NULL);
  return NULL;
}
ClientArgs a;
ServerArgs s;

int main(int argc, char *argv[]) {
  /* */
  uint32_t np = 1; /* number of producers */
  uint32_t nc = 1; /* number of consumers */

  /* command line arguments */
  const char *optstr = "i:p:c:Vh";

  int option;
  do {
    switch ((option = getopt(argc, argv, optstr))) {
    case 'p': // number of producers
      np = atoi(optarg);
      if (np < 1) {
        fprintf(stderr, "ERROR: invalid value (%d)\n", np);
        exit(1);
      }
      break;

    case 'c': // number of consumers
      nc = atoi(optarg);
      if (nc < 1) {
        fprintf(stderr, "ERROR: invalid value (%d)\n", nc);
        exit(1);
      }
      break;

    case 'V': // verbose mode
      verbose = true;
      break;

    case 'h':
      printUsage(stdout, basename(argv[0]));
      return 0;
      break;

    case -1:
      break;

    default:
      fprintf(stderr, "Not valid option\n");
      printUsage(stderr, basename(argv[0]));
      return EXIT_FAILURE;
    }
  } while (option >= 0);

  printf("Parameters: %d clients, %d servers.\n", np, nc);

  buffer = (SharedRegion *)mem_alloc(sizeof(SharedRegion));
  init_Buffer(buffer, 10);

  /* create the shared memory and init it as a fifo */
  /* launching child threads to play as consumers */
  pthread_t cthr[nc];
  for (uint32_t i = 0; i < nc; i++) {
    thread_create(&cthr[i], NULL, clientThread, &a);
  }

  /* launching child processes to play as producers */
  pthread_t pthr[np];
  for (uint32_t i = 0; i < np; i++) {
    thread_create(&pthr[i], NULL, serverThread, &s);
  }

  /* wait for producers to finish */
  for (uint32_t i = 0; i < np; i++) {
    thread_join(pthr[i], NULL);
    printf("Producer %u finished\n", i + 1);
  }

  /* wait for consumers to finish */
  for (uint32_t i = 0; i < nc; i++) {
    thread_join(cthr[i], NULL);
    printf("Consumer %u finished\n", i + 1);
  }

  /* destroy fifo */
  destroy_Buffer(buffer);

  return 0;
}
