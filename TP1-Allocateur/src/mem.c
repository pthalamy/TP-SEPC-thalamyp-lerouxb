/*****************************************************
 * Copyright Grégory Mounié 2008-2013                *
 * This code is distributed under the GLPv3 licence. *
 * Ce code est distribué sous la licence GPLv3+.     *
 *****************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include "mem.h"

/** squelette du TP allocateur memoire */

void *zone_memoire = 0;

/* TZL sous forme de tableau de tableaux */
int32_t TZL[21][ALLOC_MEM_SIZE] = {{0}};

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


int mem_destroy()
{
    /* ecrire votre code ici */
    /* Free des cellules des listes de la TZL, puis free de la TZL */

    free(zone_memoire);
    zone_memoire = 0;
    return 0;
}
