/*****************************************************
 * Copyright Grégory Mounié 2008-2015                *
 *           Simon Nieuviarts 2002-2009              *
 * This code is distributed under the GLPv3 licence. *
 * Ce code est distribué sous la licence GPLv3+.     *
 *****************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdbool.h>
#include <wordexp.h>

#include "variante.h"
#include "readcmd.h"

#ifndef VARIANTE
#error "Variante non défini !!"
#endif

/* Guile (1.8 and 2.0) is auto-detected by cmake */
/* To disable Scheme interpreter (Guile support), comment the
 * following lines.  You may also have to comment related pkg-config
 * lines in CMakeLists.txt.
 */

#define DEBUG_COMMAND_LINE 0
#define DEBUG_PID 0

void terminate(char *line);

#if USE_GUILE == 1
#include <libguile.h>

typedef struct shell_cmd{
    char **command;
    pid_t PID;
    struct shell_cmd *next;
} shell_cmd;

typedef struct shell_history {
    shell_cmd *head;
} shell_history;

shell_history BACKGROUND_PID = {NULL};

int parse_and_execute_line(char **line);

/* Insert a background process into shell_history*/
void insert_shell_cmd(shell_history *liste, shell_cmd *command) {

    if (liste->head == NULL) {
	liste->head = command;
	liste->head->next = NULL;
    }

    else {
	shell_cmd *temp;
	temp = liste->head;
	liste->head = command;
	command->next = temp;
    }
}

void print_shell_history() {
    shell_cmd *cour = (shell_cmd *)BACKGROUND_PID.head;
    while (cour != NULL) {
	printf("[PID: %i]->", cour->PID);
	cour = cour->next;
    }
    printf("NULL\n");
}

/* Delete all process terminated*/
void update_shell_history(shell_history *liste){

    shell_cmd *sentinelle = malloc(sizeof(shell_cmd));
    insert_shell_cmd(&BACKGROUND_PID, sentinelle);

    int status;
    int pid_return;

    shell_cmd *cour = liste->head, *suiv;

    while (cour != NULL) {
	suiv = cour->next;
	if (suiv != NULL) {

	    pid_return = waitpid(suiv->PID, &status, WNOHANG);

	    if (pid_return == suiv->PID) {
		cour->next = suiv->next;
		cour = suiv->next;
	    }
	    else
		cour = suiv;
	}
	else
	    cour = suiv;
    }

    liste->head = liste->head->next;

    free(sentinelle);
}

int executer(char *line)
{
    /* Insert your code to execute the command line
     * identically to the standard execution scheme:
     * parsecmd, then fork+execvp, for a single command.
     * pipe and i/o redirection are not required.
     */

    return parse_and_execute_line(&line);
}

SCM executer_wrapper(SCM x)
{
    return scm_from_int(executer(scm_to_locale_stringn(x, 0)));
}
#endif


void terminate(char *line) {
#ifdef USE_GNU_READLINE
    /* rl_clear_history() does not exist yet in centOS 6 */
    clear_history();
#endif
    if (line)
	free(line);
    printf("exit\n");
    exit(0);
}

int array_length(uintptr_t *arr)
{
    if (!arr)
	return 0;

    int count = 0;
    int i = 0;

    while (arr[i++])
	count++;

    return count;
}

/* Expands command number num of cl and stores result in expanded */
void perform_line_expansion(struct cmdline *cl, int num, wordexp_t *expanded)
{
    /* Expand program name */
    wordexp(cl->seq[num][0], expanded, 0);

    /* Expand args  */
    for (int i = 1; cl->seq[num][i]; i++) {
	if (wordexp(cl->seq[num][i], expanded, WRDE_APPEND)) {
	    wordfree(expanded);
	    fprintf(stderr, "error: wildcard expansion failed\n");
	    exit(1);
	}
    }
}

void execute_command(struct cmdline *cl, int n, int fdin, int fdout)
{
    wordexp_t expanded;
    pid_t PID;

    perform_line_expansion(cl, n, &expanded);

    switch (PID = fork()) {
    case -1:
	/* there was an error during child creation */
	perror("fork:");
	break;
    case 0:
    {
	if (fdin != 0) {
	    dup2(fdin, 0);	/* Redirect input from in */
	    close(fdin);
	}

	if (fdout != 1) {
	    dup2(fdout, 1); 	/* Redirect output to out */
	    close(fdout);
	}

	execvp(expanded.we_wordv[0], expanded.we_wordv);
    }
    default:
	/* Process is father and PID = its child's PID */
#if DEBUG_PID
	printf("child PID: %d\n", PID);
#endif

	/* Free wordexp */
	wordfree(&expanded);

	if (fdin != 0) {
	    close(fdin);
	}

	if (fdout != 1) {
	    close(fdout);
	}

	if (cl->bg) {
	    printf("+ PID: %d\n", PID);

	    shell_cmd *new = malloc(sizeof(shell_cmd));
	    new->command = malloc(sizeof(char *));
	    new->next = malloc(sizeof(shell_cmd));
	    new->PID = PID;

	    strcpy(new->command[0], cl->seq[n][0]);

	    insert_shell_cmd(&BACKGROUND_PID, new);
	} else if (!cl[n + 1))
	    waitpid(PID, NULL, 0);

	break;
    }
}

int parse_and_execute_line(char **line)
{
    /* Parse line */
    struct cmdline *cl = parsecmd(line);

    /* If input stream closed, normal termination */
    if (!cl) {
	terminate(0);
    }

    if (cl->err) {
	/* Syntax error, read another command */
	printf("error: %s\n", cl->err);
	return -1;
    } else if (!cl->seq[0])	/* Empty line */
	return 0;

#if DEBUG_COMMAND_LINE
    if (cl->in) printf("in: %s\n", cl->in);
    if (cl->out) printf("out: %s\n", cl->out);
    if (cl->bg) printf("background (&)\n");

    /* Display each command of the pipe */
    for (int i = 0; cl->seq[i] != 0; i++) {
	char **cmd = cl->seq[i];
	printf("seq[%d]: ", i);
	for (int j = 0; cmd[j] != 0; j++) {
	    printf("'%s' ", cmd[j]);
	}
	printf("\n");
    }
#endif

    int in = 0;		/* input file desc: stdin by default */
    if (cl->in)
	in = open(cl->in, O_RDONLY);
    int out = 1;		/* output file desc: stdout by default */
    if (cl->out)
	out = open(cl->out, O_WRONLY | O_CREAT | O_TRUNC);

    /* If first command is "jobs" print PID */
    if (!strncmp(cl->seq[0][0], "jobs", 4)) {

	update_shell_history(&BACKGROUND_PID);

	if (BACKGROUND_PID.head == NULL)
	    printf("No background processes\n");
	else {
	    printf("Active background processes:\n");
	    shell_cmd *cour = BACKGROUND_PID.head;
	    int j = 1;
	    while (cour != NULL) {
		printf("+ [%d] PID: %d ", j,
		       cour->PID);
		printf("%s ", cour->command[0]);
		printf("\n");
		++j;
		cour = cour->next;
	    }
	}

	return 0;
    } else {
	int fds[2];			/* A pipe file-descriptor */
	int prevFdin = in;
	int fdout;
	for (int i = 0; cl->seq[i]; i++) {
	    pipe(fds);

	    fdout = (cl->seq[i + 1] == NULL) ? out : fds[1];

	    execute_command(cl, i, prevFdin, fdout);

	    prevFdin = fds[0];
	}
    }

    return 0;
}


int main() {
    printf("Variante %d: %s\n", VARIANTE, VARIANTE_STRING);

#ifdef USE_GUILE
    scm_init_guile();
    /* register "executer" function in scheme */
    scm_c_define_gsubr("executer", 1, 0, 0, executer_wrapper);
#endif

    while (1) {
	/* struct cmdline *l; */
	char *line=0;
	/* int i, j; */
	char *prompt = "ensishell>";

	/* Readline use some internal memory structure that
	   can not be cleaned at the end of the program. Thus
	   one memory leak per command seems unavoidable yet */
	line = readline(prompt);
	if (line == 0 || ! strncmp(line,"exit", 4)) {
	    terminate(line);
	}

#ifdef USE_GNU_READLINE
	add_history(line);
#endif


#ifdef USE_GUILE
	/* The line is a scheme command */
	if (line[0] == '(') {
	    char catchligne[strlen(line) + 256];
	    sprintf(catchligne, "(catch #t (lambda () %s) (lambda (key . parameters) (display \"mauvaise expression/bug en scheme\n\")))", line);
	    scm_eval_string(scm_from_locale_string(catchligne));
	    free(line);
	    continue;
	}
#endif

	parse_and_execute_line(&line);
    }
}
