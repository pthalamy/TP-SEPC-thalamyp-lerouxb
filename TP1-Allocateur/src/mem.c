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

/*Macros*/
#define ADD_SIZE sizeof(void *)
#define MIN(a,b) (((a)<(b))?(a):(b))
#define BUDDY_ADDR(addr, order) \
    ( ( ((uintptr_t)(addr) - (uintptr_t)(zone_memoire)) \
       ^ (1 << (order))) + (uintptr_t)zone_memoire )

/**Definition des variables globales*/
void *zone_memoire = 0;
bool mem_initialized = false;

/* TZL sous forme de tableau de tableaux */
uintptr_t *TZL[BUDDY_MAX_INDEX + 1] = {NULL};

/**Fin des defintions*/


/** squelette du TP allocateur memoire */

/**Fonctions personnelles*/
static uint16_t get_pow_sup(unsigned long size)
{

    int16_t i = BUDDY_MAX_INDEX;

    while (((size - 1) >> i) == 0 && i >= 0)
	--i;

    return (uint16_t)i+1;
}


/* static void print_blocList(uintptr_t *head) */
/* { */
/*     uintptr_t *cour = head; */

/*     if (head) { */
/* 	while (cour) { */
/* 	    printf ("%p -> ", cour); */
/* 	    cour = (uintptr_t *)(*cour); */
/* 	} */
/*     } */
/*     printf ("NULL\n"); */
/* } */


/* static void print_TZL(void) */
/* { */
/*     for (int16_t i = BUDDY_MAX_INDEX; i >= 0; i--) { */
/* 	printf ("TZL[%d]: ", i); */
/* 	print_blocList (TZL[i]); */
/*     } */
/*     printf("\n"); */
/* } */


static bool find_and_delete(uintptr_t *ptr, uint16_t ordre)
{
    uintptr_t *suiv, *cour = TZL[ordre];
    if (ordre > BUDDY_MAX_INDEX)
	return false;
    else if (TZL[ordre] == NULL)
	return false;

    /* Le bloc est en tête de liste */
    if (cour == ptr) {
	TZL[ordre] = (uintptr_t *)(*ptr);
	return true;
    } else {
	while ((*cour) != 0) {

	    suiv = (uintptr_t *)(*cour);

	    if (suiv == ptr) {
		cour = (uintptr_t *)(*suiv);

		return true;
	    }
	    cour = suiv;
	}

    }
    return false;
}


static void insert_bloc_head(uintptr_t *ptr, uint16_t indice)
{

    if (TZL[indice] == NULL) { // Il n'y a pas encore de blocs de cette taille
	TZL[indice] = ptr;
	find_and_delete(ptr, indice + 1);
	*TZL[indice] = (uintptr_t)NULL;
    }
    // Sinon on ajoute en tete
    else {

	uintptr_t *temp = TZL[indice];
	TZL[indice] = ptr;
	find_and_delete(ptr, indice + 1);
	*TZL[indice] = (uintptr_t)temp;
    }
}


static int divide_block (uint16_t order)
{
    if (order > 20) {
	fprintf(stderr, "error: Not enough space! \n");
	return -1;
    }

    if (TZL[order] == NULL)
        divide_block(order + 1);

    uintptr_t *buddy = (uintptr_t *)BUDDY_ADDR(TZL[order], order-1);

    insert_bloc_head(TZL[order], order - 1);

    insert_bloc_head(buddy, order - 1);

    return 0;
}

/**Fin des fonctions personelles*/



int mem_init()
{
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

    mem_initialized = true;

    return 0;
}


void *mem_alloc(unsigned long size)
{

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

    return (void *)freeBlock;
}


int mem_free(void *ptr, unsigned long size)
{
    uintptr_t *PTR = (uintptr_t *)ptr;
    uint16_t indice = get_pow_sup(size);

    /* xor entre @ et 2^indice afin de trouver le buddy */
    uintptr_t buddy = BUDDY_ADDR(PTR, indice);

    /* Si present dans TZL, fusion jusqu'ā ce que le bloc atteigne la taille MAX */
    /*                      ou qu'un buddy manque */
    while (find_and_delete((uintptr_t *)buddy, indice) && indice <= BUDDY_MAX_INDEX) {

     	/*If we found a free buddy, we look for a larger one*/
	indice++;

	/*Update the ptr*/
	PTR = (uintptr_t *)MIN((uintptr_t)PTR, (uintptr_t)buddy);

	/*Find his new buddy*/
	buddy = BUDDY_ADDR(PTR, indice);
    }

    /* Puis on l'ajoute a la bonne place */
    insert_bloc_head(PTR, indice);

    return 0;
}


int mem_destroy()
{
    /* ecrire votre code ici */
    mem_initialized = false;

    free(zone_memoire);
    zone_memoire = 0;

    return 0;
}
