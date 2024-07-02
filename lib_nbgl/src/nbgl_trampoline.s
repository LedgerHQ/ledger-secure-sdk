.syntax unified
.text
.thumb

.section .text._nbgl_trampoline
.thumb_func
.global _nbgl_trampoline
_nbgl_trampoline:
  // r12 = nbgl_exported_functions[i]
  lsls r0, #2
  ldr  r1, =nbgl_exported_functions
  ldr  r1, [r1, r0]
  mov  r12, r1
  // jump to function
  pop  {r0-r1}
  bx   r12
