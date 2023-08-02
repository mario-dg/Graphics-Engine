/**
 * Modul zum Rendern der 3D Szene.
 * 
 * Copyright (C) 2020, FH Wedel
 * Autor: Nicolas Hollmann
 */

#include "rendering.h"

#include <string.h>

#include "model.h"
#include "utils.h"
#include "input.h"
#include "camera.h"
#include "material.h"
#include "texture.h"
#include "deferredShader.h"
#include "postProcessing.h"
#include "skybox.h"
#include "shadowMapping.h"

#define ROTATION_STEPS (3)
#define M_PI_F 3.14159265358979323846f
////////////////////////////// LOKALE DATENTYPEN ///////////////////////////////
GLuint g_depthMap;
mat4 g_lightSpaceMat;
mat4 g_pointLightProj;
mat4 g_pointLightTransforms[6];
////////////////////////////// LOKALE FUNKTIONEN ///////////////////////////////

/**
 * Rendert das Finale Bild mit allen Lichtberechnungen und PostFX-Effekten
 * 
 * @param ctx Programmkontext
 */
static void rendering_drawFinalTex(ProgContext *ctx) {
    RenderingData *data = ctx->rendering;
    // Framebuffer binden: Lesen aus dem gBuffer-FBO, schreiben in das Default-FBO.
    glBindFramebuffer(GL_READ_FRAMEBUFFER, data->fb.fbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    // Es soll aus dem Final-Attachment gelesen werden.
    glReadBuffer(data->fb.attachments[GBUFFER_COLORATTACH_RESULT]);
    // Zum Schluss wird geblitted.
    glBlitFramebuffer(
            0, 0, ctx->winData->width, ctx->winData->height,
            0, 0, ctx->winData->width, ctx->winData->height,
            GL_COLOR_BUFFER_BIT, GL_LINEAR);
}

/**
 * Rendert einen Debug-Modus, der das Positions, Normal, ALbedoSpec und Emissions
 * Attachment anzeigt
 * 
 * @param ctx Programmkontext
 */
static void rendering_drawDebugMode(ProgContext *ctx) {
    // BLITTING
    ///////////
    RenderingData *data = ctx->rendering;
    // Framebuffer binden: Lesen aus dem gBuffer-FBO, schreiben in das Default-FBO.
    glBindFramebuffer(GL_READ_FRAMEBUFFER, data->fb.fbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    GLint halfWidth = ctx->winData->width / 2;
    GLint halfHeight = ctx->winData->height / 2;
    // Es soll aus dem Position-Attachment gelesen werden.
    glReadBuffer(data->fb.attachments[GBUFFER_COLORATTACH_POSITION]);
    // Zum Schluss wird geblitted.
    glBlitFramebuffer(
            0, 0, ctx->winData->width, ctx->winData->height,
            0, 0, halfWidth, halfHeight,
            GL_COLOR_BUFFER_BIT, GL_LINEAR);

    // Es soll aus dem Normal-Attachment gelesen werden.
    glReadBuffer(data->fb.attachments[GBUFFER_COLORATTACH_NORMAL]);
    // Zum Schluss wird geblitted.
    glBlitFramebuffer(
            0, 0, ctx->winData->width, ctx->winData->height,
            halfWidth, 0, ctx->winData->width, halfHeight,
            GL_COLOR_BUFFER_BIT, GL_LINEAR);

    // Es soll aus dem AlbedoSpec-Attachment gelesen werden.
    glReadBuffer(data->fb.attachments[GBUFFER_COLORATTACH_ALBEDOSPEC]);
    // Zum Schluss wird geblitted.
    glBlitFramebuffer(
            0, 0, ctx->winData->width, ctx->winData->height,
            0, halfHeight, halfWidth, ctx->winData->height,
            GL_COLOR_BUFFER_BIT, GL_LINEAR);

    // Es soll aus dem Emission-Attachment gelesen werden.
    glReadBuffer(data->fb.attachments[GBUFFER_COLORATTACH_EMISSION]);
    // Zum Schluss wird geblitted.
    glBlitFramebuffer(
            0, 0, ctx->winData->width, ctx->winData->height,
            halfWidth, halfHeight, ctx->winData->width, ctx->winData->height,
            GL_COLOR_BUFFER_BIT, GL_LINEAR);
}

/**
 * Lädt alle Shader, die im Rendering-Modul verwendet werden.
 * 
 * @param data Zugriff auf das Rendering Datenobjekt.
 */
static void rendering_loadShaders(RenderingData *data) {
    data->modelShader = shader_createVeCoEvFrShader(
            UTILS_CONST_RES("shader/model/model.vert"),
            UTILS_CONST_RES("shader/model/model.frag"),
            UTILS_CONST_RES("shader/model/model.tesc"),
            UTILS_CONST_RES("shader/model/model.tese"));
    data->skyboxShader = shader_createVeFrShader(
            UTILS_CONST_RES("shader/skybox/skybox.vert"),
            UTILS_CONST_RES("shader/skybox/skybox.frag"));
    data->dirLight = shader_createVeFrShader(
            UTILS_CONST_RES("shader/dirLight/dirLight.vert"),
            UTILS_CONST_RES("shader/dirLight/dirLight.frag"));
    data->pointLight = shader_createVeFrShader(
            UTILS_CONST_RES("shader/pointLight/pointLight.vert"),
            UTILS_CONST_RES("shader/pointLight/pointLight.frag"));
    data->postProcessing = shader_createVeFrShader(
            UTILS_CONST_RES("shader/postProcessing/postProcessing.vert"),
            UTILS_CONST_RES("shader/postProcessing/postProcessing.frag"));
    data->null = shader_createVeFrShader(
            UTILS_CONST_RES("shader/null/null.vert"),
            UTILS_CONST_RES("shader/null/null.frag"));
    data->threshhold = shader_createVeFrShader(
            UTILS_CONST_RES("shader/threshhold/threshhold.vert"),
            UTILS_CONST_RES("shader/threshhold/threshhold.frag"));
    data->blur = shader_createVeFrShader(
            UTILS_CONST_RES("shader/blur/blur.vert"),
            UTILS_CONST_RES("shader/blur/blur.frag"));
    data->dirShadow = shader_createVeFrShader(
            UTILS_CONST_RES("shader/dirShadow/dirShadow.vert"),
            UTILS_CONST_RES("shader/dirShadow/dirShadow.frag"));
    data->pointShadow = shader_createVeGeomFrShader(
            UTILS_CONST_RES("shader/pointShadow/pointShadow.vert"),
            UTILS_CONST_RES("shader/pointShadow/pointShadow.geom"),
            UTILS_CONST_RES("shader/pointShadow/pointShadow.frag"));
}

void rendering_initWidthHeight(ProgContext *ctx) {
    // Einmal zu Begin die Größe des Framebuffers bestimmen und setzen.
    // Bei Veränderungen wird das Callback aufgerufen.
    glfwGetFramebufferSize(
            ctx->window,
            &ctx->winData->width,
            &ctx->winData->height);
    glViewport(0, 0, ctx->winData->width, ctx->winData->height);

    // Für die GUI brauchen wir die echte Fenstergröße.
    // Auch sie wird über ein Callback aktualisiert.
    glfwGetWindowSize(
            ctx->window,
            &ctx->winData->realWidth,
            &ctx->winData->realHeight);
}

//////////////////////////// ÖFFENTLICHE FUNKTIONEN ////////////////////////////

void rendering_reRenderShaders(ProgContext *ctx) {
    Shader *tempModel = shader_createVeCoEvFrShader(
            UTILS_CONST_RES("shader/model/model.vert"),
            UTILS_CONST_RES("shader/model/model.frag"),
            UTILS_CONST_RES("shader/model/model.tesc"),
            UTILS_CONST_RES("shader/model/model.tese"));

    Shader *tempSkyBox = shader_createVeFrShader(
            UTILS_CONST_RES("shader/skybox/skybox.vert"),
            UTILS_CONST_RES("shader/skybox/skybox.frag"));

    Shader *tempDirLight = shader_createVeFrShader(
            UTILS_CONST_RES("shader/dirLight/dirLight.vert"),
            UTILS_CONST_RES("shader/dirLight/dirLight.frag"));

    Shader *tempPointLight = shader_createVeFrShader(
            UTILS_CONST_RES("shader/pointLight/pointLight.vert"),
            UTILS_CONST_RES("shader/pointLight/pointLight.frag"));

    Shader *tempPostProcess = shader_createVeFrShader(
            UTILS_CONST_RES("shader/postProcessing/postProcessing.vert"),
            UTILS_CONST_RES("shader/postProcessing/postProcessing.frag"));

    Shader *tempNull = shader_createVeFrShader(
            UTILS_CONST_RES("shader/null/null.vert"),
            UTILS_CONST_RES("shader/null/null.frag"));

    Shader *tempThreshhold = shader_createVeFrShader(
            UTILS_CONST_RES("shader/threshhold/threshhold.vert"),
            UTILS_CONST_RES("shader/threshhold/threshhold.frag"));

    Shader *tempBlur = shader_createVeFrShader(
            UTILS_CONST_RES("shader/blur/blur.vert"),
            UTILS_CONST_RES("shader/blur/blur.frag"));

    Shader *tempDirShadow = shader_createVeFrShader(
            UTILS_CONST_RES("shader/dirShadow/dirShadow.vert"),
            UTILS_CONST_RES("shader/dirShadow/dirShadow.frag"));

    Shader *tempPointShadow = shader_createVeGeomFrShader(
            UTILS_CONST_RES("shader/pointShadow/pointShadow.vert"),
            UTILS_CONST_RES("shader/pointShadow/pointShadow.geom"),
            UTILS_CONST_RES("shader/pointShadow/pointShadow.frag"));

    if (tempModel != NULL) {
        shader_deleteShader(ctx->rendering->modelShader);
        ctx->rendering->modelShader = tempModel;
    }

    if (tempSkyBox != NULL) {
        shader_deleteShader(ctx->rendering->skyboxShader);
        ctx->rendering->skyboxShader = tempSkyBox;
    }

    if (tempDirLight != NULL) {
        shader_deleteShader(ctx->rendering->dirLight);
        ctx->rendering->dirLight = tempDirLight;
    }

    if (tempPointLight != NULL) {
        shader_deleteShader(ctx->rendering->pointLight);
        ctx->rendering->pointLight = tempPointLight;
    }

    if (tempPostProcess != NULL) {
        shader_deleteShader(ctx->rendering->postProcessing);
        ctx->rendering->postProcessing = tempPostProcess;
    }

    if (tempNull != NULL) {
        shader_deleteShader(ctx->rendering->null);
        ctx->rendering->null = tempNull;
    }

    if (tempThreshhold != NULL) {
        shader_deleteShader(ctx->rendering->threshhold);
        ctx->rendering->threshhold = tempThreshhold;
    }

    if (tempBlur != NULL) {
        shader_deleteShader(ctx->rendering->blur);
        ctx->rendering->blur = tempBlur;
    }

    if (tempDirShadow != NULL) {
        shader_deleteShader(ctx->rendering->dirShadow);
        ctx->rendering->dirShadow = tempDirShadow;
    }

    if (tempPointShadow != NULL) {
        shader_deleteShader(ctx->rendering->pointShadow);
        ctx->rendering->pointShadow = tempPointShadow;
    }
}

void rendering_init(ProgContext *ctx) {
    ctx->rendering = malloc(sizeof(RenderingData));
    RenderingData *data = ctx->rendering;

    // Speicher bereinigen.
    memset(data, 0, sizeof(RenderingData));

    // OpenGL Flags setzen.
    glCullFace(GL_BACK); // setzten Faceculling auf Back-Face
    glFrontFace(GL_CCW); // front Faces sind gegen den Uhrzeigersinn

    // Vorm initialisieren des Framebuffers muss ctx->winData->width
    // und ctx->winData->height gesetzt werden
    rendering_initWidthHeight(ctx);

    // Alle Shader laden.
    rendering_loadShaders(data);

    //SkyBox initialisieren
    skybox_initSkyBox(&data->skyBox);

    // Framebuffer initialisieren.
    framebuffer_initFramebuffer(&data->fb,
                                ctx->winData->width,
                                ctx->winData->height);
    framebuffer_initPingPongBuffer(&data->pingPong,
                                   ctx->winData->width,
                                   ctx->winData->height);
    framebuffer_initDepthFBO(&data->depthFBO);
    framebuffer_initDepthCubeFBO(&data->depthCubeFBO, g_pointLightProj, -1);

    //Erstellt ein ViewPort fuellendes Quad
    data->displayQuad = mesh_createQuad();

    //Laedt eine DepthMap
    g_depthMap = texture_loadTexture(UTILS_CONST_RES("textures/depthMap.dds"), GL_REPEAT, GL_FALSE);
}

GLboolean first = true;

void rendering_draw(ProgContext *ctx) {
    RenderingData *data = ctx->rendering;
    InputData *input = ctx->input;
    //Framebuffer anpassen, wenn Fenster skaliert wird
    framebuffer_resizeFramebuffer(&data->pingPong,
                                  &data->fb,
                                  ctx->winData->width,
                                  ctx->winData->height);

    // Bildschirm leeren.
    glClearColor(
            input->rendering.clearColor[0],
            input->rendering.clearColor[1],
            input->rendering.clearColor[2],
            input->rendering.clearColor[3]);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // Überprüfen, ob der Wireframe Modus verwendet werden soll.
    if (input->showWireframe) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glDisable(GL_CULL_FACE); // deaktivierung des Face Cullings
    } else {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glEnable(GL_CULL_FACE); // aktivierung des Face Cullings
    }

    // Zuerst die Projection Matrix aufsetzen.
    mat4 projectionMatrix;
    float aspect = (float) ctx->winData->width / (float) ctx->winData->height;
    float zoom = camera_getZoom(input->mainCamera);
    glm_perspective(glm_rad(zoom), aspect, 0.1f, 200.0f, projectionMatrix);

    // Dann die View-Matrix bestimmen.
    mat4 viewMatrix;
    camera_getViewMatrix(input->mainCamera, viewMatrix);

    mat4 viewProjMatrix;
    glm_mat4_mul(projectionMatrix, viewMatrix, viewProjMatrix);

    // Nur wenn die Szene richtig geladen wurde rendern
    if (input->rendering.userScene) {
        vec3 xAxis = {1.0, 0.0, 0.0};
        vec3 yAxis = {0.0, 1.0, 0.0};
        vec3 zAxis = {0.0, 0.0, 1.0};

        // objectMatrix festlegen.
        mat4 objectMatrix;
        glm_mat4_identity(objectMatrix);
        glm_rotate(objectMatrix, input->rendering.modelRotation[0], xAxis);
        glm_rotate(objectMatrix, input->rendering.modelRotation[1], yAxis);
        glm_rotate(objectMatrix, input->rendering.modelRotation[2], zAxis);

        //Modell skalieren
        glm_scale_uni(objectMatrix, input->rendering.scale);

        // Das Nutzermodell nur dann Rendern, wenn es existiert.
        if (input->rendering.userScene->model) {
            //Daten fuer Displacement an Shader uebergeben
            glActiveTexture(GL_TEXTURE5);
            glBindTexture(GL_TEXTURE_2D, g_depthMap);

            if ((data->modelShader) && (data->null) && (data->pointLight) && (data->dirLight)) {
                /*------------------------- Geometry-PASS -------------------------*/
                deferredShader_doGeometryPass(ctx, &objectMatrix, &projectionMatrix, &viewMatrix);

                Scene *currScene = input->rendering.userScene;
                if (input->lighting.pointLightActive) {

                    //Deferred Shading fuer alle Punktlichter in der Szene durchfuehren
                    for (int i = 0; i < currScene->countPointLights; i++) {
                        //Aktuelle Punktlichtquelle
                        PointLight *currPtLight = currScene->pointLights[i];
                        //MVP-Matrix des aktuellen Light-Volumes
                        mat4 lightMVP;

                        //Schatten der Punktlichtquellen
                        if (input->shadows.createPointLightShadows) {
                            shadowMapping_createPointLightTransforms(currPtLight, g_pointLightProj,
                                                                     g_pointLightTransforms);
                            if (first) {
                                framebuffer_initDepthCubeFBO(&data->depthCubeFBO, g_pointLightProj,
                                                             currScene->countPointLights);
                                shadowMapping_renderPointLightShadowMap(ctx, &objectMatrix, g_pointLightTransforms,
                                                                        currPtLight, i);
                                first = false;
                            }
                        }
                        /*------------------------- Stencil-PASS -------------------------*/
                        deferredShader_doStencilPass(ctx, currPtLight, projectionMatrix, viewMatrix, &lightMVP);

                        /*------------------------- Point-PASS -------------------------*/
                        deferredShader_activateTexturesLighting(data, i);
                        deferredShader_doPointPass(ctx, &lightMVP, currPtLight);
                    }
                    //Stencil-Testing deaktivieren
                    glDisable(GL_STENCIL_TEST);
                }

                /*------------------------- Directional Light-PASS -------------------------*/
                if (input->lighting.dirLightActive) {
                    //Schatten der Richtungslichtquelle
                    if (input->shadows.createDirShadows || input->shadows.realtimeDirShadows) {
                        shadowMapping_createDirLightSpaceMat(g_lightSpaceMat, input->lighting.dirLight.direction);
                        shadowMapping_renderDirLightShadowMap(ctx, &objectMatrix, &g_lightSpaceMat);
                    }

                    deferredShader_activateTexturesLighting(data, 0);
                    deferredShader_doDirLightPass(ctx, &g_lightSpaceMat);
                }
            }

            /* ---------------------- Threshhold - SHADER ---------------------------------- */
            if (data->threshhold) {
                deferredShader_activateTexturesThreshhold(data);
                postProcessing_extractBrightColors(ctx);
            }

            /* ---------------------- Blur - SHADER ---------------------------------- */

            if (data->blur) {
                postProcessing_blur(ctx);
            }

            /* ---------------------- Post-Process - SHADER ---------------------------------- */
            if (data->postProcessing) {
                deferredShader_activateTexturesFinalRender(data);
                postProcessing_finalRender(ctx);
            }

            // Tiefentest nach der 3D Szene wieder deaktivieren.
            glDisable(GL_DEPTH_TEST);

            // Wireframe am Ende wieder deaktivieren.
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

            if (input->showGBuffer) {
                //Debug Modus rendern
                rendering_drawDebugMode(ctx);
            } else {
                //Finales Bild mit SkyBox rendern
                rendering_drawFinalTex(ctx);
                skybox_renderSkyBox(ctx, &projectionMatrix, &viewMatrix);
            }
        }
    }
    // Framebuffer zurücksetzen.
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void rendering_cleanup(ProgContext *ctx) {
    RenderingData *data = ctx->rendering;

    // Zum Schluss müssen noch die belegten Ressourcen freigegeben werden.
    shader_deleteShader(data->modelShader);
    shader_deleteShader(data->skyboxShader);
    shader_deleteShader(data->dirLight);
    shader_deleteShader(data->pointLight);
    shader_deleteShader(data->null);
    shader_deleteShader(data->threshhold);
    shader_deleteShader(data->postProcessing);
    shader_deleteShader(data->blur);
    shader_deleteShader(data->dirShadow);
    shader_deleteShader(data->pointShadow);
    framebuffer_deleteFrameBuffer(&data->fb);
    framebuffer_deletePingPongBuffer(&data->pingPong);
    framebuffer_deleteDepthFrameBuffer(&data->depthFBO);
    framebuffer_deleteDepthCubeFrameBuffer(&data->depthCubeFBO);
    skybox_deleteSkyBox(&data->skyBox);
    free(ctx->rendering);
}