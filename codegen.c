#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hkdcc.h"

// rax: return value
// rsp: stack pointer
// rbp: base register
// rdi, rsi, rdx, rcs, r8, r9: args

static char *arg_registers[7] = {"", "rdi", "rsi", "rdx", "rcs", "r8", "r9"};
static int label_count = 0;

void generate_lvalue(Node *node, Map *variables) {
  Variable *var = map_get(variables, node->name);
  if (node->type == ND_IDENT) {
    printf("  mov rax, rbp\n");
    printf("  sub rax, %d\n", (var->index + 1) * 8);
    printf("  push rax\n");
    return;
  }
  fprintf(stderr, "unexpected lhs, it should be ND_IDENT\n");
  exit(1);
}

void generate(Node *node, Map *variables) {
  if (node->type == ND_VAR_DECL) {
    // TODO: Fix this abuse...
    printf("push 1\n"); // dummy
    return;
  }

  if (node->type == ND_PROG) {
    Vector *functions = node->functions;
    for (int i = 0; i < functions->len; i++) {
      generate(functions->data[i], variables);
    }
    return;
  }
  if (node->type == ND_FUNC) {
    Node *decl = node->lhs;
    Node *body = node->rhs;

    // prologue
    printf("_%s:\n", node->name);
    printf("  push rbp       # 現在のスタック位置 rbp を積む\n");
    printf("  mov rbp, rsp   # rbp <- rsp\n");
    printf("  sub rsp, %d     # rsp <- rsp - NUM: ローカル変数分の領域を確保\n",
           8 * body->variables->keys->len);
    // memo: ローカル変数へのアクセスは rbp - 8*n になる

    int i;
    for (i = 0; i < decl->parameters->len; i++) {
      printf("  mov rax, rbp   # rax <- rbp\n");
      printf("  sub rax, %d     # rax <- rax - NUM\n", (i + 1) * 8);
      printf("  mov [rax], %s # [rax] <- rax\n", arg_registers[i + 1]);
    }

    Vector *statements = body->statements;
    for (i = 0; i < statements->len; i++) {
      printf("  # -- stmt%04d START --\n", i);
      generate(statements->data[i], body->variables);
      printf("  pop rax\n");
      printf("  # -- stmt%04d END --\n", i);
    }

    // frame epilogue
    printf("  mov rsp, rbp # rsp <- rbp\n");
    printf("  pop rbp      # pop to rbp \n");
    printf("  ret\n");
    return;
  }

  if (node->type == ND_BLOCK) {
    Vector *statements = node->statements;
    for (int i = 0; i < statements->len; i++) {
      printf("  # -- stmt%04d for block START --\n", i);
      generate(statements->data[i], variables); // Note that block does not create a new scope
      printf("  pop rax\n");
      printf("  # -- stmt%04d for block END --\n", i);
    }
    return;
  }

  if (node->type == ND_RET) {
    generate(node->lhs, variables);
    printf("  pop rax\n");
    printf("  mov rsp, rbp\n");
    printf("  pop rbp\n");
    printf("  ret\n");
    return;
  }

  if (node->type == ND_IF) {
    int cur_cnt = ++label_count;
    generate(node->lhs, variables);
    printf("  pop rax\n");
    printf("  cmp rax, 0\n");
    printf("  je .Lend%03d\n", cur_cnt);
    generate(node->rhs, variables);
    printf(".Lend%03d:\n", cur_cnt);
    printf("  push rax\n");
    return;
  }

  if (node->type == ND_WHILE) {
    int beg_count = ++label_count;
    int end_count = ++label_count;
    printf(".Lbegin%03d:\n", beg_count);
    generate(node->lhs, variables);
    printf("  pop rax\n");
    printf("  cmp rax, 0\n");
    printf("  je .Lend%03d\n", end_count);
    generate(node->rhs, variables);
    printf("  jmp .Lbegin%03d\n", beg_count);
    printf(".Lend%03d:\n", end_count);
    printf("  push rax\n");
    return;
  }

  if (node->type == ND_NUM) {
    printf("  push %d\n", node->value);
    return;
  }

  if (node->type == ND_IDENT) {
    generate_lvalue(node, variables);
    printf("  pop rax\n");
    printf("  mov rax, [rax]\n");
    printf("  push rax\n");
    return;
  }

  if (node->type == ND_CALL) {
    if (node->lhs) {
      generate(node->lhs, variables);
      for (int i = node->value; i > 0; i--) {
        printf("  pop %s\n", arg_registers[i]);
      }
    }
    printf("  call _%s\n", node->name); // always with underscore
    printf("  push rax\n");
    return;
  }

  if (node->type == ND_ARGS) {
    generate(node->lhs, variables);
    if (node->rhs) {
      generate(node->rhs, variables);
    }
    return;
  }

  if (node->type == ND_ASGN) {
    generate_lvalue(node->lhs, variables);
    generate(node->rhs, variables);

    printf("  pop rdi\n");
    printf("  pop rax\n");
    printf("  mov [rax], rdi\n");
    printf("  push rdi\n");
    return;
  }

  if (node->type == ND_EQEQ || node->type == ND_NEQ || node->type == ND_LTEQ ||
      node->type == ND_LT) {
    generate(node->lhs, variables);
    generate(node->rhs, variables);
    printf("  pop rax\n");
    printf("  pop rdi\n");
    printf("  cmp rdi, rax\n");
    switch (node->type) {
    case ND_EQEQ:
      printf("  sete al\n");
      break;
    case ND_NEQ:
      printf("  setne al\n");
      break;
    case ND_LTEQ:
      printf("  setle al\n");
      break;
    case ND_LT:
      printf("  setl al\n");
      break;
    }
    printf("  movzx rax, al\n");
    printf("  push rax\n");
    return;
  }

  if (node->lhs)
    generate(node->lhs, variables);
  if (node->rhs)
    generate(node->rhs, variables);

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
