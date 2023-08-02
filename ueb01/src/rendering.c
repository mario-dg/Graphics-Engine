/**
 * Modul zum Rendern der 3D Szene.
 * 
 * Copyright (C) 2020, FH Wedel
 * Autor: Nicolas Hollmann
 */

#include "rendering.h"

#include <string.h>

#include "shader.h"
#include "model.h"
#include "utils.h"
#include "input.h"
#include "camera.h"
#include "material.h"
#include "texture.h"

////////////////////////////// LOKALE DATENTYPEN ///////////////////////////////

// Datentyp für alle persistenten Daten des Renderers.
struct RenderingData
{
    Shader *modelShader;
    Shader *skybox;
};
typedef struct RenderingData RenderingData;

GLuint g_skymap_vao;
GLuint g_skymap_vbo;
GLuint g_tex_cube;
////////////////////////////// LOKALE FUNKTIONEN ///////////////////////////////

/**
 * Lädt alle Shader, die im Rendering-Modul verwendet werden.
 * 
 * @param data Zugriff auf das Rendering Datenobjekt.
 */
static void rendering_loadShaders(RenderingData *data)
{
    data->modelShader = shader_createVeFrShader(
        UTILS_CONST_RES("shader/model/model.vert"),
        UTILS_CONST_RES("shader/model/model.frag"));
    data->skybox = shader_createVeFrShader(
        UTILS_CONST_RES("shader/skybox/skybox.vert"),
        UTILS_CONST_RES("shader/skybox/skybox.frag"));
}

//////////////////////////// ÖFFENTLICHE FUNKTIONEN ////////////////////////////

void rendering_reRenderShaders(ProgContext *ctx)
{
    Shader *tempModel = shader_createVeFrShader(
        UTILS_CONST_RES("shader/model/model.vert"),
        UTILS_CONST_RES("shader/model/model.frag"));
        
    Shader *tempSkyBox = shader_createVeFrShader(
        UTILS_CONST_RES("shader/skybox/skybox.vert"),
        UTILS_CONST_RES("shader/skybox/skybox.frag"));

    if (tempModel != NULL)
    {
        shader_deleteShader(ctx->rendering->modelShader);
        ctx->rendering->modelShader = tempModel;
    }

    if(tempSkyBox != NULL){
        shader_deleteShader(ctx->rendering->skybox);
        ctx->rendering->skybox = tempSkyBox;
    }
}

void rendering_init(ProgContext *ctx)
{
    model_createCube(&g_skymap_vbo, &g_skymap_vao);
    texture_create_cube_map("../res/textures/nz.png", "../res/textures/pz.png", "../res/textures/py.png", "../res/textures/ny.png", "../res/textures/nx.png", "../res/textures/px.png", &g_tex_cube);
    ctx->rendering = malloc(sizeof(RenderingData));
    RenderingData *data = ctx->rendering;

    // Speicher bereinigen.
    memset(data, 0, sizeof(RenderingData));

    // OpenGL Flags setzen.
    glCullFace(GL_BACK); // setzten Faceculling auf Back-Face
    glFrontFace(GL_CCW); // front Faces sind gegen den Uhrzeigersinn

    // Alle Shader laden.
    rendering_loadShaders(data);
}

void rendering_draw(ProgContext *ctx)
{
    RenderingData *data = ctx->rendering;
    InputData *input = ctx->input;

    // Bildschirm leeren.
    glClearColor(
        input->rendering.clearColor[0],
        input->rendering.clearColor[1],
        input->rendering.clearColor[2],
        input->rendering.clearColor[3]);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Überprüfen, ob der Wireframe Modus verwendet werden soll.
    if (input->showWireframe)
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glDisable(GL_CULL_FACE); // deaktivierung des Face Cullings
    }
    else
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glEnable(GL_CULL_FACE); // aktivierung des Face Cullings
    }

    // Tiefentest aktivieren.
    glEnable(GL_DEPTH_TEST);

    // Zuerst die Projection Matrix aufsetzen.
    mat4 projectionMatrix;
    float aspect = (float)ctx->winData->width / (float)ctx->winData->height;
    float zoom = camera_getZoom(input->mainCamera);
    glm_perspective(glm_rad(zoom), aspect, 0.1f, 200.0f, projectionMatrix);

    // Dann die View-Matrix bestimmen.
    mat4 viewMatrix;
    camera_getViewMatrix(input->mainCamera, viewMatrix);

    // Das Nutzermodell nur dann Rendern, wenn es existiert.
    if (input->rendering.userModel)
    {
        // objectMatrix festlegen.
        mat4 objectMatrix;
        glm_mat4_identity(objectMatrix);
        //Modell rotieren
        glm_rotate_x(objectMatrix, input->rendering.modelRotation[0], objectMatrix);
        glm_rotate_y(objectMatrix, input->rendering.modelRotation[1], objectMatrix);
        glm_rotate_z(objectMatrix, input->rendering.modelRotation[2], objectMatrix);
        //Modell skalieren
        glm_scale_uni(objectMatrix, input->rendering.scale);

        mat4 modelMatrix;
        glm_mat4_identity(modelMatrix);
        mat4 modelViewMatrix;
        glm_mat4_mul(modelMatrix, viewMatrix, modelViewMatrix);

        // MVP Matrix vorberechnen.
        mat4 mvpMatrix;
        glm_mat4_mul(projectionMatrix, viewMatrix, mvpMatrix);
        glm_mat4_mul(mvpMatrix, modelMatrix, mvpMatrix);


        /* ---------------------- MODEL - SHADER ---------------------------------- */
        // Shader vorbereiten.
        shader_useShader(data->modelShader);

        //MVP-Matrix an Shader schicken
        shader_setMat4(data->modelShader, "mvpMatrix", &mvpMatrix);

        //Projektions-Matrix an Shader schicken
        shader_setMat4(data->modelShader, "projectionMatrix", &projectionMatrix);

        //ModelView-Matrix an Shader schicken
        shader_setMat4(data->modelShader, "modelViewMatrix", &modelViewMatrix);
        shader_setMat4(data->modelShader, "modelMatrix", &modelMatrix);

        //Licht Richtung und Licht Farbe an Shader schicken
        shader_setVec3(data->modelShader, "lightDir", &(input->rendering.lightDirection));
        shader_setVec3(data->modelShader, "lightColor", &(input->rendering.lightColor));
        shader_setVec3(data->modelShader, "camPos", camera_getCameraPos(input->mainCamera));

        shader_setInt(data->modelShader, "displayOpt", input->rendering.displayOpt);
        // Modell zeichnen
        model_drawModel(input->rendering.userModel, data->modelShader);

        /* ---------------------- Skybox - SHADER ---------------------------------- */
        shader_useShader(data->skybox);
        shader_setMat4(data->skybox, "projection", &projectionMatrix);
        shader_setMat4(data->skybox, "view", &viewMatrix);
        shader_setInt(data->skybox, "skybox", 0);
        model_drawCubeMap(data->skybox, &g_skymap_vao, &g_tex_cube);
    }

    // Tiefentest nach der 3D Szene wieder deaktivieren.
    glDisable(GL_DEPTH_TEST);

    // Wireframe am Ende wieder deaktivieren.
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void rendering_cleanup(ProgContext *ctx)
{
    RenderingData *data = ctx->rendering;

    // Zum Schluss müssen noch die belegten Ressourcen freigegeben werden.
    shader_deleteShader(data->modelShader);
    shader_deleteShader(data->skybox);

    free(ctx->rendering);
}