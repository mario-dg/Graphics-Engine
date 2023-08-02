/**
 * Modul zum Rendern der 3D Szene.
 * 
 * Copyright (C) 2020, FH Wedel
 * Autor: Nicolas Hollmann
 */

#ifndef RENDERING_H
#define RENDERING_H

#include "common.h"
#include "framebuffer.h"
#include "shader.h"
#include "mesh.h"
#include "skybox.h"

//////////////////////////// ÖFFENTLICHE FUNKTIONEN ////////////////////////////

// Datentyp für alle persistenten Daten des Renderers.
struct RenderingData
{
    Framebuffer fb;
    PingPong pingPong;
    depthFBO depthFBO;
    depthCubeFBO depthCubeFBO;
    Shader *modelShader;
    Shader *skyboxShader;
    SkyBox skyBox;
    Shader *dirLight;
    Shader *pointLight;
    Shader *postProcessing;
    Shader *null;
    Shader *threshhold;
    Shader *blur;
    Shader *dirShadow;
    Shader *pointShadow;
    Shader *particles;
    Mesh *displayQuad;
};
typedef struct RenderingData RenderingData;

/**
 * Initialisiert das Rendering-Modul. 
 * 
 * @param ctx Programmkontext.
 */
void rendering_init(ProgContext* ctx);

/**
 * Rendert die 3D Szene. 
 * 
 * @param ctx Programmkontext.
 */
void rendering_draw(ProgContext* ctx);

/**
 * Gibt die Ressourcen des Rendering-Moduls wieder frei.
 * 
 * @param ctx Programmkontext.
 */
void rendering_cleanup(ProgContext* ctx);

void rendering_reRenderShaders(ProgContext* ctx);

#endif // RENDERING_H
