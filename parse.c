#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hkdcc.h"

typedef struct {
  Vector *tokens;
  int pos;
} ParseState;

Token *cur_token(ParseState *state) {
  return (Token *)state->tokens->data[state->pos];
}

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
  node->lhs = NULL;
  node->rhs = NULL;
  return node;
}

Node *new_node_ident(char *var) {
  Node *node = malloc(sizeof(Node));
  node->type = ND_IDENT;
  node->name = var;
  node->lhs = NULL;
  node->rhs = NULL;
  return node;
}

Node *expr();

// term: number | ident | ident "(" ")"
// term: "(" expr ")"
Node *term(ParseState *state) {
  int beg = state->pos;
  Token *token = cur_token(state);
  if (token->type == TK_NUM) {
    Node *ret = new_node_num(token->value);
    state->pos++;
    return ret;
  }
  if (token->type == TK_IDENT) {
    state->pos++;
    Token *next = cur_token(state);
    if (next->type != TK_LPAREN) {
      Node *ret = new_node_ident(token->input);
      return ret;
    }
    int beg = state->pos;
    state->pos++; // skip "("

    if (cur_token(state)->type == TK_RPAREN) {
      // function call
      Node *ret = new_node_ident(token->input); // TODO: fix this abuse
      ret->type = ND_CALL;
      state->pos++; // skip ")"
      return ret;
    }
    fprintf(stderr, "mismatch paren for function call, begin at %d, now %d\n",
            beg, state->pos);
    exit(1);
  }
  if (token->type == TK_LPAREN) {
    state->pos++; // skip (
    Node *ret = expr(state);
    token = cur_token(state);
    if (token->type == TK_RPAREN) {
      state->pos++; // skip )
      return ret;
    }
    fprintf(stderr, "mismatch paren, begin at %d, now %d\n", beg, state->pos);
    exit(1);
  }
  fprintf(stderr, "unexpected token at %d: %s\n", state->pos, token->input);
  exit(1);
}

// mul:  term
// mul:  term "*" mul
// mul:  term "/" mul
Node *mul(ParseState *state) {
  Node *lhs = term(state);
  Token *token = cur_token(state);
  if (token->type == '*') {
    state->pos++; // skip *
    Node *rhs = mul(state);
    return new_node('*', lhs, rhs);
  }
  if (token->type == '/') {
    state->pos++; // skip /
    Node *rhs = mul(state);
    return new_node('/', lhs, rhs);
  }
  return lhs;
}

// expr:  mul expr'
// expr': ε | "+" expr | "-" expr | "==" expr | "!=" expr
Node *expr(ParseState *state) {
  Node *lhs = mul(state);
  Token *token = cur_token(state);
  if (token->type == '+') {
    state->pos++; // skip +
    Node *rhs = expr(state);
    return new_node('+', lhs, rhs);
  }
  if (token->type == '-') {
    state->pos++; // skip -
    Node *rhs = expr(state);
    return new_node('-', lhs, rhs);
  }
  if (token->type == TK_EQEQ) {
    state->pos++; // skip TK_EQEQ
    Node *rhs = expr(state);
    return new_node(ND_EQEQ, lhs, rhs);
  }
  if (token->type == TK_NEQ) {
    state->pos++; // skip TK_NEQ
    Node *rhs = expr(state);
    return new_node(ND_NEQ, lhs, rhs);
  }
  return lhs;
}

// assign': ε | "=" expr assign'
Node *assign_tail(ParseState *state) {
  Token *token = cur_token(state);
  if (token->type != TK_EQ) { // ε
    return NULL;
  }
  state->pos++; // skip "="
  Node *lhs = expr(state);
  Node *rhs = assign_tail(state);
  if (!rhs) {
    return lhs;
  }
  return new_node(ND_ASGN, lhs, rhs);
}

// assign : expr assign' ";"
// assign': ε | "=" expr assign'
Node *assign(ParseState *state) {
  Node *lhs = expr(state);
  Node *rhs = assign_tail(state);

  Token *token = cur_token(state);
  if (token->type == TK_SCOLON) { // ε
    state->pos++;                 // skip ;
    if (!rhs) {
      return lhs;
    }
    return new_node(ND_ASGN, lhs, rhs);
  }
  fprintf(stderr, "unexpected token at %d\n", state->pos);
  exit(1);
}

// program : assign program'
// program': ε | assign program'
Node *program(ParseState *state) {
  Node *lhs = assign(state);
  Token *token = cur_token(state);
  if (token->type == TK_EOF) {
    return lhs;
  }
  Node *rhs = assign(state);
  return new_node(ND_PROG, lhs, rhs);
}

char *tokenize_eq(char *p, Vector *tokens) {
  if (*p != '=') {
    fprintf(stderr, "assert error: tokenize_eq should be called with =\n");
    exit(1);
  }

  p++; // skip "="

  if (*p == '=') { // ==
    Token *token = malloc(sizeof(Token));
    token->type = TK_EQEQ;
    token->input = p;
    p++;
    vec_push(tokens, token);
    return p;
  }

  // single eq, such as "a = b"
  Token *token = malloc(sizeof(Token));
  token->type = TK_EQ;
  token->input = p - 1;
  vec_push(tokens, token);
  return p;
}

char *tokenize_bang(char *p, Vector *tokens) {
  if (*p != '!') {
    fprintf(stderr, "assert error: tokenize_bang should be called with !\n");
    exit(1);
  }

  p++; // skip "!"

  if (*p == '=') { // !=
    Token *token = malloc(sizeof(Token));
    token->type = TK_NEQ;
    token->input = p;
    p++; // skip !=
    vec_push(tokens, token);
    return p;
  }

  // TODO: currently, "!" is only a part of "!=". After impl unary operator
  // "!", this is not an error.
  fprintf(stderr, "unexpected characters %s\n", p);
  exit(1);
}

int is_identifier_head(char c) {
  return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || c == '_';
}

int is_identifier_char(char c) {
  return is_identifier_head(c) || ('0' <= c && c <= '9');
}

Vector *tokenize(char *p) {
  Vector *ret = new_vector();
  while (*p) {
    if (*p == '=') {
      p = tokenize_eq(p, ret);
      continue;
    }

    if (*p == '!') {
      p = tokenize_bang(p, ret);
      continue;
    }

    if (isspace(*p)) {
      p++;
      continue;
    }

    // semicolon
    if (*p == ';') {
      Token *token = malloc(sizeof(Token));
      token->type = TK_SCOLON;
      token->input = p;
      p++;
      vec_push(ret, token);
      continue;
    }
    // paren
    if (*p == '(') {
      Token *token = malloc(sizeof(Token));
      token->type = TK_LPAREN;
      token->input = p;
      p++;

      vec_push(ret, token);
      continue;
    }
    if (*p == ')') {
      Token *token = malloc(sizeof(Token));
      token->type = TK_RPAREN;
      token->input = p;
      p++;

      vec_push(ret, token);
      continue;
    }
    if (*p == ',') {
      Token *token = malloc(sizeof(Token));
      token->type = TK_COMMA;
      token->input = p;
      p++;

      vec_push(ret, token);
      continue;
    }

    // operator
    if (*p == '+' || *p == '-' || *p == '*' || *p == '/') {
      Token *token = malloc(sizeof(Token));
      token->type = *p;
      token->input = p;
      p++;
      vec_push(ret, token);
      continue;
    }

    // digit
    if (isdigit(*p)) {
      Token *token = malloc(sizeof(Token));
      token->type = TK_NUM;
      token->input = p;
      token->value = strtol(p, &p, 10);
      vec_push(ret, token);
      continue;
    }

    // identifier
    if (is_identifier_head(*p)) {
      char *beg = p;
      while (is_identifier_char(*p))
        p++;
      char *end = p;
      int size = end - beg;
      char *s = malloc(sizeof(char) * (size + 1));
      memcpy(s, beg, size);
      s[size] = '\0';
      Token *token = malloc(sizeof(Token));
      token->type = TK_IDENT;
      token->input = s;
      vec_push(ret, token);
      continue;
    }

    fprintf(stderr, "unexpected characters %s\n", p);
    exit(1);
  }
  Token *token = malloc(sizeof(Token));
  token->type = TK_EOF;
  token->input = p;
  vec_push(ret, token);
  return ret;
}

Vector *parse(Vector *tokens) {
  Vector *ret = new_vector();
  ParseState *state = malloc(sizeof(ParseState));
  state->tokens = tokens;
  state->pos = 0;
  while (1) {
    Token *token = cur_token(state);
    if (token->type == TK_EOF) {
      break;
    }

    Node *asgn = assign(state);
    vec_push(ret, asgn);
    // for debug
    // printf("show_node at %d\n", i);
    // show_node(asgn, 0);
  }

  free(state);
  return ret;
}

void show_tokens(Vector *tokens) {
  for (int i = 0; i < tokens->len; i++) {
    Token *token = tokens->data[i];
    switch (token->type) {
    case TK_IDENT:
      printf("%10s: %s\n", "TK_IDENT", token->input);
      break;
    case TK_NUM:
      printf("%10s: %d\n", "TK_NUM", token->value);
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
    case TK_EQEQ:
      printf("%10s:\n", "TK_EQEQ");
      break;
    default:
      printf("%10c:\n", token->type);
      break;
    }
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

void put_variable_name_on_node(Map *m, long *i, Node *node) {
  if (node == NULL) {
    return;
  }
  put_variable_name_on_node(m, i, node->lhs);
  put_variable_name_on_node(m, i, node->rhs);
  if (node->type == ND_IDENT) {
    if (!map_get(m, node->name)) {
      long idx = *i;
      map_put(m, node->name, (void *)idx);
      *i = *i + 1;
    }
  }
}

Map *variable_names(Vector *nodes) {
  Map *m = new_map();
  long idx = 0;
  for (int i = 0; i < nodes->len; i++) {
    put_variable_name_on_node(m, &idx, nodes->data[i]);
  }
  return m;
}
