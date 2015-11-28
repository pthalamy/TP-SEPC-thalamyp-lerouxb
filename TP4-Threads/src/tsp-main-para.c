#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>
#include <time.h>
#include <assert.h>
#include <complex.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>

#include "tsp-types.h"
#include "tsp-job.h"
#include "tsp-genmap.h"
#include "tsp-print.h"
#include "tsp-tsp.h"
#include "tsp-lp.h"
#include "tsp-hkbound.h"


/* macro de mesure de temps, retourne une valeur en nanosecondes */
#define TIME_DIFF(t1, t2)						\
    ((t2.tv_sec - t1.tv_sec) * 1000000000ll + (long long int) (t2.tv_nsec - t1.tv_nsec))

static void *END_SUCCESS = (void *)123456789L;

/* tableau des distances */
tsp_distance_matrix_t tsp_distance ={};

/** Paramètres **/

/* nombre de villes */
int nb_towns=10;
/* graine */
long int myseed= 0;
/* nombre de threads */
int nb_threads=1;

/* affichage SVG */
bool affiche_sol= false;
bool affiche_progress=false;
bool quiet=false;

/* Variables globales pour accès concurrent*/
tsp_path_t solution;
long long int cuts = 0;
int sol_len= 0;

typedef struct {
    struct tsp_queue q;
    uint64_t vpres;
    tsp_path_t *sol;
} thread_args;

static void *compute_jobs(void * args) {
    printf("Thread %lx starts!\n", pthread_self());

    thread_args *my_args = args;

    struct tsp_queue q = my_args->q;
    uint64_t vpres = my_args->vpres;
    tsp_path_t *sol = my_args->sol;

    while (!empty_queue (&q)) {
        int hops = 0, len = 0;
        get_job (&q, solution, &hops, &len, &vpres);

	// le noeud est moins bon que la solution courante
	pthread_mutex_lock(&minMut); 
	if (minimum < INT_MAX
	    && (nb_towns - hops) > 10
	    && ( (lower_bound_using_hk(solution, hops, len, vpres)) >= minimum
		 || (lower_bound_using_lp(solution, hops, len, vpres)) >= minimum)
	    ){
	    pthread_mutex_unlock(&minMut);
	    continue;
	}
	tsp (hops, len, vpres, solution, &cuts, *sol, &sol_len);
	pthread_mutex_unlock(&minMut);
    }

    printf("Thread %lx terminates!\n", pthread_self());

    return END_SUCCESS;
}

static void generate_tsp_jobs (struct tsp_queue *q, int hops, int len, uint64_t vpres,
			       tsp_path_t path, long long int *cuts, tsp_path_t sol,
			       int *sol_len, int depth)
{
    if (len >= minimum) {
        (*cuts)++ ;
        return;
    }

    if (hops == depth) {
        /* On enregistre du travail à faire plus tard... */
	add_job (q, path, hops, len, vpres);
    } else {
        int me = path [hops - 1];
        for (int i = 0; i < nb_towns; i++) {
	    if (!present (i, hops, path, vpres)) {
                path[hops] = i;
		vpres |= (1<<i);
                int dist = tsp_distance[me][i];
                generate_tsp_jobs (q, hops + 1, len + dist, vpres, path, cuts, sol, sol_len, depth);
		vpres &= (~(1<<i));
            }
        }
    }
}

static void usage(const char *name) {
    fprintf (stderr, "Usage: %s [-s] <ncities> <seed> <nthreads>\n", name);
    exit (-1);
}

int main (int argc, char **argv)
{
    unsigned long long perf;
    tsp_path_t path;
    uint64_t vpres=0;
    tsp_path_t sol;
    struct tsp_queue q;
    struct timespec t1, t2;

    /* lire les arguments */
    int opt;
    while ((opt = getopt(argc, argv, "spq")) != -1) {
	switch (opt) {
	case 's':
	    affiche_sol = true;
	    break;
	case 'p':
	    affiche_progress = true;
	    break;
	case 'q':
	    quiet = true;
	    break;
	default:
	    usage(argv[0]);
	    break;
	}
    }

    if (optind != argc-3)
	usage(argv[0]);

    nb_towns = atoi(argv[optind]);
    myseed = atol(argv[optind+1]);
    nb_threads = atoi(argv[optind+2]);
    assert(nb_towns > 0);
    assert(nb_threads > 0);
    pthread_mutex_init(&minMut, NULL);
    
    minimum = INT_MAX;

    /* generer la carte et la matrice de distance */
    if (! quiet)
	fprintf (stderr, "ncities = %3d\n", nb_towns);
    genmap ();

    init_queue (&q);

    clock_gettime (CLOCK_REALTIME, &t1);

    memset (path, -1, MAX_TOWNS * sizeof (int));
    path[0] = 0;
    vpres=1;

    /* mettre les travaux dans la file d'attente */
    generate_tsp_jobs (&q, 1, 0, vpres, path, &cuts, sol, & sol_len, 3);
    no_more_jobs (&q);

    /* Preparation au calcul de chacun des travaux en parallèle */
    memset (solution, -1, MAX_TOWNS * sizeof (int));
    solution[0] = 0;

    /* Mise en place des arguments */
    thread_args compute_jobs_args;
    compute_jobs_args.q = q;
    compute_jobs_args.vpres = vpres;
    compute_jobs_args.sol = &sol;

    /* Creation des threads pour le calcul concurrentiel */
    pthread_t threads_pid[nb_threads];
    for (int i = 0; i < nb_threads; i++) {
	pthread_create(&threads_pid[i], NULL, compute_jobs , (void *)&compute_jobs_args);
    }

    /* On attend qu'ils aient tous terminé pour continuer */
    void *status;
    for (int i = 0; i < nb_threads; i++) {
	pthread_join(threads_pid[i], &status);
	if (status != END_SUCCESS)
	    fprintf(stderr,
		    "error: Thread %i didn't terminate correctly - sucess =  %p\n",
		    i, status);
    }

    clock_gettime (CLOCK_REALTIME, &t2);

    if (affiche_sol)
	print_solution_svg (sol, sol_len);

    perf = TIME_DIFF (t1,t2);
    printf("<!-- # = %d seed = %ld len = %d threads = %d time = %lld.%03lld ms ( %lld coupures ) -->\n",
	   nb_towns, myseed, sol_len, nb_threads,
	   perf/1000000ll, perf%1000000ll, cuts);

    return 0 ;
}
