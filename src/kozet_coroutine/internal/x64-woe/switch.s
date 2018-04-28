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

.intel_syntax noprefix

.text
.global contextSwitch
contextSwitch:
  # rcx = where to stash the old stack pointer
  # rdx = stack pointer to switch to
  # Saved stack pointer locations point to the last entry on the stack.
  # Save all callee-saved registers on stack, plus rcx
  # (because we want the coroutine to get it as the userdata argument)
  # (rsp doesn't need to be saved; it is stored elsewhere)
  push rcx
  push rbx
  push rbp
  push rdi
  push rsi
  push r12
  push r13
  push r14
  push r15
  # Save rsp
  mov QWORD PTR [rcx], rsp
  # Switch to new rsp
  mov rsp, rdx
  # Restore registers
  pop r15
  pop r14
  pop r13
  pop r12
  pop rsi
  pop rdi
  pop rbp
  pop rbx
  pop rcx
  ret
