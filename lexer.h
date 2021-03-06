#ifndef LEXER_H_SENTRY
#define LEXER_H_SENTRY

/* Not defined by default */
#if !defined(LEXER_DEBUG) && 0
#define LEXER_DEBUG
#endif

#define ES_LEXER_INCURABLE_ERROR 1

#include <stdio.h>

typedef enum type_of_lex {
    LEX_INPUT,         /* '<'  */
    LEX_OUTPUT,        /* '>'  */
    LEX_APPEND,        /* '>>' */
    LEX_PIPE,          /* '|'  */
    LEX_OR,            /* '||' */
    LEX_BACKGROUND,    /* '&'  */
    LEX_AND,           /* '&&' */
    LEX_SEMICOLON,     /* ';'  */
    LEX_BRACKET_OPEN,  /* '('  */
    LEX_BRACKET_CLOSE, /* ')'  */
    LEX_REVERSE,       /* '`'  */
    LEX_WORD,     /* all different */
    LEX_EOLINE,        /* '\n' */
    LEX_EOFILE,        /* EOF  */
    LEX_ERROR     /* error in lexer */
} type_of_lex;

typedef struct lexeme {
/*    struct lexeme *next; */
    type_of_lex type;
    char *str;
} lexeme;

typedef enum lexer_state {
    ST_START,
    ST_ONE_SYM_LEX,
    /* '<', ';', '(', ')' */
    ST_ONE_TWO_SYM_LEX,
    /* '>', '>>', '|', '||', '&', '&&' */
    ST_BACKSLASH,
    ST_BACKSLASH_IN_QUOTES,
    ST_IN_QUOTES,
    ST_ERROR,
    ST_WORD,
    ST_EOLN_EOF
} lexer_state;

typedef struct lexer_info {
    lexer_state state;
    int c; /* current symbol */
    unsigned int get_next_char:1;
} lexer_info;

void init_lexer(lexer_info *info);
lexeme *get_lex(lexer_info *info);
void destroy_lex(lexeme *lex);

void print_lex(FILE *stream, lexeme *lex);

#endif
