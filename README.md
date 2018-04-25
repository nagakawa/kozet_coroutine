## kozet_coroutine

A nice coroutine library with x64 assembly voodoo.

### Compatibility

Currently, the library assumes that you have an x86-64 processor and your
compiler uses the System V AMD64 ABI, which means that the library probably
won't work on Windows as is. (Modifying the library is left to the reader
as an exercise.)

C11 is required to build the library itself. The header can be included
under C99 or later, or under C++11 or later.

### Building

`make` will build both a static library (`build/libkozet_coroutine.a`) and a
test program (`build/test`). The header file is at
`include/kozet_coroutine/kcr.h`.

### Usage

Include the header file:

    #include <kozet_coroutine/kcr.h>

#### KCRManager

    // Create manager object
    KCRManager* kcrManagerCreate (void);
    // Destroy manager object
    void        kcrManagerDestroy(KCRManager* manager);

A KCRManager stores some number of coroutines. Most of the fields in this
struct won't be very useful, but the `firstOccupied` field holds the
KCREntry corresponding to the current coroutine.

#### KCREntry

    // Spawn a new coroutine
    KCREntry* kcrManagerSpawn(
      KCRManager* manager,
      void (*callback)(void*),
      void* userdata,
      size_t stackSize);
    // Like the above, but when the coroutine exits, tearDown is called
    // on userdata
    KCREntry* kcrManagerSpawnD(
      KCRManager* manager,
      void (*callback)(void*),
      void* userdata,
      size_t stackSize,
      void (*tearDown)(void*));
    // Like the above, but to be called inside a coroutine
    void      kcrSpawn(
      void (*callback)(void*),
      void* userdata,
      size_t stackSize);
    void      kcrSpawnD(
      void (*callback)(void*),
      void* userdata,
      size_t stackSize,
      void (*tearDown)(void*));

Creates a new coroutine to be executed immediately after the current one
(if any). `userdata` is the void pointer you want passed into `callback`.
`stackSize` is, unsurprisingly, the size of the stack reserved for the
coroutine. Be sure not to overflow it.

    // Enter a coroutine
    void kcrManagerEnter(KCRManager* manager, KCREntry* node);
    // Exit to the outside world
    void kcrManagerExit (KCRManager* manager, KCREntry* node);
    // Transfer control to the next coroutine of the manager
    void kcrManagerYield(KCRManager* manager, KCREntry* node);
    // These are similar, but they're meant to be called inside coroutines.
    void kcrEnter(void);
    void kcrExit (void);
    void kcrYield(void);

When a coroutine returns from the end of the callback, it will be purged
from the manager. If that happens, then control flow will continue to the
next coroutine if present; otherwise, it will transfer control to the
outside world.

#### Thread safety

Building the library with the `KCR_MULTITHREADED` flag will make certain
internal variables thread-local as to preserve reëntrancy. **Note that this
*does not* mean that you can share KCRManagers across multiple threads
without locking!**

### Licence

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
