/*
   Copyright 2018 AGC.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#ifdef __linux__
#define _GNU_SOURCE
#endif

#include "kozet_coroutine/kcr.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef __linux__
#include <sys/mman.h>
#endif

#ifdef KCR_MULTITHREADED
#define THREADLOCAL _Thread_local
#else
#define THREADLOCAL
#endif

#define INITIAL_QUEUE_SIZE 8

static THREADLOCAL KCRManager* currentKCRManager = 0;
static THREADLOCAL KCREntry* currentKCREntry = 0;

static bool inBounds(KCRManager* manager, KCREntry* entry) {
  return entry < manager->queue + manager->size;
}

void contextSwitch(void** oldStackPointer, void* newStackPointer);

#ifdef fuck
static void* allocateStack(size_t stackSize) {
  void* ptr = mmap(
    0, stackSize, PROT_READ | PROT_WRITE,
    MAP_PRIVATE | MAP_STACK | MAP_NORESERVE | MAP_GROWSDOWN | MAP_ANONYMOUS,
    -1, 0
  );
  if (ptr == MAP_FAILED) {
    perror("allocateStack");
    return 0;
  }
  return (char*) ptr + stackSize;
}
static void freeStack(void* stack, size_t stackSize) {
  munmap((char*) stack - stackSize, stackSize);
}
#else
static void* allocateStack(size_t stackSize) {
  void* mem = malloc(stackSize);
  return (void*) ((char*) mem + stackSize);
}
static void freeStack(void* stack, size_t stackSize) {
  free((char*) stack - stackSize);
}
#endif

void kcrReturnFromCoroutine(void);

void* kcrSetUpStack(
  void* base,
  void (*returnAddress)(void*),
  void* userdata);

KCRManager* kcrManagerCreate(void) {
  KCRManager* manager = (KCRManager*) malloc(sizeof(*manager));
  if (manager == 0) return 0;
  manager->queue = (KCREntry*) malloc(INITIAL_QUEUE_SIZE * sizeof(KCREntry));
  manager->firstOccupied = 0;
  manager->size = INITIAL_QUEUE_SIZE;
  manager->firstFree = manager->queue;
  manager->firstFree->size = INITIAL_QUEUE_SIZE;
  manager->firstFree->sizePrev = 0;
  manager->firstFree->next = manager->firstFree;
  manager->firstFree->prev = manager->firstFree;
  for (size_t i = 1; i < INITIAL_QUEUE_SIZE; ++i) {
    manager->queue[i].size = 0; // Prevent erroneous coälescing
    manager->queue[i].sizePrev = 0;
  }
  return manager;
}

/*
static void debugPrintOccupiedList(KCRManager* man) {
  printf("Occupied:\n");
  KCREntry* e = man->firstOccupied;
  if (e == 0) return;
  KCREntry* f = e;
  do {
    printf("Node at %p (size = %zu, prevSize = %zu)\n", (void*) e, e->size, e->sizePrev);
    e = e->next;
  } while (e != f);
}

static void debugPrintFreeList(KCRManager* man) {
  printf("Free:\n");
  KCREntry* e = man->firstFree;
  if (e == 0) return;
  KCREntry* f = e;
  do {
    printf("Node at %p (size = %zu, prevSize = %zu)\n", (void*) e, e->size, e->sizePrev);
    e = e->next;
  } while (e != f);
}
*/

static void freeStacksOfOccupiedEntries(KCRManager* man) {
  KCREntry* e = man->firstOccupied;
  if (e == 0) return;
  KCREntry* f = e;
  do {
    freeStack(e->stack, e->stackSize);
    e = e->next;
  } while (e != f);
}

void kcrManagerDestroy(KCRManager* manager) {
  freeStacksOfOccupiedEntries(manager);
  free(manager->queue);
  free(manager);
}

static KCREntry* kcrManagerGetFreeEntry(KCRManager* manager, KCREntry** node) {
  KCREntry* e = manager->firstFree;
  if (e == 0) {
    // Get old pointer
    KCREntry* q = manager->queue;
    // No free space left; double capacity
    manager->queue = (KCREntry*) realloc(
      manager->queue, 2 * manager->size * sizeof(KCREntry));
    off_t offset = (char*) manager->queue - (char*) q;
    for (size_t i = manager->size; i < 2 * manager->size; ++i) {
      manager->queue[i].size = 0; // Prevent erroneous coälescing
      manager->queue[i].sizePrev = 0;
    }
    if (offset != 0) {
      for (size_t i = 0; i < 2 * manager->size; ++i) {
        // Update pointers
        KCREntry* ep = manager->queue + i;
        if (ep->next != 0)
          ep->next = (KCREntry*) ((char*) ep->next + offset);
        if (ep->prev != 0)
          ep->prev = (KCREntry*) ((char*) ep->prev + offset);
      }
      manager->firstOccupied =
        (KCREntry*) ((char*) manager->firstOccupied + offset);
      *node =
        (KCREntry*) ((char*) *node + offset);
    }
    // Create a new free block
    manager->firstFree = manager->queue + manager->size;
    manager->firstFree->size = manager->size;
    manager->firstFree->sizePrev = 1; // since all other entries are taken
    manager->firstFree->next = manager->firstFree;
    manager->firstFree->prev = manager->firstFree;
    // Update size
    manager->size *= 2;
    e = manager->firstFree;
  }
  assert(inBounds(manager, e));
  if (e->size == 1) {
    // Remove this block altogether
    if (e == e->next) {
      // Only free block remaining
      manager->firstFree = 0;
    } else {
      e->next->prev = manager->firstFree->prev;
      e->prev->next = manager->firstFree->next;
      if (e == manager->firstFree)
        manager->firstFree = e->next;
    }
  } else {
    // Shave off the first entry
    KCREntry* f = e + 1;
    assert(inBounds(manager, f));
    f->size = e->size - 1;
    f->next = e == e->next ? f : e->next;
    f->prev = e == e->prev ? f : e->prev;
    f->sizePrev = 1;
    if (e == manager->firstFree)
      manager->firstFree = f;
  }
  e->size = 0;
  return e;
}

static KCREntry* kcrManagerAddEntry(KCRManager* manager, KCREntry* node) {
  if (node == 0) node = manager->firstOccupied;
  KCREntry* newNode = kcrManagerGetFreeEntry(manager, &node);
  if (manager->firstOccupied == 0) {
    // List is empty; make the node point to itself
    newNode->next = newNode;
    newNode->prev = newNode;
    manager->firstOccupied = newNode;
  } else {
    newNode->next = node;
    newNode->prev = node->prev;
    newNode->next->prev = newNode;
    newNode->prev->next = newNode;
  }
  return newNode;
}

void kcrManagerFreeEntry(KCRManager* manager, KCREntry* node) {
  // Do cleanup
  freeStack(node->stack, node->stackSize);
  // Remove from occupied list
  if (node == node->next) {
    // Last node in list; delete altogether
    manager->firstOccupied = 0;
  } else {
    // Stitch
    node->next->prev = node->prev;
    node->prev->next = node->next;
    // Was the firstOccupied node deleted?
    if (node == manager->firstOccupied)
      manager->firstOccupied = node->next;
  }
  // Add to free list
  node->size = 1;
  // Can we coälesce to the right?
  bool coalesced = false;
  KCREntry* succ = node + 1;
  if (inBounds(manager, succ) && succ->size != 0) {
    node->size = succ->size + 1;
    succ->prev->next = node;
    succ->next->prev = node;
    node->prev = succ->prev;
    node->next = succ->next;
    // Remember to update the next entry
    KCREntry* nextBlock = succ + succ->size;
    if (inBounds(manager, nextBlock)) {
      nextBlock->sizePrev = node->size;
    }
    if (manager->firstFree == succ)
      manager->firstFree = node;
    coalesced = true;
  }
  // What about to the left?
  if (node->sizePrev != 0) {
    KCREntry* prevBlock = node - node->sizePrev;
    assert(prevBlock >= manager->queue);
    if (prevBlock->size != 0) {
      // Prev block's size is increased by this block's
      prevBlock->size += node->size;
      // Next block also notes the new sizePrev
      KCREntry* nextBlock = node + node->size;
      if (inBounds(manager, nextBlock)) {
        nextBlock->sizePrev = prevBlock->size;
      }
      node->size = 0;
      // If already coälesced to the right, delete this block from free list
      if (coalesced) {
        node->prev->next = node->next;
        node->next->prev = node->prev;
        if (manager->firstFree == node)
          manager->firstFree = node->next;
      }
      coalesced = true;
    }
  }
  if (!coalesced) {
    // Add new entry to free list
    if (manager->firstFree == 0) {
      manager->firstFree = node;
      node->next = node;
      node->prev = node;
    } else {
      node->prev = manager->firstFree->prev;
      node->next = manager->firstFree;
      node->prev->next = node;
      node->next->prev = node;
    }
  }
}

KCREntry* kcrManagerSpawn(
    KCRManager* manager,
    void (*callback)(void*),
    void* userdata,
    size_t stackSize) {
  KCREntry* e = kcrManagerAddEntry(
    manager,
    manager->firstOccupied ? manager->firstOccupied->next : 0
  );
  e->callback = callback;
  e->userdata = userdata;
  e->stackSize = stackSize;
  e->stack = allocateStack(stackSize);
  e->stackPointer = kcrSetUpStack(e->stack, e->callback, e->userdata);
  return e;
}

void kcrManagerExit(KCRManager* manager, KCREntry* node) {
  // Prepare to enter next coroutine (if any)
  if (manager->firstOccupied != 0) {
    manager->firstOccupied = manager->firstOccupied->next;
  }
  contextSwitch(&(node->stackPointer), manager->outsideStackPointer);
}

void kcrReturnFromCoroutine(void) {
  // Called when a coroutine returns.
  // Remove from list
  KCREntry* next = currentKCREntry->next;
  kcrManagerFreeEntry(currentKCRManager, currentKCREntry);
  // If there are no coroutines left, return to outside
  if (currentKCRManager->firstOccupied == 0) {
    kcrManagerExit(currentKCRManager, currentKCREntry);
  }
  // Switch contexts
  currentKCREntry = next;
  currentKCRManager->firstOccupied = next;
  void* sink;
  contextSwitch(&sink, next->stackPointer);
}

void kcrManagerEnter(KCRManager* manager, KCREntry* node) {
  if (node == 0) node = manager->firstOccupied;
  // Set global vars for use inside coroutines
  currentKCRManager = manager;
  currentKCREntry = node;
  // Switch stacks
  contextSwitch(&(manager->outsideStackPointer), node->stackPointer);
}

void kcrManagerYield(KCRManager* manager, KCREntry* node) {
  // Switch to next coroutine in manager while not outright
  // ending the current one
  if (node == node->next) return;
  manager->firstOccupied = node->next;
  currentKCREntry = node->next;
  contextSwitch(
    &(node->stackPointer),
    node->next->stackPointer
  );
}

void kcrEnter(void) {
  kcrManagerEnter(currentKCRManager, currentKCREntry);
}

void kcrExit(void) {
  kcrManagerExit(currentKCRManager, currentKCREntry);
}

void kcrYield(void) {
  kcrManagerYield(currentKCRManager, currentKCREntry);
}

void kcrSpawn(
    void (*callback)(void*),
    void* userdata,
    size_t stackSize) {
  kcrManagerSpawn(currentKCRManager, callback, userdata, stackSize);
}
