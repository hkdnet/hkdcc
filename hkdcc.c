#include <stdio.h>

#include "hkdcc.h"

// rax: return value
// rsp: stack pointer
// rbp: base register
// rdi, rsi, rdx, rcs, r8, r9: args
int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "arguments count mismatch\n");
    return 1;
  }

  tokenize(argv[1]);

  // for debug
  // show_tokens();

  parse();

  printf(".intel_syntax noprefix\n");
  printf(".global _main\n");
  printf("_main:\n");

  // prologue
  printf("  push rbp\n");
  printf("  mov rbp, rsp\n");
  printf("  sub rsp, %d\n", 8 * 26);

  int i = 0;
  while (code[i]) {
    generate(code[i++]);
    printf("  pop rax\n");
  }
  // frame epilogue
  printf("  mov rsp, rbp\n");
  printf("  pop rbp\n");
  printf("  ret\n");
  return 0;
}
