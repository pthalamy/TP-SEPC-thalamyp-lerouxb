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
/* void init_shell_history() { */
/*     BACKGROUND_PID = malloc(sizeof(shell_history)); */
/*     BACKGROUND_PID->head = NULL; */
/* } */

int parse_and_execute_line(char **line);

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

void remove_shell_cmd(shell_history *liste, shell_cmd *command){
    shell_cmd *cour, *suiv = liste->head;

    if (liste || command)
	return;
    else if (liste->head == command)
	liste->head = liste->head->next;

    while (!cour) {
	suiv = cour->next;
	if (suiv == command)
	    cour->next = suiv->next;
	cour = suiv;
    }
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

    int fds[2];			/* A pipe file-descriptor */
    pid_t PID;

    int in = 0;		/* input file desc: stdin by default */
    if (cl->in)
	in = open(cl->in, O_RDONLY);
    int out = 1;		/* output file desc: stdout by default */
    if (cl->out)
	out = open(cl->out, O_WRONLY | O_CREAT);

    /* If first command is "jobs" print PID */
    if (!strncmp(cl->seq[0][0], "jobs", 4)) {
	if (BACKGROUND_PID.head == NULL)
	    printf("No background processes\n");
	else {
	    shell_cmd *cour = BACKGROUND_PID.head;
	    printf("Active background processes:\n");
	    int j = 1;
	    while (cour != NULL) {
		printf("+ [%d] PID: %d ", j,
		       cour->PID);
		int k = 0;
		/* while (cour->command[k] != NULL) { */
		printf("%s ", cour->command[k]);
		/* 	++k; */
		/* } */
		printf("\n");
		++j;
		cour = cour->next;
	    }
	}
    } else if (cl->seq[1]) {    /* Execute command line, command has pipe*/
	if (cl->seq[2])
	    fprintf(stderr,
		    "error: execution of commands with multiple pipes "
		    "not implemented\n");

	pipe(fds);

	/* Execute each command from command line */
	switch (PID = fork()) {
	case -1:
	    /* there was an error during child creation */
	    perror("fork:");
	    break;
	case 0:
	{
	    /* Process is child process, READING pipe end */
	    dup2(fds[0], 0);
	    close(fds[1]); close(fds[0]);

	    if (out != 1) {
		dup2(out, 1); 	/* Redirect output to out */
		close(out);
	    }

	    execvp(cl->seq[1][0], cl->seq[1]);
	}
	default:
	    break;
	}
    }

    switch (PID = fork()) {
    case -1:
	/* there was an error during child creation */
	perror("fork:");
	break;
    case 0:
    {
	/* Process is child process, WRITING pipe end */
	if (cl->seq[1]) {
	    dup2(fds[1], 1);
	    close(fds[0]); close(fds[1]);
	} else if (out != 1) {
	    dup2(out, 1); 	/* Redirect output to out */
	    close(out);
	}

	if (in != 0) {
	    dup2(in, 0); 	/* Redirect input from in */
	    close(in);
	}

	wordexp_t p;
	if (cl->seq[0][1]) {
	    wordexp(cl->seq[0][1], &p, 0);
	    cl->seq[0][1] = p.we_wordv[0];
	}

	execvp(cl->seq[0][0], cl->seq[0]);
    }
    default:
	/* Process is father and PID = its child's PID */
#if DEBUG_PID
	printf("child PID: %d\n", PID);
#endif

	if (cl->seq[1]) {
	    close(fds[1]); close(fds[0]);
	}

	if (cl->bg) {
	    printf("+ PID: %d\n", PID);

	    shell_cmd *new = malloc(sizeof(shell_cmd));
	    new->command = malloc(sizeof(char *));
	    new->next = malloc(sizeof(shell_cmd));
	    new->PID = PID;

	    int k = 0;

	    while (cl->seq[0][k] != NULL) {
		strcpy(new->command[k], cl->seq[0][k]);
		++k;
	    }

	    insert_shell_cmd(&BACKGROUND_PID, new);
	}
	else
	    waitpid(PID, NULL, 0);

	break;
    }

    return 0;
}


int main() {
    printf("Variante %d: %s\n", VARIANTE, VARIANTE_STRING);

//    init_shell_history();

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
