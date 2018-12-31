typedef struct {
  void **data;
  int capacity;
  int len;
} Vector;

typedef struct {
  Vector *keys;
  Vector *values;
} Map;

enum {
  TK_NUM = 256, // number
  TK_LPAREN,    // (
  TK_RPAREN,    // )
  TK_IDENT,     // a-z
  TK_SCOLON,    // ;
  TK_EQ,        // =
  TK_EQEQ,      // ==
  TK_NEQ,       // !=
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
  ND_EQEQ,
  ND_NEQ,
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
Node *code[100];
extern int pos;

// util.c
Vector *new_vector();
void vec_push(Vector *vec, void *elem);
Map *new_map();
void map_put(Map *map, void *key, void *value);
void *map_get(Map *map, char *key);

// util_test.c
void runtest();

// parser.c
void tokenize(char *p);
int parse();
void show_tokens();
void show_node(Node *node, int indent);

// codegen.c
void generate_lvalue(Node *node);
void generate(Node *node);
