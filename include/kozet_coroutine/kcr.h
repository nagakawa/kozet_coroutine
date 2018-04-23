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

#pragma once
#ifndef KOZET_COROUTINE_H
#define KOZET_COROUTINE_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct KCREntry {
  void (*callback)(void*); // The function to actually execute
  void* userdata; // Some data to pass to the function
  void* stack; // Stack base
  size_t stackSize; // Stack size
  void* stackPointer; // Stack pointer
  struct KCREntry* next; // Pointer to next coroutine that should be executed
  struct KCREntry* prev; // Pointer to previous coroutine
  // If free, these point to the next and previous free blocks.
  size_t size; // Size of free region (in KCREntry's) if free; otherwise 0
  size_t sizePrev; // Size of previous free region before this one
} KCREntry;

typedef struct KCRManager {
  KCREntry* queue;
  KCREntry* firstOccupied;
  KCREntry* firstFree;
  size_t size;
  void* outsideStackPointer;
} KCRManager;

KCRManager* kcrManagerCreate(void);
void kcrManagerDestroy(KCRManager* manager);
// Create a new KCREntry right before `node`
KCREntry* kcrManagerSpawn(
  KCRManager* manager,
  void (*callback)(void*),
  void* userdata,
  size_t stackSize);
void kcrManagerFreeEntry(KCRManager* manager, KCREntry* node);
void kcrManagerEnter(KCRManager* manager, KCREntry* node);
void kcrManagerExit(KCRManager* manager, KCREntry* node);
void kcrManagerYield(KCRManager* manager, KCREntry* node);

void kcrEnter(void);
void kcrExit(void);
void kcrYield(void);
void kcrSpawn(
  void (*callback)(void*),
  void* userdata,
  size_t stackSize);

#ifdef __cplusplus
}
// C++ wrapper code will go here
#endif

#endif // KOZET_COROUTINE_H
