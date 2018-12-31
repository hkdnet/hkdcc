#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include "hkdcc.h"

int pos = 0;

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

static int is_eq = 0;

void tokenize(char *p) {
  int idx = 0;
  while (*p) {
    if (*p == '=') {
      if (is_eq) {
        tokens[idx].type = TK_EQEQ;
        tokens[idx].input = p;
        p++;
        idx++;

        is_eq = 0;
        continue;
      } else {
        is_eq = 1;
        p++;
        continue;
      }
    } else {
      if (is_eq) {
        tokens[idx].type = TK_EQ;
        tokens[idx].input = p;
        p++;
        idx++;

        is_eq = 0;
        continue;
      }
    }

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

int parse() {
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
  return i;
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
