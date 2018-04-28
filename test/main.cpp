#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <iostream>

#include <kozet_coroutine/kcr.h>

int main() {
  srand(time(0));
  int things[5][2] = {
    {0, 1}, {0, 2}, {0, 3}, {0, 4}, {0, 5}
  };
  kcr::Manager man;
  for (size_t i = 0; i < 5; ++i) {
    volatile int* thing = things[(i * 2 + 1) % 5];
    man.spawn(
      [thing] () {
        for (size_t j = 0; j < 10; ++j) {
          thing[0] = rand();
          printf("%d: Sent %d\n", thing[1], thing[0]);
          kcr::yield();
        }
        thing[0] = -1;
      },
      [thing] () {
        printf("Producer #%d exited\n", thing[1]);
      }
    );
  }
  for (size_t i = 0; i < 5; ++i) {
    volatile int* thing = things[(i * 3 + 4) % 5];
    man.spawn(
      [] (volatile int* thing) {
        while (thing[0] != -1) {
          printf("%d: Received %d\n", thing[1], thing[0]);
          kcr::yield();
        }
      },
      [] (volatile int* thing) {
        printf("Consumer #%d exited\n", thing[1]);
      },
      thing
    );
  }
  man.enter();
  return 0;
}
