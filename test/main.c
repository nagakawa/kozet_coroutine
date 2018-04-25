#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <kozet_coroutine/kcr.h>

void producer(void* x) {
  volatile int* n = (volatile int*) x;
  for (size_t i = 0; i < 10; ++i) {
    n[0] = rand();
    printf("%d: Sent %d\n", n[1], n[0]);
    kcrYield();
  }
  n[0] = -1;
}
void producerExit(void* x) {
  volatile int* n = (volatile int*) x;
  printf("Producer #%d exited\n", n[1]);
}
void consumer(void* x) {
  volatile int* n = (volatile int*) x;
  while (n[0] != -1) {
    printf("%d: Received %d\n", n[1], n[0]);
    kcrYield();
  }
}
void consumerExit(void* x) {
  volatile int* n = (volatile int*) x;
  printf("Consumer #%d exited\n", n[1]);
}

int main(void) {
  srand(time(0));
  int things[5][2] = {
    {0, 1}, {0, 2}, {0, 3}, {0, 4}, {0, 5}
  };
  KCRManager* man = kcrManagerCreate();
  for (size_t i = 0; i < 5; ++i) {
    kcrManagerSpawnD(
      man,
      producer, things[(i * 2 + 1) % 5], 16384, producerExit);
  }
  for (size_t i = 0; i < 5; ++i) {
    kcrManagerSpawnD(
      man,
      consumer, things[(i * 3 + 4) % 5], 16384, consumerExit);
  }
  kcrManagerEnter(man, 0);
  kcrManagerDestroy(man);
  return 0;
}
