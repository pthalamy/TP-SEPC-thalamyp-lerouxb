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

#define ADD_SIZE sizeof(void *)

/** squelette du TP allocateur memoire */

void *zone_memoire = 0;

/* TZL sous forme de tableau de tableaux */
uintptr_t *TZL[21] = {NULL};	/* TODO: VARIABLE INDEX */

static uint16_t get_pow_sup(unsigned long size) {

    uint16_t i = 31;

    while (((size - 1)>>i) > 0 && i >= 0)
	--i;

    return i+1;
}


void print_blocList(uintptr_t *head)
{
    uintptr_t *cour = head;

    while (*cour) {
	printf ("%p -> ", (void *)(*cour));
	cour = (uintptr_t *)(*cour);
    }

    printf ("NULL\n");
}


void insert_bloc_head(uintptr_t *ptr, uint16_t indice) {

    if (TZL[indice] == NULL) { // Il n'y a pas encore de blocs de cette taille
	TZL[indice] = ptr;
	*TZL[indice] = 0;
    }
    // Sinon on ajoute en tete
    else {
	uintptr_t *temp = TZL[indice];
	TZL[indice] = ptr;
	ptr = temp;
    }

}


bool find_and_delete(uintptr_t *ptr, uint16_t indice) {

    uintptr_t *suiv, *cour = TZL[indice];

    /* Le bloc est en tête de liste */
    if (*cour == *ptr) {
        TZL[indice] = (uintptr_t *)ptr;
	return true;
    } else {			/*  */
	while ((uintptr_t *)(*cour) != NULL) {
	    suiv = (uintptr_t *)(*cour);
	    if (*suiv == *ptr) {
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
    if (size <= 0) {
	fprintf (stderr,
		 "erreur: Allocation d'une zone de taille %ld interdite !\n",
		 size);
	return 0;
    }

    /* Regarde si TZL[log2(size - 1) + 1] comprend un bloc libre  */
    /* Si oui, retire de la tzl et retourne l'@ associée */
    uintptr_t freeBlock = *TZL[size]; /* TODO: PROPER INDEX! */
    if (freeBlock) {
	find_and_delete ((uintptr_t *)freeBlock ,size); /* TODO: PROPER INDEX! 2 */
	return (void *)freeBlock;
    }
    /* Sinon, recherche récursive d'un bloc d'ordre supérieur à diviser */
    else {
	/* TODO */
	return 0;
    }
}

int mem_free(void *ptr, unsigned long size) {

    uintptr_t *PTR = (uintptr_t *)ptr;
    uint16_t indice = get_pow_sup(size);

    /* xor entre @ et 2^indice afin de trouver le buddy */
    uintptr_t buddy = *((uintptr_t *)PTR) ^ (1<<indice);

    /* Si present dans TZL, fusion jusqu'ā ce que le bloc atteigne la taille MAX */
    /*                      ou qu'un buddy manque */
    while (find_and_delete((uintptr_t *)buddy, indice)) {
    	++indice;
    	if (indice == 20)
    	    break;
	buddy = *((uintptr_t *)PTR) ^ indice;
    }

    /* Puis on l'ajoute a la bonne place */
	insert_bloc_head(PTR, indice);

    return 0;
}


int mem_destroy() {
    /* ecrire votre code ici */


    free(zone_memoire);
    zone_memoire = 0;
    return 0;
}
