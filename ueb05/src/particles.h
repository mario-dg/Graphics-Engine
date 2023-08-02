/**
 * Partikelsimulationsmodul.
 * 
 * Copyright (C) 2020, FH Wedel
 * Autor: Nicolas Hollmann
 */

#ifndef PARTICLES_H
#define PARTICLES_H

#include "common.h"

//////////////////////////// ÖFFENTLICHE FUNKTIONEN ////////////////////////////


/**
 * Initialisiert alle Partikel und ihre Shader
 * 
 * @param ctx Programmkontext.
 */
void particles_init(ProgContext* ctx);

/**
 * Updatet die Partikel und uebergibt Daten an den
 * Compute-Shader weiter.
 * 
 * @param ctx Programmkontext.
 */
void particles_update(ProgContext* ctx);

/**
 * Zeichnen der Partikel
 * Setzt alle benoetigeten Parameter und gibt diese
 * and die Shader weiter.
 * @param viewPojMat View-Projektions-Matrix
 * @param ctx Programmkontext.
 */
void particles_draw(ProgContext* ctx, mat4 viewProjMat);

/**
 * Entfernt alle Daten beim Beenden des Programms
 * 
 * @param ctx Programmkontext.
 */
void particles_cleanup(ProgContext* ctx);


#endif // PARTICLES_H
