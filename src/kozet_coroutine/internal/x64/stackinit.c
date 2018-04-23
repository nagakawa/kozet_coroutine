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

extern int kcrReturnFromCoroutine;

void* kcrSetUpStack(void* base, void* returnAddress, void* userdata) {
  void** baseOfPointers = (void**) base;
  baseOfPointers[-1] = (void*) &kcrReturnFromCoroutine;
  baseOfPointers[-2] = returnAddress; // return address
  baseOfPointers[-4] = userdata;
  return baseOfPointers - 17; // 2 return addresses + 15 registers saved to stack
}
