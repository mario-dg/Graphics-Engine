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

#define ROTATION_STEPS (3)
#define M_PI_F 3.14159265358979323846f
////////////////////////////// LOKALE DATENTYPEN ///////////////////////////////

// Datentyp für alle persistenten Daten des Renderers.
struct RenderingData
{
    Shader *modelShader;
    Shader *skybox;
    Shader *LightVis;
};
typedef struct RenderingData RenderingData;

GLuint g_skymap_vao;
GLuint g_skymap_vbo;
GLuint g_tex_cube;
GLuint g_depthMap;
GLfloat g_rotationAngle = 0.0f;
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
        UTILS_CONST_RES("shader/model/model.frag"),
        UTILS_CONST_RES("shader/model/model.tesc"),
        UTILS_CONST_RES("shader/model/model.tese"));
    data->skybox = shader_createVeFrShader(
        UTILS_CONST_RES("shader/skybox/skybox.vert"),
        UTILS_CONST_RES("shader/skybox/skybox.frag"),
        NULL,
        NULL);
    data->LightVis = shader_createVeFrShader(
        UTILS_CONST_RES("shader/LightVis/LightVis.vert"),
        UTILS_CONST_RES("shader/LightVis/LightVis.frag"),
        NULL,
        NULL);
}

//////////////////////////// ÖFFENTLICHE FUNKTIONEN ////////////////////////////

void rendering_reRenderShaders(ProgContext *ctx)
{
    Shader *tempModel = shader_createVeFrShader(
        UTILS_CONST_RES("shader/model/model.vert"),
        UTILS_CONST_RES("shader/model/model.frag"),
        UTILS_CONST_RES("shader/model/model.tesc"),
        UTILS_CONST_RES("shader/model/model.tese"));

    Shader *tempSkyBox = shader_createVeFrShader(
        UTILS_CONST_RES("shader/skybox/skybox.vert"),
        UTILS_CONST_RES("shader/skybox/skybox.frag"),
        NULL,
        NULL);
    
     Shader *tempLightVis = shader_createVeFrShader(
        UTILS_CONST_RES("shader/LightVis/LightVis.vert"),
        UTILS_CONST_RES("shader/LightVis/LightVis.frag"),
        NULL,
        NULL);

    if (tempModel != NULL)
    {
        shader_deleteShader(ctx->rendering->modelShader);
        ctx->rendering->modelShader = tempModel;
    }

    if (tempSkyBox != NULL)
    {
        shader_deleteShader(ctx->rendering->skybox);
        ctx->rendering->skybox = tempSkyBox;
    }

    if(tempLightVis != NULL) {
        shader_deleteShader(ctx->rendering->LightVis);
        ctx->rendering->LightVis = tempLightVis;
    }
}

void rendering_init(ProgContext *ctx)
{
    model_createCube(&g_skymap_vbo, &g_skymap_vao);
    texture_create_cube_map(UTILS_CONST_RES("textures/nz.png"),
                            UTILS_CONST_RES("textures/pz.png"),
                            UTILS_CONST_RES("textures/py.png"),
                            UTILS_CONST_RES("textures/ny.png"),
                            UTILS_CONST_RES("textures/nx.png"),
                            UTILS_CONST_RES("textures/px.png"), &g_tex_cube);
    ctx->rendering = malloc(sizeof(RenderingData));
    RenderingData *data = ctx->rendering;

    // Speicher bereinigen.
    memset(data, 0, sizeof(RenderingData));

    // OpenGL Flags setzen.
    glCullFace(GL_BACK); // setzten Faceculling auf Back-Face
    glFrontFace(GL_CCW); // front Faces sind gegen den Uhrzeigersinn

    // Alle Shader laden.
    rendering_loadShaders(data);

    //Depth Map laden
    shader_useShader(ctx->rendering->modelShader);
    g_depthMap = texture_loadTexture(UTILS_CONST_RES("textures/depthMap.dds"), GL_REPEAT);
}

/**
 * Rotiert ein Punktlicht
 * 
 * @param PointLight Punktlicht
 * @param ctx Kontext des Shaders
 */ 
void rotatePointLight(PointLight *light, ProgContext *ctx)
{
    //Rotation nur bei nicht Pausierung
    if (!light->paused)
    {
        //Berechnung des neuen Rotationswinkels
        g_rotationAngle += 360.0f * (1.0f / ROTATION_STEPS) * (GLfloat) ctx->winData->deltaTime;
        float rad = g_rotationAngle / 180.0f * M_PI_F;
        //Degree -> Rad
        float xRot = cosf(rad);
        float zRot = sinf(rad);
        light->position[0] = (xRot* light->radius + light->middle[0] ) ;
        light->position[1] = light->middle[1];
        light->position[2] = (zRot* light->radius + light->middle[2] ) ;
    }
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
        //Modell skalieren
        glm_scale_uni(objectMatrix, input->rendering.scale);

        mat4 modelViewMatrix;
        glm_mat4_mul(viewMatrix, objectMatrix, modelViewMatrix);

        // MVP Matrix vorberechnen.
        mat4 mvpMatrix;
        glm_mat4_mul(projectionMatrix, viewMatrix, mvpMatrix);
        glm_mat4_mul(mvpMatrix, objectMatrix, mvpMatrix);

        mat4 vpMatrix;
        glm_mat4_mul(projectionMatrix, viewMatrix, vpMatrix);

        /* ---------------------- MODEL - SHADER ---------------------------------- */
        // Shader vorbereiten.
        shader_useShader(data->modelShader);

        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_2D, g_depthMap);

        //Daten fuer die Tessellation an Shader uebergeben
        shader_setBool(data->modelShader, "useTessellation", input->rendering.useTessellation);
        shader_setFloat(data->modelShader, "innerTessellation", input->rendering.innerTessellation);
        shader_setFloat(data->modelShader, "outerTessellation", input->rendering.outerTessellation);

        //Daten fuer Displacement an Shader uebergeben
        shader_setInt(data->modelShader, "gDisplacementMap", 5);
        shader_setFloat(data->modelShader, "displacementFactor", input->rendering.displacementFactor);
        shader_setFloat(data->modelShader, "useDisplacement", input->rendering.useDisplacement);

        //MVP-Matrix an Shader schicken
        shader_setMat4(data->modelShader, "mvpMatrix", &mvpMatrix);

        shader_setInt(data->modelShader, "depthMap", 5);

        //Projektions-Matrix an Shader schicken
        shader_setMat4(data->modelShader, "projectionMatrix", &projectionMatrix);

        //ModelView-Matrix an Shader schicken
        shader_setMat4(data->modelShader, "modelViewMatrix", &modelViewMatrix);
        shader_setMat4(data->modelShader, "modelMatrix", &objectMatrix);

        shader_setVec3(data->modelShader, "rotation", &input->rendering.modelRotation);

        //Licht Richtung und Licht Farbe an Shader schicken
        shader_setVec3(data->modelShader, "lightDir", &(input->rendering.dirLight.dir));
        shader_setVec3(data->modelShader, "lightColor", &(input->rendering.dirLight.color));
        shader_setVec3(data->modelShader, "pointLight.position", &(input->rendering.pointLight.position));
        shader_setVec3(data->modelShader, "pointLight.middle", &(input->rendering.pointLight.middle));
        shader_setFloat(data->modelShader, "pointLight.constant", 1.0f);
        shader_setFloat(data->modelShader, "pointLight.linear", 0.09f);
        shader_setFloat(data->modelShader, "pointLight.quadratic", 0.032f);
        shader_setVec3(data->modelShader, "pointLight.ambient", &(input->rendering.pointLight.color));
        shader_setVec3(data->modelShader, "pointLight.diffuse", &(input->rendering.pointLight.color));
        shader_setVec3(data->modelShader, "pointLight.specular", &(input->rendering.pointLight.color));
        shader_setBool(data->modelShader, "usePointLight", input->rendering.pointLight.active);

        //Kameraposition und Display Optionen an Shader shicken
        shader_setVec3(data->modelShader, "camPos", camera_getCameraPos(input->mainCamera));
        shader_setInt(data->modelShader, "displayOpt", input->rendering.displayOpt);
        shader_setBool(data->modelShader, "useNormalMapping", input->rendering.useNormalMapping);
        shader_setBool(data->modelShader, "useParallaxMapping", input->rendering.useParallaxMapping);
        shader_setFloat(data->modelShader, "heightScale", input->rendering.heightScale);

        // Modell zeichnen
        model_drawModel(input->rendering.userModel, data->modelShader);
    }

        /* ---------------------- Skybox - SHADER ---------------------------------- */
        shader_useShader(data->skybox);
        shader_setMat4(data->skybox, "projection", &projectionMatrix);
        shader_setMat4(data->skybox, "view", &viewMatrix);
        shader_setInt(data->skybox, "skybox", 0);
        model_drawCubeMap(data->skybox, &g_skymap_vao, &g_tex_cube);

    //Punktlicht berechnen und zeichnen
    if(input->rendering.sphereModel) {
        //Punktlicht rotieren
        rotatePointLight(&(input->rendering.pointLight), ctx);
        mat4 sphereMatrix;
        glm_mat4_identity(sphereMatrix);
        glm_translate_x(sphereMatrix, input->rendering.pointLight.position[0]);
        glm_translate_y(sphereMatrix, input->rendering.pointLight.position[1]);
        glm_translate_z(sphereMatrix, input->rendering.pointLight.position[2]);
        glm_scale_uni(sphereMatrix, 0.05f);

        mat4 modelViewSMatrix;
        glm_mat4_mul(viewMatrix, sphereMatrix, modelViewSMatrix);

        // MVP Matrix vorberechnen.
        mat4 mvpMatrix;
        glm_mat4_mul(projectionMatrix, viewMatrix, mvpMatrix);
        glm_mat4_mul(mvpMatrix, sphereMatrix, mvpMatrix);

        //LightVis Shader benutzen und Daten einreichen
        shader_useShader(data->LightVis);
        shader_setMat4(data->LightVis, "mvpMatrix", &mvpMatrix);
        shader_setVec3(data->LightVis, "color", &input->rendering.pointLight.color);

        //LichtSphere zeichnen
        model_drawModelTris(input->rendering.sphereModel, data->LightVis);
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
    shader_deleteShader(data->LightVis);

    free(ctx->rendering);
}