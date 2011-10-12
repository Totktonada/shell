#include "word_buffer.h"
#include "lexer.h"
#include <stdlib.h>

typedef struct cmd_pipeline_item {
    /* Item data: */
    char **argv;
    char *input;  /* temporally */
    char *output; /* temporally */
    unsigned int append:1; /* temporally */
    struct cmd_list *cmd_lst;
    /* End of item data */
    struct cmd_pipeline_item *next;
} cmd_pipeline_item;

typedef struct cmd_pipeline {
    char *input;
    char *output;
    unsigned int append:1;
    struct cmd_pipeline_item *first_item;
} cmd_pipeline;

typedef enum type_of_relation {
    REL_NONE,  /* no relation */
    REL_OR,    /* '||' */
    REL_AND,   /* '&&' */
    REL_BOTH   /* ';'  */
} type_of_relation;

typedef struct cmd_list_item {
    /* Item data: */
    struct cmd_pipeline *pl;
    type_of_relation rel;
    /* End of item data */
    struct cmd_list_item *next;
} cmd_list_item;

typedef struct cmd_list {
    unsigned int foreground:1;
    struct cmd_list_item *first_item;
} cmd_list;

typedef struct parser_info {
    lexer_info *linfo;
    lexeme *cur_lex;
    int error;
} parser_info;

#define PARSER_DEBUG

#ifdef PARSER_DEBUG

#include <stdio.h>

void parser_print_action (parser_info *pinfo, const char *where, int leaving)
{
    if (leaving)
        fprintf (stderr, "Parser: leaving from %s; ", where);
    else
        fprintf (stderr, "Parser: entering to %s; ", where);
    print_lex (stderr, pinfo->cur_lex);
}

#endif

void parser_print_error (parser_info *pinfo, const char *where)
{
    fprintf (stderr, "Parser: error #%d in %s; ", pinfo->error, where);
    print_lex (stderr, pinfo->cur_lex);
}

void parser_get_lex (parser_info *pinfo);

void init_parser (parser_info *pinfo)
{
/*  parser_info *pinfo =
        (parser_info *) malloc (sizeof (parser_info)); */

    pinfo->linfo = (lexer_info *) malloc (sizeof (lexer_info));
    init_lexer (pinfo->linfo);
    pinfo->cur_lex = NULL;
}

void parser_get_lex (parser_info *pinfo)
{
    if (pinfo->cur_lex != NULL)
        free (pinfo->cur_lex); /* do not freeing string */

    pinfo->cur_lex = get_lex (pinfo->linfo);

#ifdef PARSER_DEBUG
    fprintf (stderr, "Get lex: ");
    print_lex (stderr, pinfo->cur_lex);
#endif

    if (pinfo->cur_lex->type == LEX_ERROR) /* Error 1 */
        pinfo->error = 1;
    else
        pinfo->error = 0; /* Error 0: all right */
}

cmd_pipeline_item *make_cmd_pipeline_item ()
{
    cmd_pipeline_item *simple_cmd =
        (cmd_pipeline_item *) malloc (sizeof (cmd_pipeline_item));
    simple_cmd->argv = NULL;
    simple_cmd->input = NULL;
    simple_cmd->output = NULL;
    simple_cmd->append = 0;
    simple_cmd->cmd_lst = NULL;
    simple_cmd->next = NULL;
    return simple_cmd;
}

cmd_pipeline *make_cmd_pipeline ()
{
    cmd_pipeline *pipeline =
        (cmd_pipeline *) malloc (sizeof (cmd_pipeline));
    pipeline->input = NULL;
    pipeline->output = NULL;
    pipeline->append = 0;
    pipeline->first_item = NULL;
    return pipeline;
}

cmd_list_item *make_cmd_list_item ()
{
    cmd_list_item *list_item =
        (cmd_list_item *) malloc (sizeof (cmd_list_item));
    list_item->pl = NULL;
    list_item->rel = REL_NONE;
    list_item->next = NULL;
    return list_item;
}

cmd_list *make_cmd_list ()
{
    cmd_list *list =
        (cmd_list *) malloc (sizeof (cmd_list));
    list->foreground = 1;
    list->first_item = NULL;
    return list;
}

void print_cmd_list (FILE *stream, cmd_list *list, int newline);

void print_cmd_pipeline (FILE *stream, cmd_pipeline *pipeline)
{
    cmd_pipeline_item *current;

    if (pipeline == NULL) {
        fprintf (stream, "NULL_PIPELINE");
        return;
    }

    current = pipeline->first_item;
    while (current != NULL) {
        if (current->cmd_lst == NULL) {
            print_argv (stream, current->argv);
        } else {
            fprintf (stream, "(");
            print_cmd_list (stream, current->cmd_lst, 0);
            fprintf (stream, ")");
        }
        if (current->next != NULL)
            fprintf (stream, " | ");
        current = current->next;
    }

    if (pipeline->input != NULL) {
        fprintf (stream, " < [%s]", pipeline->input);
    }
    if (pipeline->output != NULL) {
        if (pipeline->append)
            fprintf (stream, " >> [%s]", pipeline->output);
        else
            fprintf (stream, " > [%s]", pipeline->output);
    }
}

void print_relation (FILE *stream, type_of_relation rel) {
    switch (rel) {
    case REL_OR:
        fprintf (stream, " || ");
        break;
    case REL_AND:
        fprintf (stream, " && ");
        break;
    case REL_BOTH:
        fprintf (stream, " ; ");
        break;
    case REL_NONE:
        /* Do nothing */
        break;
    }
}

void print_cmd_list (FILE *stream, cmd_list *list, int newline)
{
    cmd_list_item *current;

    if (list == NULL) {
        fprintf (stream, "[NULL_CMD_LIST]\n");
        return;
    }

    current = list->first_item;
    while (current != NULL) {
        print_cmd_pipeline (stream, current->pl);
        print_relation (stream, current->rel);
        current = current->next;
    }

    if (!list->foreground)
        fprintf (stream, "&");
    if (newline)
        fprintf (stream, "\n");
}

void destroy_cmd_list (cmd_list *list);

void destroy_cmd_pipeline (cmd_pipeline *pipeline)
{
    cmd_pipeline_item *current;
    cmd_pipeline_item *next;

    if (pipeline == NULL)
        return;

    current = pipeline->first_item;
    while (current != NULL) {
        next = current->next;
        destroy_argv (current->argv);
        destroy_cmd_list (current->cmd_lst);
        free (current);
        current = next;
    }

    if (pipeline->input != NULL)
        free (pipeline->input);
    if (pipeline->output != NULL)
        free (pipeline->output);
    free (pipeline);
}

void destroy_cmd_list (cmd_list *list)
{
    cmd_list_item *current;
    cmd_list_item *next;

    if (list == NULL)
        return;

    current = list->first_item;
    while (current != NULL) {
        next = current->next;
        destroy_cmd_pipeline (current->pl);
        free (current);
        current = next;
    }

    free (list);
}

cmd_pipeline_item *parse_cmd_pipeline_item (parser_info *pinfo)
{
    cmd_pipeline_item *simple_cmd = make_cmd_pipeline_item ();
    word_buffer wbuf;
    new_word_buffer (&wbuf);

#ifdef PARSER_DEBUG
    parser_print_action (pinfo, "parse_cmd_pipeline_item ()", 0);
#endif

    do {
        switch (pinfo->cur_lex->type) {
        case LEX_WORD:
            /* Add to word buffer for making argv */
            add_to_word_buffer (&wbuf, pinfo->cur_lex->str);
            parser_get_lex (pinfo);
            break;
        case LEX_INPUT:
            pinfo->error = (simple_cmd->input == NULL) ? 0 : 10; /* Error 10 */
            if (pinfo->error)
                goto error;

            parser_get_lex (pinfo);
            /* Lexer error possible */
            if (pinfo->error)
                goto error;

            pinfo->error = (pinfo->cur_lex->type != LEX_WORD) ? 12 : 0; /* Error 12 */
            if (pinfo->error)
                goto error;

            simple_cmd->input = pinfo->cur_lex->str;
            parser_get_lex (pinfo);
            break;
        case LEX_OUTPUT:
        case LEX_APPEND:
            pinfo->error = (simple_cmd->output == NULL) ? 0 : 11; /* Error 11 */
            if (pinfo->error)
                goto error;

            simple_cmd->append = (pinfo->cur_lex->type == LEX_OUTPUT) ? 0 : 1;
            parser_get_lex (pinfo);
            /* Lexer error possible */
            if (pinfo->error)
                goto error;

            pinfo->error = (pinfo->cur_lex->type != LEX_WORD) ? 2 : 0; /* Error 2 */
            if (pinfo->error)
                goto error;

            simple_cmd->output = pinfo->cur_lex->str;
            parser_get_lex (pinfo);
            break;
        default:
            /* Lexer error possible */
            if (pinfo->error)
                goto error;

            pinfo->error = (wbuf.count_words == 0) ? 3 : 0; /* Error 3 */
            if (pinfo->error)
                goto error;

            /* make argv from word buffer */
            simple_cmd->argv = convert_to_argv (&wbuf, 1);
#ifdef PARSER_DEBUG
            parser_print_action (pinfo, "parse_cmd_pipeline_item ()", 1);
#endif
            return simple_cmd;
        }
    } while (!pinfo->error);

error:
#ifdef PARSER_DEBUG
    parser_print_error (pinfo, "parse_cmd_pipeline_item ()");
#endif
    clear_word_buffer (&wbuf, 1);
    if (simple_cmd->input != NULL)
        free (simple_cmd->input);
    if (simple_cmd->output != NULL)
        free (simple_cmd->output);
    free (simple_cmd);
    return NULL;
}

cmd_list *parse_cmd_list (parser_info *pinfo, int bracket_terminated);

cmd_pipeline *parse_cmd_pipeline (parser_info *pinfo)
{
    cmd_pipeline *pipeline = make_cmd_pipeline ();
    cmd_pipeline_item *cur_item = NULL, *tmp_item = NULL;

#ifdef PARSER_DEBUG
    parser_print_action (pinfo, "parse_cmd_pipeline ()", 0);
#endif

    do {
        switch (pinfo->cur_lex->type) {
        case LEX_WORD:
            tmp_item = parse_cmd_pipeline_item (pinfo);
            break;
        case LEX_BRACKET_OPEN:
            /* TODO: переместить считывание
             * открывающей скобки в parse_cmd_list.
             * Нужно ли? */
            parser_get_lex (pinfo);
            if (pinfo->error)
                goto error;

            tmp_item = make_cmd_pipeline_item ();
            tmp_item->cmd_lst = parse_cmd_list (pinfo, 1);
            if (pinfo->error) {
                free (tmp_item);
                goto error;
            }

            parser_get_lex (pinfo);
            break;
        default:
            pinfo->error = 4; /* Error 4 */
            break;
        }

        if (pinfo->error)
            goto error;

        if (pipeline->first_item == NULL) {
            /* First simple cmd */
            pipeline->first_item = cur_item = tmp_item;
            pipeline->input = cur_item->input;
            cur_item->input = NULL;
        } else {
            /* Second and following simple cmd */
            cur_item = cur_item->next = tmp_item;
            pinfo->error = (cur_item->input == NULL) ? 0 : 8; /* Error 8 */
            if (pinfo->error) {
                free (cur_item->input);
                if (cur_item->output != NULL)
                    free (cur_item->output);
                goto error;
            }
        }

        if (pinfo->cur_lex->type == LEX_PIPE) {
            /* Not last simple cmd */
            pinfo->error = (cur_item->output == NULL) ? 0 : 9; /* Error 9 */
            if (pinfo->error) {
                free (cur_item->output);
                if (cur_item->input != NULL)
                    free (cur_item->input);
                goto error;
            }

            parser_get_lex (pinfo);
            continue;
        } else {
            /* Last simple cmd in this pipeline */
            pipeline->output = cur_item->output;
            pipeline->append = cur_item->append;
            cur_item->output = NULL;
            cur_item->append = 0;
#ifdef PARSER_DEBUG
            parser_print_action (pinfo, "parse_cmd_pipeline ()", 1);
#endif
            return pipeline;
        }
    } while (!pinfo->error);

error:
#ifdef PARSER_DEBUG
    parser_print_error (pinfo, "parse_cmd_pipeline ()");
#endif
    destroy_cmd_pipeline (pipeline);
    return NULL;
}

cmd_list_item *parse_cmd_list_item (parser_info *pinfo)
{
    cmd_list_item *list_item = make_cmd_list_item ();

#ifdef PARSER_DEBUG
    parser_print_action (pinfo, "parse_cmd_list_item ()", 0);
#endif

    list_item->pl = parse_cmd_pipeline (pinfo);
    if (pinfo->error)
        goto error;

    switch (pinfo->cur_lex->type) {
    case LEX_OR:
        list_item->rel = REL_OR;
        break;
    case LEX_AND:
        list_item->rel = REL_AND;
        break;
    case LEX_SEMICOLON:
        list_item->rel = REL_BOTH;
        break;
    default:
        list_item->rel = REL_NONE;
        break;
    }

    if (list_item->rel != REL_NONE)
        parser_get_lex (pinfo);

    if (pinfo->error)
        goto error;

#ifdef PARSER_DEBUG
    parser_print_action (pinfo, "parse_cmd_list_item ()", 1);
#endif
    return list_item;

error:
#ifdef PARSER_DEBUG
    parser_print_error (pinfo, "parse_cmd_list_item ()");
#endif
    free (list_item);
    return NULL;
}

cmd_list *parse_cmd_list (parser_info *pinfo, int bracket_terminated)
{
    cmd_list *list = make_cmd_list (pinfo);
    cmd_list_item *cur_item = NULL, *tmp_item = NULL;

#ifdef PARSER_DEBUG
    parser_print_action (pinfo, "parse_cmd_list ()", 0);
#endif

    do {
        tmp_item = parse_cmd_list_item (pinfo);
        if (pinfo->error)
            goto error;

        if (list->first_item == NULL)
            list->first_item = cur_item = tmp_item;
        else
            cur_item = cur_item->next = tmp_item;

        if (pinfo->cur_lex->type == LEX_BACKGROUND) {
            list->foreground = 0;
            parser_get_lex (pinfo);
            if (pinfo->error)
                goto error;
        }

        switch (pinfo->cur_lex->type) {
        case LEX_BRACKET_CLOSE:
            pinfo->error = (bracket_terminated) ? 0 : 5; /* Error 5 */
            break;
        case LEX_EOLINE:
        case LEX_EOFILE:
            pinfo->error = (bracket_terminated) ? 6 : 0; /* Error 6 */
            break;
        default:
            /* Do nothing */
            continue;
        }

        if (pinfo->error)
            goto error;

        switch (cur_item->rel) {
        case REL_NONE:
            /* Do nothing */
            break;
        case REL_BOTH:
            cur_item->rel = REL_NONE;
            break;
        case REL_OR:
        case REL_AND:
            pinfo->error = 7; /* Error 7 */
            break;
        }

        if (pinfo->error)
            continue;

#ifdef PARSER_DEBUG
        parser_print_action (pinfo, "parse_cmd_list ()", 1);
#endif
        return list;
    } while (!pinfo->error);

error:
#ifdef PARSER_DEBUG
    parser_print_error (pinfo, "parse_cmd_list ()");
#endif
    destroy_cmd_list (list);
    return NULL;
}

/* Compile:
gcc -g -Wall -ansi -pedantic -c buffer.c -o buffer.o &&
gcc -g -Wall -ansi -pedantic -c lexer.c -o lexer.o &&
gcc -g -Wall -ansi -pedantic -c word_buffer.c -o word_buffer.o &&
gcc -g -Wall -ansi -pedantic parser.c buffer.o lexer.o word_buffer.o -o parser
 * Grep possible parsing errors:
grep -Pn '\* Error \d+ \*' parser.c
*/

#include <stdio.h>

int main ()
{
    cmd_list *list;
    parser_info pinfo;
    init_parser (&pinfo);

    do {
        parser_get_lex (&pinfo);
        list = parse_cmd_list (&pinfo, 0);

        if (pinfo.error) {
            /* TODO: flush read buffer,
             * possibly via buffer function. */
            fprintf (stderr, "Parser: error;\n");
        } else {
            print_cmd_list (stdout, list, 1);
            destroy_cmd_list (list);
            list = NULL;
        }

        if (pinfo.cur_lex->type == LEX_EOFILE)
            exit (pinfo.error);
    } while (1);

    return 0;
}
