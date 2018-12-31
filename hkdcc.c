#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

enum {
  TK_NUM = 256, // number
  TK_LPAREN,    // (
  TK_RPAREN,    // )
  TK_IDENT,     // a-z
  TK_SCOLON,    // ;
  TK_EQ,        // =
  TK_EOF,
};

typedef struct {
  int type;
  int value;
  char *input;
} Token;

enum {
  ND_NUM = 256, // number node
  ND_IDENT,     // identifier
  ND_PROG,      // program
  ND_ASGN,
};

typedef struct Node {
  int type; // ND_X, + or -
  struct Node *lhs;
  struct Node *rhs;
  int value; // the value of ND_NUM
  char name; // for ND_IDENT
} Node;

// Buffer for tokens.
// up to 100 tokens for now...
Token tokens[100];
int pos = 0;
Node *code[100];

Node *new_node(int type, Node *lhs, Node *rhs) {
  Node *node = malloc(sizeof(Node));
  node->type = type;
  node->lhs = lhs;
  node->rhs = rhs;
  return node;
}

Node *new_node_num(int value) {
  Node *node = malloc(sizeof(Node));
  node->type = ND_NUM;
  node->value = value;
  return node;
}

Node *new_node_ident(char var) {
  Node *node = malloc(sizeof(Node));
  node->type = ND_IDENT;
  node->name = var;
  return node;
}

Node *expr();

// term: number | ident
// term: "(" expr ")"
Node *term() {
  int beg = pos;
  if (tokens[pos].type == TK_NUM) {
    return new_node_num(tokens[pos++].value);
  }
  if (tokens[pos].type == TK_IDENT) {
    return new_node_ident(*tokens[pos++].input);
  }
  if (tokens[pos].type == TK_LPAREN) {
    pos++;
    Node *ret = expr();
    if (tokens[pos].type == TK_RPAREN) {
      pos++;
      return ret;
    }
    fprintf(stderr, "mismatch paren, begin at %d, now %d\n", beg, pos);
    exit(1);
  }
  fprintf(stderr, "unexpected token at %d: %s\n", pos, tokens[pos].input);
  exit(1);
}

// mul:  term
// mul:  term "*" mul
// mul:  term "/" mul
Node *mul() {
  Node *lhs = term();
  if (tokens[pos].type == '*') {
    pos++; // skip *
    Node *rhs = mul();
    return new_node('*', lhs, rhs);
  }
  if (tokens[pos].type == '/') {
    pos++; // skip /
    Node *rhs = mul();
    return new_node('/', lhs, rhs);
  }
  return lhs;
}

// expr:  mul expr'
// expr': ε | "+" expr | "-" expr
Node *expr() {
  Node *lhs = mul();
  if (tokens[pos].type == '+') {
    pos++; // skip +
    Node *rhs = expr();
    return new_node('+', lhs, rhs);
  }
  if (tokens[pos].type == '-') {
    pos++; // skip -
    Node *rhs = expr();
    return new_node('-', lhs, rhs);
  }
  return lhs;
}

// assign': ε | "=" expr assign'
Node *assign_tail() {
  if (tokens[pos].type != TK_EQ) { // ε
    return NULL;
  }
  pos++; // skip "="
  Node *lhs = expr();
  Node *rhs = assign_tail();
  if (!rhs) {
    return lhs;
  }
  return new_node(ND_ASGN, lhs, rhs);
}

// assign : expr assign' ";"
// assign': ε | "=" expr assign'
Node *assign() {
  Node *lhs = expr();
  Node *rhs = assign_tail();
  if (tokens[pos].type == TK_SCOLON) { // ε
    pos++;                             // skip ;
    if (!rhs) {
      return lhs;
    }
    return new_node(ND_ASGN, lhs, rhs);
  }
  fprintf(stderr, "unexpected token at %d\n", pos);
  exit(1);
}

// program : assign program'
// program': ε | assign program'
Node *program() {
  Node *lhs = assign();
  if (tokens[pos].type == TK_EOF) {
    return lhs;
  }
  Node *rhs = assign();
  return new_node(ND_PROG, lhs, rhs);
}

void tokenize(char *p) {
  int idx = 0;
  while (*p) {
    if (isspace(*p)) {
      p++;
      continue;
    }

    // semicolon
    if (*p == ';') {
      tokens[idx].type = TK_SCOLON;
      tokens[idx].input = p;
      p++;
      idx++;
      continue;
    }
    if (*p == '=') {
      tokens[idx].type = TK_EQ;
      tokens[idx].input = p;
      p++;
      idx++;
      continue;
    }
    // paren
    if (*p == '(') {
      tokens[idx].type = TK_LPAREN;
      tokens[idx].input = p;
      p++;
      idx++;
      continue;
    }
    if (*p == ')') {
      tokens[idx].type = TK_RPAREN;
      tokens[idx].input = p;
      p++;
      idx++;
      continue;
    }

    // operator
    if (*p == '+' || *p == '-' || *p == '*' || *p == '/') {
      tokens[idx].type = *p;
      tokens[idx].input = p;
      p++;
      idx++;
      continue;
    }

    // digit
    if (isdigit(*p)) {
      tokens[idx].type = TK_NUM;
      tokens[idx].input = p;
      tokens[idx].value = strtol(p, &p, 10);
      idx++;
      continue;
    }

    // identifier
    // TODO: for now, identifier is a, b, c,..., z
    if ('a' <= *p && *p <= 'z') {
      tokens[idx].type = TK_IDENT;
      tokens[idx].input = p;
      idx++;
      p++;
      continue;
    }

    fprintf(stderr, "unexpected characters %s\n", p);
    exit(1);
  }
  tokens[idx].type = TK_EOF;
  tokens[idx].input = p;
}

void show_tokens() {
  int i = 0;
  while (tokens[i].type != 0) {
    switch (tokens[i].type) {
    case TK_IDENT:
      printf("%10s: %c\n", "TK_IDENT", *tokens[i].input);
      break;
    case TK_NUM:
      printf("%10s: %d\n", "TK_NUM", tokens[i].value);
      break;
    case TK_LPAREN:
      printf("%10s:\n", "TK_LPAREN");
      break;
    case TK_RPAREN:
      printf("%10s:\n", "TK_RPAREN");
      break;
    case TK_EOF:
      printf("%10s:\n", "TK_EOF");
      break;
    case TK_SCOLON:
      printf("%10s:\n", "TK_SCOLON");
      break;
    case TK_EQ:
      printf("%10s:\n", "TK_EQ");
      break;
    default:
      printf("%10c:\n", tokens[i].type);
      break;
    }
    i++;
  }
}

void show_node(Node *node, int indent) {
  for (int i = 0; i < indent; i++)
    printf(" ");

  switch (node->type) {
  case ND_NUM:
    printf("ND_NUM: %d\n", node->value);
    return;
  case ND_ASGN:
    printf("ND_ASGN:\n");
  default:
    printf("%c\n", node->type);
  }

  if (node->lhs)
    show_node(node->lhs, indent + 2);
  if (node->rhs)
    show_node(node->rhs, indent + 2);
}

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

  int i = 0;
  while (tokens[pos].type != TK_EOF) {
    Node *asgn = assign();
    code[i] = asgn;
    // for debug
    // printf("show_node at %d\n", i);
    // show_node(asgn, 0);
    i++;
  }
  code[i] = NULL;

  printf(".intel_syntax noprefix\n");
  printf(".global _main\n");
  printf("_main:\n");

  // prologue
  printf("  push rbp\n");
  printf("  mov rbp, rsp\n");
  printf("  sub rsp, %d\n", 8 * 26);

  i = 0;
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
