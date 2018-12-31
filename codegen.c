#include <stdio.h>
#include <stdlib.h>

#include "hkdcc.h"

// rax: return value
// rsp: stack pointer
// rbp: base register
// rdi, rsi, rdx, rcs, r8, r9: args

void generate_lvalue(Node *node) {
  int diff = 'z' - node->name + 1;
  if (node->type == ND_IDENT) {
    printf("  mov rax, rbp\n");
    printf("  sub rax, %d\n", diff * 8);
    printf("  push rax\n");
    return;
  }
  fprintf(stderr, "unexpected lhs, it should be ND_IDENT\n");
  exit(1);
}

void generate(Node *node) {
  if (node->type == ND_NUM) {
    printf("  push %d\n", node->value);
    return;
  }

  if (node->type == ND_IDENT) {
    generate_lvalue(node);
    printf("  pop rax\n");
    printf("  mov rax, [rax]\n");
    printf("  push rax\n");
    return;
  }

  if (node->type == ND_ASGN) {
    generate_lvalue(node->lhs);
    generate(node->rhs);

    printf("  pop rdi\n");
    printf("  pop rax\n");
    printf("  mov [rax], rdi\n");
    printf("  push rdi\n");
    return;
  }

  if (node->lhs)
    generate(node->lhs);
  if (node->rhs)
    generate(node->rhs);

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
