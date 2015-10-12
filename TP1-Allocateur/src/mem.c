/*****************************************************
 * Copyright Grégory Mounié 2008-2013                *
 * This code is distributed under the GLPv3 licence. *
 * Ce code est distribué sous la licence GPLv3+.     *
 *****************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "mem.h"

#define DEBUG 0

#define ADD_SIZE sizeof(void *)

/** squelette du TP allocateur memoire */

void *zone_memoire = 0;
bool mem_initialized = false;

/* TZL sous forme de tableau de tableaux */
uintptr_t *TZL[BUDDY_MAX_INDEX + 1] = {NULL};

static uint16_t get_pow_sup(unsigned long size) {

    int16_t i = BUDDY_MAX_INDEX;

    while (((size - 1) >> i) == 0 && i >= 0)
	--i;

    return (uint16_t)i+1;
}


void print_blocList(uintptr_t *head)
{
    uintptr_t *cour = head;

    if (head) {
	while (cour) {
	    printf ("%p -> ", cour);
	    cour = (uintptr_t *)(*cour);
	}
    }
    printf ("NULL\n");
}


void insert_bloc_head(uintptr_t *ptr, uint16_t indice) {
#if DEBUG
    printf("Insertion de %p dans TZL[%d] = ", ptr, indice);
    print_blocList(TZL[indice]);
#endif

    if (TZL[indice] == NULL) { // Il n'y a pas encore de blocs de cette taille
#if DEBUG
	printf("Liste vide\n");
#endif
	TZL[indice] = ptr;
	*TZL[indice] = (uintptr_t)NULL;
    }
    // Sinon on ajoute en tete
    else {
#if DEBUG
	printf("Liste non vide\n");
#endif
	uintptr_t *temp = TZL[indice];
	TZL[indice] = ptr;
	*TZL[indice] = (uintptr_t)temp;
    }
#if DEBUG
    printf("Fin Insertion. TZL[%d] = ", indice);
    print_blocList(TZL[indice]);
#endif

}


bool find_and_delete(uintptr_t *ptr, uint16_t ordre) {
#if DEBUG
    printf("Try to find and delete %p in TZL[%d] = ", ptr, ordre);
    print_blocList(TZL[ordre]);
#endif

    uintptr_t *suiv, *cour = TZL[ordre];

    if (TZL[ordre] == NULL)
	return false;

    /* Le bloc est en tête de liste */
    if (cour == ptr) {
	TZL[ordre] = (uintptr_t *)(*ptr);
#if DEBUG
	printf("End delete. TZL[%d] = ", ordre);
	print_blocList(TZL[ordre]);
#endif

	return true;
    }

    else {
	while ((*cour) != 0) {
	    suiv = (uintptr_t *)(*cour);
	    if ((uintptr_t *)(*suiv) == ptr) {
		cour = (uintptr_t *)(*suiv);
#if DEBUG
		printf("End delete. TZL[%d] = ", ordre);
		print_blocList(TZL[ordre]);
#endif

		return true;
	    }
	    cour = (uintptr_t *)(*suiv);
	}
    }
#if DEBUG
    printf("End delete. TZL[%d] = ", ordre);
    print_blocList(TZL[ordre]);
#endif
    return false;
}

static int divide_block (uint16_t order)
{
    if (order > 20) {
	fprintf(stderr, "error: Not enough space! \n");
	return -1;
    }

    if (TZL[order] == NULL)
        divide_block(order + 1);

    insert_bloc_head((uintptr_t *)TZL[order], order - 1);
    insert_bloc_head((uintptr_t *)((uintptr_t)TZL[order] + (1 << (order-1))), order - 1);
#if DEBUG
    print_blocList(TZL[order-1]);
#endif

    find_and_delete(TZL[order], order);

    return 0;
}

int mem_init() {
    if (! zone_memoire)
	zone_memoire = (void *) malloc(ALLOC_MEM_SIZE);
    if (zone_memoire == 0) {
	perror("mem_init:");
	return -1;
    }

    /* ecrire votre code ici */
    /* On entre l'adresse dans la zone_memoire */
    /* TZL vide sauf deniere case, bloc memoire complet */
    TZL[BUDDY_MAX_INDEX] = (uintptr_t *)zone_memoire;
    *TZL[BUDDY_MAX_INDEX] = 0;

    printf ("init mem @%p\n", zone_memoire);

    mem_initialized = true;

    return 0;
}

void *mem_alloc(unsigned long size) {

    if (!mem_initialized) {
	fprintf (stderr,
		 "error: Memory has not been initialized!\n");
	return NULL;
    }

    /* Check que size > 0 */
    if (size <= 0) {
	fprintf (stderr,
		 "error: Allocation of size %ld forbidden!\n",
		 size);
	return NULL;
    }

    /* Regarde si TZL[log2(size - 1) + 1] comprend un bloc libre  */
    /* Si oui, retire de la tzl et retourne l'@ associée */
    uint16_t order = get_pow_sup(size);

    if (!TZL[order]) {
	if (divide_block (order+1))
	    return NULL;
    }

    uintptr_t *freeBlock = TZL[order];
    find_and_delete (freeBlock, order);

#if DEBUG
    printf("Block de 2^%d alloue a l'@ %p\n", order, freeBlock);
#endif
    return (void *)freeBlock;
}

int mem_free(void *ptr, unsigned long size) {

    uintptr_t *PTR = (uintptr_t *)ptr;
    uint16_t indice = get_pow_sup(size);

    /* xor entre @ et 2^indice afin de trouver le buddy */
    uintptr_t buddy = (uintptr_t)PTR ^ (1<<indice);

    /* Si present dans TZL, fusion jusqu'ā ce que le bloc atteigne la taille MAX */
    /*                      ou qu'un buddy manque */
    while (find_and_delete((uintptr_t *)buddy, indice)) {
#if DEBUG
	printf ("buddy @%p of size %ld found!\n", (void*)buddy, size);
#endif
     	++indice;
    	if (indice == BUDDY_MAX_INDEX)
    	    break;
	buddy = (uintptr_t)PTR ^ (1 << indice);
    }

    /* Puis on l'ajoute a la bonne place */
    insert_bloc_head(PTR, indice);

    return 0;
}


int mem_destroy() {
    /* ecrire votre code ici */
    mem_initialized = false;
    printf ("destroy mem @%p\n", zone_memoire);

    free(zone_memoire);
    zone_memoire = 0;
    return 0;
}
