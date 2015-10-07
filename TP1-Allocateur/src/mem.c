/*****************************************************
 * Copyright Grégory Mounié 2008-2013                *
 * This code is distributed under the GLPv3 licence. *
 * Ce code est distribué sous la licence GPLv3+.     *
 *****************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <stdbool.h>
#include "mem.h"

#define ADD_SIZE sizeof(void *)

/** squelette du TP allocateur memoire */

void *zone_memoire = 0;

/* TZL sous forme de tableau de tableaux */
uintptr_t *TZL[20] = {NULL};	/* TODO: VARIABLE INDEX */

void print_blocList(uintptr_t *head)
{
    uintptr_t *cour = head;

    while (*cour) {
	printf ("%p -> ", (void *)(*cour));
	cour = (uintptr_t *)(*cour);
    }

    printf ("NULL\n");
}

void insert_bloc_head(uintptr_t *ptr, int size) {

    if (TZL[size] == NULL) { // Il n'y a pas encore de blocs de cette taille
	TZL[size] = ptr;
	*TZL[size] = 0;
    }
    // Sinon on ajoute en tete
    else {
	uintptr_t *temp = TZL[size];
	TZL[size] = ptr;
	ptr = temp;
    }

}

bool find_and_delete(uintptr_t ptr, int size) {

    uintptr_t *suiv, *cour = TZL[size];

    /* Le bloc est en tête de liste */
    if (*cour == ptr) {
        TZL[size] = (uintptr_t *)ptr;
	return true;
    } else {			/*  */
	while ((uintptr_t *)(*cour) != NULL) {
	    suiv = (uintptr_t *)(*cour);
	    if (*suiv == ptr) {
		*cour = *suiv;
		return true;
	    }
	    cour = suiv;
	}
    }
	return false;
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
    TZL[20] = (uintptr_t *)zone_memoire;
    *TZL[20] = 0;

    return 0;;
}

void *mem_alloc(unsigned long size) {
    /*  ecrire votre code ici */
    /* Check que size > 0 */
    /* Regarde si TZL[log2(size - 1) + 1] comprend un bloc libre  */
    /* Si oui, retire de la tzl et retourne l'@ associée */
    /* Sinon, recherche récursive d'un bloc d'ordre supérieur à diviser */


    return 0;
}

int mem_free(void *ptr, unsigned long size) {
    /* ecrire votre code ici */
    /* uint16_t indice = pow(2, log(size - 1) / log(2) + 1); */
    /* xor entre @ et log2(size) + ? afin de trouver le buddy */
    /* void *buddy = *ptr ^ indice; */

    /* Si present dans TZL, fusion jusqu'ā ce que le bloc atteigne la taille MAX */
    /*                      ou qu'un buddy manque */
    /* while (search_and_remove_bloc(TZL[indice], buddy)) { */
    /* 	++indice; */
    /* 	if (indice == log(ALLOC_MEM_SIZE) / log(2)) */
    /* 	    break; */
    /* } */

    /* Sinon, on se contente d'ajouter le bloc libéré à la TZL */
    /* insert_bloc(&TZL[indice], ptr); */

    return 0;
}


int mem_destroy() {
    /* ecrire votre code ici */


    free(zone_memoire);
    zone_memoire = 0;
    return 0;
}
