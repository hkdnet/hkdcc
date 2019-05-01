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

void generate_lvalue(Node *node, Vector *var_names) {
  int idx;
  for (idx = 0; idx < var_names->len; idx++) {
    char *s = var_names->data[idx];
    if (strcmp(s, node->name) == 0) {
      break;
    }
  }
  if (node->type == ND_IDENT) {
    printf("  mov rax, rbp\n");
    printf("  sub rax, %d\n", (idx + 1) * 8);
    printf("  push rax\n");
    return;
  }
  fprintf(stderr, "unexpected lhs, it should be ND_IDENT\n");
  exit(1);
}

void generate(Node *node, Vector *var_names) {
  if (node->type == ND_PROG) {
    Vector *functions = node->functions;
    for (int i = 0; i < functions->len; i++) {
      generate(functions->data[i], var_names);
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
           8 * body->variable_names->len);
    // memo: ローカル変数へのアクセスは rbp - 8*n になる

    int i;
    for (i = 0; i < decl->parameters->len; i++) {
      char *parameter_name = decl->parameters->data[i];
      int idx;
      for (idx = 0; idx < body->variable_names->len; idx++) {
        char *s = body->variable_names->data[idx];
        if (strcmp(s, parameter_name) == 0) {
          break;
        }
      }
      if (idx == body->variable_names->len) {
        fprintf(stderr, "parameter %s is not found in local variables\n",
                parameter_name);
        exit(1);
      }

      printf("  mov rax, rbp   # rax <- rbp\n");
      printf("  sub rax, %d     # rax <- rax - NUM\n", (idx + 1) * 8);
      printf("  mov [rax], %s # [rax] <- rax\n", arg_registers[i + 1]);
    }

    Vector *statements = body->statements;
    for (i = 0; i < statements->len; i++) {
      printf("  # -- stmt%04d START --\n", i);
      generate(statements->data[i], body->variable_names);
      printf("  pop rax\n");
      printf("  # -- stmt%04d END --\n", i);
    }

    // frame epilogue
    printf("  mov rsp, rbp # rsp <- rbp\n");
    printf("  pop rbp      # pop to rbp \n");
    printf("  ret\n");
    return;
  }

  if (node->type == ND_RET) {
    generate(node->lhs, var_names);
    printf("  pop rax\n");
    printf("  mov rsp, rbp\n");
    printf("  pop rbp\n");
    printf("  ret\n");
    return;
  }

  if (node->type == ND_IF) {
    generate(node->lhs, var_names);
    printf("  pop rax\n");
    printf("  cmp rax, 0\n");
    printf("  je .Lend%03d\n", ++label_count);
    int cur_cnt = label_count;
    generate(node->rhs, var_names);
    printf(".Lend%03d:\n", cur_cnt);
    printf("  push rax\n");
    return;
  }

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
    printf("  call _%s\n", node->name); // always with underscore
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
