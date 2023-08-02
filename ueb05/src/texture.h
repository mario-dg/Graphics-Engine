/**
 * Modul für das Laden und Schreiben von Texturen.
 * 
 * Copyright (C) 2020, FH Wedel
 * Autor: Nicolas Hollmann
 */

#ifndef TEXTURE_H
#define TEXTURE_H

#include "common.h"

//////////////////////////// ÖFFENTLICHE FUNKTIONEN ////////////////////////////

/**
 * Erzeugt eine OpenGL Textur aus einer Bilddatei.
 * Es werden auch DDS Dateien unterstützt.
 * 
 * Im Fehlerfall wird immer eine korrekte Textur-ID zurückgegeben. Allerdings
 * fehlen unter umständen die nötigen Bilddaten.
 * 
 * @param filename der Pfad zur Bilddatei
 * @param wrapping der Wrapping Modus (z.B. GL_REPEAT, GL_MIRRORED_REPEAT, 
 *        GL_CLAMP_TO_EDGE, GL_CLAMP_TO_BORDER)
 * @return eine OpenGL Textur ID
 */
GLuint texture_loadTexture(const char* filename, GLenum wrapping, GLboolean diffuse);

/**
 * Löscht eine zuvor angelegte Textur wieder.
 * Die Textur-ID muss valide und noch nicht gelöscht sein.
 * 
 * @param textureId die Textur-ID der Textur, die gelöscht werden soll.
 */
void texture_deleteTexture(GLuint textureId);

/**
 * Speichert einen Screenshot in dem Programmverzeichnis.
 * Der Dateiname lautet screenshot_yyyy-MM-dd_hh-mm-ss.png wobei das aktuelle
 * Datum und die aktuelle Uhrzeit eingesetzt wird.
 * Es wird grundsätzlich der aktive Framebuffer ausgelesen.
 * 
 * @param ctx der aktuelle Programmkontext
 */
void texture_saveScreenshot(ProgContext* ctx);

/**
 * Gibt den reservierten Speicher vom Texture Cache wieder frei
 */ 
void deleteTextureCache();

void texture_create_cube_map(
    const char *front,
    const char *back,
    const char *top,
    const char *bottom,
    const char *left,
    const char *right,
    GLuint *texture);

#endif // TEXTURE_H
