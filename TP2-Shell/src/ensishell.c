/*****************************************************
 * Copyright Grégory Mounié 2008-2015                *
 *           Simon Nieuviarts 2002-2009              *
 * This code is distributed under the GLPv3 licence. *
 * Ce code est distribué sous la licence GPLv3+.     *
 *****************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>

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

int BACKGROUND_PID[20] = {0};

#if USE_GUILE == 1
#include <libguile.h>

int executer(char *line)
{
    /* Insert your code to execute the command line
     * identically to the standard execution scheme:
     * parsecmd, then fork+execvp, for a single command.
     * pipe and i/o redirection are not required.
     */

    /* printf("line: %s\n", line); */

    /* Parse line */
    struct cmdline *cl = parsecmd(&line);

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

    /* If first command is "jobs" print PID */
    if (!strncmp(cl->seq[0][0], "jobs", 4)) {
	if (BACKGROUND_PID[0] == 0)
	    printf("No background processes\n");
	else {
	    printf("Active background processes:\n");
	    int j = 0;
	    while (BACKGROUND_PID[j] > 0 && j < 20) {
		printf("+ [%d] PID : %d\n", j, BACKGROUND_PID[j]);
		++j;
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
	    if (cl->seq[1]) {		/* Pipe command */
		dup2(fds[0], 0);
		close(fds[1]); close(fds[0]);

		execvp(cl->seq[1][0], cl->seq[1]);
	    }
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
	}

	execvp(cl->seq[0][0], cl->seq[0]);
    }
    default:
	/* Process is father and PID = its child's PID */
#if DEBUG_PID
	printf("child PID: %d\n", PID);
#endif
	if (cl->bg) {
	    printf("+ PID: %d\n", PID);
	    int j = 0;
	    while (BACKGROUND_PID[j] != 0 && j < 20)
		++j;
	    BACKGROUND_PID[j] = PID;
	}
	else
	    waitpid(PID, NULL, 0);

	break;
    }

    return 0;
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

	executer(line);
    }

}
