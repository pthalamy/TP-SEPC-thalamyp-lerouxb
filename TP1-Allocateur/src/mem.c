/*****************************************************
 * Copyright Grégory Mounié 2008-2013                *
 * This code is distributed under the GLPv3 licence. *
 * Ce code est distribué sous la licence GPLv3+.     *
 *****************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "mem.h"

/** squelette du TP allocateur memoire */

void *zone_memoire = 0;

/* STRUCTURES: */
/* Blocs de la forme (suivant, ...) */
typedef struct _BlocZL {
    struct _BlocZL *next;
    struct _BlocZL *prev;
    void *addr;
} BlocZL;

/* TZL sous forme de tableau de blocs? */
BlocZL **TZL;

/* static void enqueue_bloc(BlocZL *head, BlocZL b) */
/* { */
/*     return; */
/* } */

/* static BlocZL *search_bloc(BlocZL *head, void *addr) */
/* { */
/*     return NULL; */
/* } */

int mem_init()
{
    if (! zone_memoire)
	zone_memoire = (void *) malloc(ALLOC_MEM_SIZE);
    if (zone_memoire == 0) {
	perror("mem_init:");
	return -1;
    }

    /* ecrire votre code ici */
    /* Allocation de la table de TZL de taille log2(ALLOC_MEM_SIZE) + 1 */
    /* TZL vide sauf deniere case, bloc memoire complet */

    return 0;
}

void *mem_alloc(unsigned long size)
{
    /*  ecrire votre code ici */
    /* Check que size > 0 */
    /* Regarde si TZL[log2(size - 1) + 1] comprend un bloc libre  */
    /* Si oui, retire de la tzl et retourne l'@ associée */
    /* Sinon, recherche récursive d'un bloc d'ordre supérieur à diviser */

    return 0;
}

int mem_free(void *ptr, unsigned long size)
{
    /* ecrire votre code ici */
    /* xor entre @ et log2(size) + ? afin de trouver le buddy */
    /* Si present dans TZL, fusion jusqu'ā ce que le bloc atteigne la taille MAX */
    /*                      ou qu'un buddy manque */
    /* Sinon, on se contente d'ajouter le bloc libéré à la TZL */

    return 0;
}


int mem_destroy()
{
    /* ecrire votre code ici */
    /* Free des cellules des listes de la TZL, puis free de la TZL */

    free(zone_memoire);
    zone_memoire = 0;
    return 0;
}
