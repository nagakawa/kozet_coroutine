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

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DEFAULT_STACK_SIZE 8192

typedef struct KCREntry {
  void (*callback)(void*); // The function to actually execute
  void (*tearDown)(void*); // Destructor to be called when it exits
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
  bool advanceOnExit;
} KCRManager;

KCRManager* kcrManagerCreate(void);
void kcrManagerDestroy(KCRManager* manager);
// Create a new KCREntry right before `node`
KCREntry* kcrManagerSpawn(
  KCRManager* manager,
  void (*callback)(void*),
  void* userdata,
  size_t stackSize);
KCREntry* kcrManagerSpawnD(
  KCRManager* manager,
  void (*callback)(void*),
  void* userdata,
  size_t stackSize,
  void (*tearDown)(void*));
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
void kcrSpawnD(
  void (*callback)(void*),
  void* userdata,
  size_t stackSize,
  void (*tearDown)(void*));

KCRManager* kcrManagerGetCurrent(void);
KCREntry* kcrEntryGetCurrent(void);

#ifdef __cplusplus
}
// C++ wrapper code will go here
#ifndef KCR_NO_CPP_WRAPPER

#include <tuple>
#include <utility>

extern "C" void kcrTeardownNull(void* userdata);

namespace kcr {
  template<typename F, typename T, size_t... I>
  auto apply2(F&& f, T&& t, std::index_sequence<I...> seq) {
    (void) seq;
    return f(std::get<I>(t)...);
  }
  template<typename F, typename T>
  auto apply(F&& f, T&& t) {
    return apply2(f, t,
      std::make_index_sequence<std::tuple_size<T>::value>());
  }
  template<typename UD>
  void proxyCallback(void* userdata) {
    UD* data = (UD*) userdata;
    auto&& callback = std::move(std::get<0>(*data));
    apply(callback, std::move(std::get<2>(*data)));
  }
  template<typename UD>
  void proxyDestroy(void* userdata) {
    UD* data = (UD*) userdata;
    auto&& callback = std::move(std::get<1>(*data));
    apply(callback, std::move(std::get<2>(*data)));
    delete data;
  }
  class Manager {
  public:
    class Entry {
    public:
      Entry(KCREntry* e) : ent(e) {}
      KCREntry* underlying() { return ent; }
      Entry(const KCREntry& e) = delete;
      Entry(KCREntry&& e) = delete;
    private:
      KCREntry* ent;
    };
    Manager() : man(kcrManagerCreate()) {}
    Manager(KCRManager* m) : man(m) {}
    ~Manager() { if (man != nullptr) kcrManagerDestroy(man); }
    Manager(const Manager& m) = delete;
    Manager(Manager&& m) : man(m.man) {
      m.man = nullptr;
    }
    Entry spawnRaw(
        void (*callback)(void*),
        void* userdata,
        size_t stackSize = DEFAULT_STACK_SIZE,
        void (*tearDown)(void*) = kcrTeardownNull
    ) {
      KCREntry* e = kcrManagerSpawnD(
        man, callback, userdata, stackSize, tearDown);
      return Entry(e);
    }
    template<typename F1, typename F2, typename... Args>
    Entry spawn(
        F1&& callback,
        F2&& tearDown,
        Args&&... args
    ) {
      using T = std::tuple<
        std::decay_t<F1>,
        std::decay_t<F2>,
        std::tuple<std::decay_t<Args>...>
      >;
      T* ud = new T(
        std::move(callback),
        std::move(tearDown),
        std::tuple<std::decay_t<Args>...>(std::forward<Args>(args)...)
      );
      return spawnRaw(
        proxyCallback<T>,
        ud,
        DEFAULT_STACK_SIZE,
        proxyDestroy<T>);
    }
    template<typename F1, typename... Args>
    Entry spawn(
        F1&& callback,
        Args&&... args
    ) {
      return spawn(
        callback,
        [](const Args&... /*args*/) { },
        std::forward<Args>(args)...);
    }
    void enter() {
      kcrManagerEnter(man, man->firstOccupied);
    }
    void enter(Entry e) {
      kcrManagerEnter(man, e.underlying());
    }
    void yield(Entry e) {
      kcrManagerYield(man, e.underlying());
    }
    void exit(Entry e) {
      kcrManagerExit(man, e.underlying());
    }
    void setAdvanceOnExit(bool e) {
      man->advanceOnExit = e;
    }
    KCRManager* underlying() { return man; }
  private:
    KCRManager* man;
  };
  void enter() {
    kcrEnter();
  }
  void exit() {
    kcrExit();
  }
  void yield() {
    kcrYield();
  }
  Manager getCurrentManager() {
    return Manager(kcrManagerGetCurrent());
  }
  Manager::Entry getCurrentEntry() {
    return Manager::Entry(kcrEntryGetCurrent());
  }
  void spawnRaw(
      void (*callback)(void*),
      void* userdata,
      size_t stackSize = DEFAULT_STACK_SIZE,
      void (*tearDown)(void*) = kcrTeardownNull
  ) {
    kcrSpawnD(callback, userdata, stackSize, tearDown);
  }
  template<typename F1, typename F2, typename... Args>
  void spawn(
      F1&& callback,
      F2&& tearDown,
      Args&&... args
  ) {
    getCurrentManager().spawn(
      callback, tearDown, std::forward<Args>(args)...);
  }
  template<typename F1, typename... Args>
  void spawn(
      F1&& callback,
      Args&&... args
  ) {
    getCurrentManager().spawn(
      callback, std::forward<Args>(args)...);
  }
}

#endif
#endif

#endif // KOZET_COROUTINE_H
