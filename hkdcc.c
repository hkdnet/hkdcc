#include <stdio.h>
#include <string.h>

#include "hkdcc.h"

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "arguments count mismatch\n");
    return 1;
  }

  if (strcmp(argv[1], "-test") == 0) {
    runtest();
    return 0;
  }

  Vector *tokens = tokenize(argv[1]);

  // for debug
  // show_tokens();

  Vector *nodes = parse(tokens);

  printf(".intel_syntax noprefix\n");
  printf(".global _main\n");
  printf("_main:\n");

  // prologue
  int var_size = 26; // a to z
  printf("  push rbp\n");
  printf("  mov rbp, rsp\n");
  printf("  sub rsp, %d\n", 8 * var_size);

  int i;
  for (i = 0; i < nodes->len; i++) {
    generate(nodes->data[i]);
    printf("  pop rax\n");
  }

  // frame epilogue
  printf("  mov rsp, rbp\n");
  printf("  pop rbp\n");
  printf("  ret\n");
  return 0;
}
