#ifndef _RUNNER_H
#define _RUNNER_H

#include "common.h"

#define RUNNER_DEBUG

typedef struct process {
    char **argv;
    pid_t pid;
    unsigned int completed:1;
    unsigned int stopped:1;
    int exit_status;
    /* exit_status correct, if process
     * completed */
    struct process *next;
} process;

typedef struct job {
/*  Need? char *command; */
    process *first_process;
    pid_t pgid;
    unsigned int notified:1;
/*    struct termious tmodes;
 *    Not compiled with it */
    int infile;
    int outfile;
    /* int errfile; */
    struct job *next;
} job;

typedef struct shell_info {
    char **envp;
    pid_t shell_pgid;
    unsigned int shell_interactive:1;
    int orig_stdin;
    int orig_stdout;
    /* orig_stdin/orig_stdout —
     * temporally for
     * change_redirections () */
     job *first_job;
     job *last_job;
    /*job *active_jobs;*/
} shell_info;

#include "parser.h"
void run_cmd_list (shell_info *sinfo,
        cmd_list *list);

#endif
