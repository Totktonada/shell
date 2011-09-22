#include "lexer.h"

#include "buffer.h"
#include <stdio.h>
#include <stdlib.h>

/* #define DEBUG_LEXER */

void print_state (const char *state_name, int c)
{
    fprintf (stderr, "Lexer: %s; ", state_name);

    switch (c) {
    case '\n':
        fprintf(stderr, "\'\\n\';\n");
        break;
    case EOF:
        fprintf(stderr, "\'EOF\';\n");
        break;
    default:
        fprintf(stderr, "\'%c\';\n", c);
        break;
    }
}

void print_lex (lexeme *lex)
{
    printf ("!!! Lexeme: ");
    switch (lex->type) {
    case LEX_INPUT:         /* '<'  */
        printf ("[<]\n");
        break;
    case LEX_OUTPUT:        /* '>'  */
        printf ("[>]\n");
        break;
    case LEX_APPEND:        /* '>>' */
        printf ("[>>]\n");
        break;
    case LEX_PIPE:          /* '|'  */
        printf ("[|]\n");
        break;
    case LEX_OR:            /* '||' */
        printf ("[||]\n");
        break;
    case LEX_BACKGROUND:    /* '&'  */
        printf ("[&]\n");
        break;
    case LEX_AND:           /* '&&' */
        printf ("[&&]\n");
        break;
    case LEX_SEMICOLON:     /* ';'  */
        printf ("[;]\n");
        break;
    case LEX_BRACKET_OPEN:  /* '('  */
        printf ("[(]\n");
        break;
    case LEX_BRACKET_CLOSE: /* ')'  */
        printf ("[)]\n");
        break;
    case LEX_REVERSE:       /* '`'  */
        printf ("[`]\n");
        break;
    case LEX_WORD:     /* all different */
        printf ("[WORD:%s]\n", lex->str);
        break;
    case LEX_EOLINE:        /* '\n' */
        printf ("[EOLINE]\n");
        break;
    case LEX_EOFILE:         /* EOF  */
        printf ("[EOFILE]\n");
        break;
    }
}

void deferred_get_char (lexer_info *info)
{
    info->get_next_char = 1;
}

void get_char (lexer_info *info)
{
    info->c = getchar();
}

void init_lexer (lexer_info *info)
{
/*  lexer_info *info =
        (lexer_info *) malloc (sizeof (lexer_info)); */

    deferred_get_char (info);
    info->state = ST_START;
}

lexeme *make_lex (type_of_lex type)
{
    lexeme *lex = (lexeme *) malloc (sizeof (lexeme));
/*    lex->next = NULL; */
    lex->type = type;
    lex->str = NULL;
    return lex;
}

void destroy_lex (lexeme *lex)
{
    if (lex->str != NULL)
        free (lex->str);
    free (lex);
}

lexeme *get_lex (lexer_info *info)
{
    lexeme *lex = NULL;
    buffer buf;

    new_buffer (&buf);

    do {
        if (info->get_next_char) {
            info->get_next_char = 0;
            get_char (info);
        }

        switch (info->state) {
        case ST_START:
#ifdef DEBUG_LEXER
            print_state ("ST_START", info->c);
#endif

            switch (info->c) {
            case EOF:
            case '\n':
                info->state = ST_EOLN_EOF;
                break;
            case ' ':
                deferred_get_char (info);
                break;
            case '<':
            case ';':
            case '(':
            case ')':
                info->state = ST_ONE_SYM_LEX;
                break;
            case '>':
            case '|':
            case '&':
                info->state = ST_ONE_TWO_SYM_LEX;
                break;
            case '\\':
            case '\"':
            default:
                info->state = ST_WORD;
                break;
            }
            break;

        case ST_ONE_SYM_LEX:
#ifdef DEBUG_LEXER
            print_state ("ST_ONE_SYM_LEX", info->c);
#endif

            switch (info->c) {
            case '<':
                lex = make_lex (LEX_INPUT);
                break;
            case ';':
                lex = make_lex (LEX_SEMICOLON);
                break;
            case '(':
                lex = make_lex (LEX_BRACKET_OPEN);
                break;
            case ')':
                lex = make_lex (LEX_BRACKET_CLOSE);
                break;
            default:
                fprintf (stderr, "Lexer: error in ST_ONE_SYM_LEX;");
                print_state ("ST_ONE_SYM_LEX", info->c);
                exit (1);
            }
            /* We don't need buffer */
            deferred_get_char (info);
            info->state = ST_START;
            return lex;

        case ST_ONE_TWO_SYM_LEX:
#ifdef DEBUG_LEXER
            print_state ("ST_ONE_TWO_SYM_LEX", info->c);
#endif

            if (buf.count_sym == 0) {
                add_to_buffer (&buf, info->c);
                deferred_get_char (info);
            } else if (buf.count_sym == 1) {
                /* TODO: make it more pretty *;
                char prev_c = get_last_from_buffer (&buf);
                clear_buffer (&buf);
                switch (prev_c) {
                case '>':
                    lex = (prev_c == info->c) ?
                        make_lex (LEX_APPEND) :
                        make_lex (LEX_OUTPUT);
                    break;
                case '|':
                    lex = (prev_c == info->c) ?
                        make_lex (LEX_OR) :
                        make_lex (LEX_PIPE);
                    break;
                case '&':
                    lex = (prev_c == info->c) ?
                        make_lex (LEX_AND) :
                        make_lex (LEX_BACKGROUND);
                    break;
                default:
                    fprintf (stderr, "Lexer: error (type 1) in ST_ONE_TWO_SYM_LEX;");
                    print_state ("ST_ONE_TWO_SYM_LEX", info->c);
                    exit (1);
                }
                if (prev_c == info->c)
                    deferred_get_char (info);
                info->state = ST_START;
                return lex;
            } else {
                fprintf (stderr, "Lexer: error (type 2) in ST_ONE_TWO_SYM_LEX;");
                print_state ("ST_ONE_TWO_SYM_LEX", info->c);
                exit (1);
            }
            break;


        case ST_BACKSLASH:
#ifdef DEBUG_LEXER
            print_state ("ST_BACKSLASH", info->c);
#endif

            switch (info->c) {
            case EOF:
                info->state = ST_ERROR;
                break;
            case ' ':
            case '\"':
            case '\\':
                add_to_buffer (&buf, info->c);
            case '\n':
                /* Ignore newline symbol */
                break;
            case 'a':
                add_to_buffer (&buf, '\a');
                break;
            case 'b':
                add_to_buffer (&buf, '\b');
                break;
            case 'f':
                add_to_buffer (&buf, '\f');
                break;
            case 'n':
                add_to_buffer (&buf, '\n');
                break;
            case 'r':
                add_to_buffer (&buf, '\r');
                break;
            case 't':
                add_to_buffer (&buf, '\t');
                break;
            case 'v':
                add_to_buffer (&buf, '\v');
                break;
            default:
                /* Substitution not found */
                add_to_buffer (&buf, '\\');
                add_to_buffer (&buf, info->c);
            }
            deferred_get_char (info);
            info->state = ST_WORD;
            break;

        case ST_BACKSLASH_IN_QUOTES:
#ifdef DEBUG_LEXER
            print_state ("ST_BACKSLASH_IN_QUOTES", info->c);
#endif

            /* TODO: what different from ST_BACKSLASH? */
            /* Probably, absolutelly identical ST_BACKSLASH,
             * but: info->state = ST_IN_QUOTES */
            /* Currently backslash ignored */
            info->state = ST_IN_QUOTES;
            break;

        case ST_IN_QUOTES:
#ifdef DEBUG_LEXER
            print_state ("ST_IN_QUOTES", info->c);
#endif

            switch (info->c) {
            case EOF:
                info->state = ST_ERROR;
                break;
            case '\\':
                deferred_get_char (info);
                info->state = ST_BACKSLASH_IN_QUOTES;
                break;
            case '\"':
                deferred_get_char (info);
                info->state = ST_WORD;
                break;
            default:
                add_to_buffer (&buf, info->c);
                deferred_get_char (info);
                break;
            }
            break;

        case ST_WORD:
#ifdef DEBUG_LEXER
            print_state ("ST_WORD", info->c);
#endif

            switch (info->c) {
            case EOF:
            case '\n':
            case ' ':
            case '<':
            case ';':
            case '(':
            case ')':
            case '>':
            case '|':
            case '&':
            case '`':
                info->state = ST_START;
                lex = make_lex (LEX_WORD);
                lex->str = convert_to_string (&buf, 1);
                clear_buffer (&buf);
                return lex;
            case '\\':
                deferred_get_char (info);
                info->state = ST_BACKSLASH;
                break;
            case '\"':
                deferred_get_char (info);
                info->state = ST_IN_QUOTES;
                break;
            default:
                add_to_buffer (&buf, info->c);
                deferred_get_char (info);
            }
            break;

        case ST_ERROR:
            print_state ("ST_ERROR", info->c);
            break;

        case ST_EOLN_EOF:
#ifdef DEBUG_LEXER
            print_state ("ST_EOLN_EOF", info->c);
#endif

            switch (info->c) {
            case '\n':
                lex = make_lex(LEX_EOLINE);
                break;
            case EOF:
                lex = make_lex(LEX_EOFILE);
                break;
            default:
                fprintf (stderr, "Lexer: error in ST_EOLN_EOF;");
                print_state ("ST_EOLN_EOF", info->c);
                exit (1);
            }
            deferred_get_char (info);
            info->state = ST_START;
            return lex;

        default:
            print_state ("unrecognized state", info->c);
            exit (1);
            break;
        }
    } while (1);
}
