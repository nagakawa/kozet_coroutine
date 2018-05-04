#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <kozet_coroutine/kcr.h>

void dummy(void* x) {
  (void) x;
  while (true) {
    kcrYield();
  }
}

void intercepter(void* x) {
  size_t n = *(size_t*) x;
  for (size_t i = 0; i < n; ++i) {
    kcrYield();
  }
  kcrExit();
}

int main(int argc, char** argv) {
  size_t iters = (argc >= 2) ? strtoul(argv[1], 0, 10) : 10000;
  size_t nDummies = (argc >= 3) ? strtoul(argv[2], 0, 10) : 10000;
  printf(
    "Testing coroutine performance with "
    "%zu dummy processes and %zu iterations\n",
    nDummies, iters
  );
  clock_t cl = clock();
  KCRManager* man = kcrManagerCreate();
  for (size_t i = 0; i < nDummies; ++i) {
    kcrManagerSpawn(man, dummy, 0, 8192);
  }
  kcrManagerSpawn(man, intercepter, &iters, 8192);
  kcrManagerEnter(man, 0);
  clock_t cl2 = clock();
  double t = (double) (cl2 - cl) / CLOCKS_PER_SEC;
  printf(
    "Took %f seconds, or %f ns per iteration per dummy coroutine\n",
    t, t / iters / nDummies * 1e9
  );
  kcrManagerDestroy(man);
}
