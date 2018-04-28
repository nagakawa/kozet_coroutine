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
  # Save all registers on stack
  push rax
  push rdi
  push rcx
  push rdx
  push rbx
  push rbp
  push rsi
  push r8
  push r9
  push r10
  push r11
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
  pop r11
  pop r10
  pop r9
  pop r8
  pop rsi
  pop rbp
  pop rbx
  pop rdx
  pop rcx
  pop rdi
  pop rax
  ret
