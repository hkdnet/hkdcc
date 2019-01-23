#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hkdcc.h"

// rax: return value
// rsp: stack pointer
// rbp: base register
// rdi, rsi, rdx, rcs, r8, r9: args

static char *arg_registers[7] = {"", "rdi", "rsi", "rdx", "rcs", "r8", "r9"};

void generate_lvalue(Node *node, Map *var_names) {
  int idx;
  for (idx = 0; idx < var_names->keys->len; idx++) {
    char *s = var_names->keys->data[idx];
    if (strcmp(s, node->name) == 0) {
      break;
    }
  }
  if (node->type == ND_IDENT) {
    printf("  mov rax, rbp\n");
    printf("  sub rax, %d\n", idx * 8);
    printf("  push rax\n");
    return;
  }
  fprintf(stderr, "unexpected lhs, it should be ND_IDENT\n");
  exit(1);
}

void generate(Node *node, Map *var_names) {
  if (node->type == ND_NUM) {
    printf("  push %d\n", node->value);
    return;
  }

  if (node->type == ND_IDENT) {
    generate_lvalue(node, var_names);
    printf("  pop rax\n");
    printf("  mov rax, [rax]\n");
    printf("  push rax\n");
    return;
  }

  if (node->type == ND_CALL) {
    if (node->lhs) {
      generate(node->lhs, var_names);
      for (int i = node->value; i > 0; i--) {
        printf("  pop %s\n", arg_registers[i]);
      }
    }
    printf("  call _%s\n", node->name); // TODO: always with underscore ?
    printf("  push rax\n");
    return;
  }

  if (node->type == ND_ARGS) {
    generate(node->lhs, var_names);
    if (node->rhs) {
      generate(node->rhs, var_names);
    }
    return;
  }

  if (node->type == ND_ASGN) {
    generate_lvalue(node->lhs, var_names);
    generate(node->rhs, var_names);

    printf("  pop rdi\n");
    printf("  pop rax\n");
    printf("  mov [rax], rdi\n");
    printf("  push rdi\n");
    return;
  }

  if (node->type == ND_EQEQ || node->type == ND_NEQ) {
    generate(node->lhs, var_names);
    generate(node->rhs, var_names);
    printf("  pop rax\n");
    printf("  pop rdi\n");
    printf("  cmp rdi, rax\n");
    printf("  %s al\n", node->type == ND_EQEQ ? "sete" : "setne");
    printf("  movzx rax, al\n");
    printf("  push rax\n");
    return;
  }

  if (node->lhs)
    generate(node->lhs, var_names);
  if (node->rhs)
    generate(node->rhs, var_names);

  // two operand
  printf("  pop rdi\n");
  printf("  pop rax\n");
  switch (node->type) {
  case '+':
    printf("  add rax, rdi\n");
    break;
  case '-':
    printf("  sub rax, rdi\n");
    break;
  case '*':
    printf("  mul rdi\n"); // memo: this means `rax = rax * rdi`
    break;
  case '/':
    printf("  mov rdx, 0\n");
    printf("  div rdi\n"); // memo: rax = ((rdx << 64) | rax) / rdi
    break;
  }
  printf("  push rax\n");
}
