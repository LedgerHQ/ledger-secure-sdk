.syntax unified
.text
.thumb

.section .text._shared_trampoline
.thumb_func
.global _shared_trampoline
_shared_trampoline:
  // r12 = shared_exported_functions[i]
  lsls r0, #2
  ldr  r1, =shared_exported_functions
  ldr  r1, [r1, r0]
  mov  r12, r1
  // jump to function
  pop  {r0-r1}
  bx   r12
