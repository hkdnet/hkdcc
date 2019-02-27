#include <stdio.h>
#include <stdlib.h>
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

  Node *main = malloc(sizeof(Node));
  main->type = ND_FUNC;
  main->name = "_main";
  Node *main_decl = malloc(sizeof(Node));
  main_decl->type = ND_FUNC_DECL;
  main_decl->parameters = nodes;
  Node *main_body = malloc(sizeof(Node));
  main_body->type = ND_FUNC_BODY;
  main_body->expressions = nodes;
  main_body->variable_names = names;
  main->lhs = main_decl;
  main->rhs = main_body;

  printf(".intel_syntax noprefix\n");
  printf(".global _main\n");

  generate(main, NULL);
  return 0;
}
