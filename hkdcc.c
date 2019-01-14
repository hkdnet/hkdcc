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
  // show_tokens(tokens);

  Vector *nodes = parse(tokens);
  Map *names = variable_names(nodes);

  printf(".intel_syntax noprefix\n");
  printf(".global _main\n");
  printf("_main:\n");

  // prologue
  printf("  push rbp\n");
  printf("  mov rbp, rsp\n");
  printf("  sub rsp, %d\n", 8 * names->keys->len);

  int i;
  for (i = 0; i < nodes->len; i++) {
    generate(nodes->data[i], names);
    printf("  pop rax\n");
  }

  // frame epilogue
  printf("  mov rsp, rbp\n");
  printf("  pop rbp\n");
  printf("  ret\n");
  return 0;
}
