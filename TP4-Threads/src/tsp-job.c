/* impl�mentation des queues de jobs, nul besoin de lire dans un premier temps */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "tsp-types.h"
#include "tsp-job.h"

struct tsp_job {
    tsp_path_t path;
    int hops;
    int len;
    uint64_t vpres;
};

struct tsp_cell {
    struct tsp_job tsp_job;
    struct tsp_cell *next;
};

void init_queue (struct tsp_queue *q) {
    q->first = 0;
    q->last = 0;
    q->end = 0;
    q->nbmax = 0;
    q->nb = 0;
}

void printJobs(struct tsp_queue *q) {
    printf("NbJobs: ");
    pthread_mutex_lock(&solution_mutex);
    int compteur = 0;
    if (q){
	struct tsp_cell *temp = q->first;
	while (temp) {
	    ++compteur;
	    temp = temp->next;
	}
    }
    printf("%d\n", compteur);
    pthread_mutex_unlock(&solution_mutex);
}

int empty_queue (struct tsp_queue *q) {
    pthread_mutex_lock(&q->jobs_mutex);
    int ret = ((q->first == 0) && (q->end == 1));
    pthread_mutex_unlock(&q->jobs_mutex);

    return ret;
}

void add_job (struct tsp_queue *q, tsp_path_t p, int hops, int len, uint64_t vpres) {
    struct tsp_cell *ptr;

    ptr = malloc (sizeof (*ptr));
    if (!ptr) {
	printf ("L'allocation a echoue, recursion infinie ?\n");
	exit (1);
    }
    ptr->next = 0;
    ptr->tsp_job.len = len;
    ptr->tsp_job.hops = hops;
    ptr->tsp_job.vpres = vpres;
    memcpy (ptr->tsp_job.path, p, hops * sizeof(p[0]));

    if (q->first == 0) {
	q->first = q->last = ptr;
    } else {
	q->last->next = ptr;
	q->last = ptr;
    }
    q->nbmax ++;
    q->nb ++;
}

int get_job (struct tsp_queue *q, tsp_path_t p, int *hops, int *len, uint64_t *vpres) {
    pthread_mutex_lock(&q->jobs_mutex);
    struct tsp_cell *ptr;

    if (q->first == 0) {
	pthread_mutex_unlock(&q->jobs_mutex);

	return 0;
    }

    ptr = q->first;

    q->first = ptr->next;
    if (q->first == 0) {
	q->last = 0;
    }

    *len = ptr->tsp_job.len;
    *hops = ptr->tsp_job.hops;
    *vpres = ptr->tsp_job.vpres;

    memcpy (p, ptr->tsp_job.path, *hops * sizeof(p[0]));

    free (ptr);

    q->nb --;
    if (affiche_progress)
	printf("<!- %d / %d %% ->\n",q->nb, q->nbmax);

    pthread_mutex_unlock(&q->jobs_mutex);

    return 1;
}

void no_more_jobs (struct tsp_queue *q) {
    q->end = 1;
}
