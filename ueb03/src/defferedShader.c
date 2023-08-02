#include "deferredShader.h"
#include "framebuffer.h"
#include "shader.h"
#include "rendering.h"
#include "input.h"
#include "scene.h"

/**
 * Fuehrt den Geometry-Pass im deferred Shading aus
 * 
 * @param ctx Programmkontext
 * @param modelMatrix ModelMatrix des zu zeichnenden Models
 * @param projectionMatrix ProjektionsMatrix der Szene
 * @param viewMatrix ViewMatrix der Szene
 * 
 */
void deferredShader_doGeometryPass(ProgContext *ctx, mat4 *modelMatrix, mat4 *projectionMatrix, mat4 *viewMatrix)
{
    // ---------------------- MODEL - SHADER ---------------------------------- //
    RenderingData *data = ctx->rendering;
    InputData *input = ctx->input;

    // Tiefentest aktivieren.
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);

    //GBuffer leeren
    glBindFramebuffer(GL_FRAMEBUFFER, data->fb.fbo);
    glDrawBuffers(GBUFFER_NUM_COLORATTACH, data->fb.attachments);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //Position, Normal, AlbedoSpec und Emissions-Attachment beschreiben
    glDrawBuffers(4, data->fb.attachments);

    // Shader vorbereiten.
    shader_useShader(data->modelShader);

    //Matrizen an Vertex-Shader schicken
    shader_setMat4(data->modelShader, "projectionMatrix", projectionMatrix);
    shader_setMat4(data->modelShader, "modelMatrix", modelMatrix);
    shader_setMat4(data->modelShader, "viewMatrix", viewMatrix);

    //Daten fuer die Tessellation an Shader uebergeben
    shader_setBool(data->modelShader, "useTessellation", input->rendering.useTessellation);
    shader_setFloat(data->modelShader, "innerTessellation", input->rendering.innerTessellation);
    shader_setFloat(data->modelShader, "outerTessellation", input->rendering.outerTessellation);
    shader_setBool(data->modelShader, "useDistanceTessellation", input->rendering.useDistanceTessellation);
    shader_setFloat(data->modelShader, "tessellationAmount", input->rendering.tessellationAmount);
    shader_setVec3(data->modelShader, "camPos", camera_getCameraPos(input->mainCamera));

    //Displacement Daten an Shader schicken
    shader_setInt(data->modelShader, "depthMap", 5);
    shader_setFloat(data->modelShader, "displacementFactor", input->rendering.displacementFactor);
    shader_setBool(data->modelShader, "useDisplacement", input->rendering.useDisplacement);

    //Normalmapping
    shader_setBool(data->modelShader, "useNormalMapping", input->rendering.useNormalMapping);

    //Parallaxmapping Daten an Shader schicken
    shader_setBool(data->modelShader, "useParallaxMapping", input->rendering.useParallax);
    shader_setFloat(data->modelShader, "heightScale", input->rendering.heightScale);

    // Modell zeichnen
    model_drawModel(input->rendering.userScene->model, data->modelShader);

    //Schreiben auf Depth-Buffer deaktivieren
    glDepthMask(GL_FALSE);
}

/**
 * Berechnet den Radius vom Ligh-Volume eines Punktlichtes
 * 
 * @param Punktlicht
 */
static float rendering_calcLightVolumeRadius(PointLight *ptLight)
{
    vec3 color = {ptLight->diffuse[0], ptLight->diffuse[1], ptLight->diffuse[2]};
    float lightMax = fmaxf(fmaxf(color[0], color[1]), color[2]);
    float linear = ptLight->linear;
    float quadratic = ptLight->quadratic;
    float constant = ptLight->constant;
    return ((-linear + sqrtf(linear * linear - 4 * quadratic * (constant - (255.0f / 5.0f) * (float)lightMax))) / (2.0f * quadratic));
}

/**
 * Fuehrt den Stencil-Pass des Deferred Shading durch
 * 
 * @param ctx Programmkontext
 * @param currPtLight aktuelles Punktlicht
 * @param projectionMatrix ProjektionsMatrix der Szene
 * @param viewMatrix ViewMatrix der Szene
 * @param lightMVP MVP-Matrix des aktuellen Punktlichtes
 * 
 */
void deferredShader_doStencilPass(ProgContext *ctx, PointLight *currPtLight, mat4 projectionMatrix, mat4 viewMatrix, mat4 *lightMVP)
{
    // ---------------------- NULL - SHADER ---------------------------------- //
    RenderingData *data = ctx->rendering;
    InputData *input = ctx->input;

    //GBuffer binden
    glBindFramebuffer(GL_FRAMEBUFFER, data->fb.fbo);
    //Keinen COLOR-Buffer binden
    glDrawBuffer(GL_NONE);
    //Depth-Test deaktivieren
    glEnable(GL_DEPTH_TEST);
    //NULL-Shader aktivieren
    shader_useShader(data->null);
    //Stencil-Testing initialisieren
    glEnable(GL_STENCIL_TEST);
    glClear(GL_STENCIL_BUFFER_BIT);
    glStencilFunc(GL_ALWAYS, 0, 0);
    glStencilOpSeparate(GL_BACK, GL_KEEP, GL_INCR_WRAP, GL_KEEP);
    glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_DECR_WRAP, GL_KEEP);

    //Face-Culling deaktivieren
    glDisable(GL_CULL_FACE);

    //Light-Volume erstellen, skalieren und translatieren
    float radius = rendering_calcLightVolumeRadius(currPtLight);
    mat4 modelMat;
    glm_mat4_identity(modelMat);
    glm_translate(modelMat, currPtLight->position);
    glm_scale_uni(modelMat, radius);
    mat4 temp;
    glm_mat4_mul(projectionMatrix, viewMatrix, temp);
    glm_mat4_mul(temp, modelMat, *lightMVP);
    shader_setMat4(data->null, "lightMVP", lightMVP);
    //Sphere mit Light-Volume Radius rendern
    model_drawModelTris(input->rendering.lightVolSphere, data->null);
}

/**
 * Fuehrt den Point-Pass des Deferred Shading durch
 * 
 * @param ctx Programmkontext
 * @param lightMVP MVP-Matrix des aktuellen Punktlichtes
 * @param currPointLight aktuelles Punktlicht
 */
void deferredShader_doPointPass(ProgContext *ctx, mat4 *lightMVP, PointLight *currPointLight)
{
    // ---------------------- PointLight - SHADER ---------------------------------- //
    RenderingData *data = ctx->rendering;
    InputData *input = ctx->input;

    //Gbuffer binden
    glBindFramebuffer(GL_FRAMEBUFFER, data->fb.fbo);
    //Ergebnis in das Final-Attachment schreiben
    glDrawBuffer(data->fb.attachments[GBUFFER_COLORATTACH_FINAL]);
    //Point-Light-Shader aktivieren
    shader_useShader(data->pointLight);
    //Stencil Function umsetzen
    glStencilMask(0xFF);
    glStencilFunc(GL_NOTEQUAL, 0, 0xFF);
    //Depth-Test deaktivieren
    glDisable(GL_DEPTH_TEST);
    //Blending aktivieren mit 1 zu 1 Addition
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_ONE, GL_ONE);
    //Front-Face Culling aktivieren
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);

    //MVP-Matrix senden
    shader_setMat4(data->pointLight, "lightMVP", lightMVP);

    //Texturen binden und an shader schicken
    shader_setInt(data->pointLight, "gPosition", 1);
    shader_setInt(data->pointLight, "gNormal", 2);
    shader_setInt(data->pointLight, "gAlbedoSpec", 3);
    //Kamera-Position und Punktlicht senden
    shader_setVec3(data->pointLight, "camPos", camera_getCameraPos(ctx->input->mainCamera));
    light_activatePointLight(currPointLight, data->pointLight);
    shader_setInt(data->pointLight, "viewPortWidth", ctx->winData->width);
    shader_setInt(data->pointLight, "viewPortHeight", ctx->winData->height);
    //LightVolume rendern
    model_drawModelTris(input->rendering.lightVolSphere, data->pointLight);

    //Back-Face Culling aktivieren
    glEnable(GL_CULL_FACE);
    //Back-Face Culling aktivieren
    glCullFace(GL_BACK);
    //Blending deaktivieren
    glDisable(GL_BLEND);
}

/**
 * Fuehrt den Directional Light Pass des Deferred Shading durch
 * 
 * @param ctx Programmkontext
 */
void deferredShader_doDirLightPass(ProgContext *ctx)
{
    // ---------------------- DirLight - SHADER ---------------------------------- //
    RenderingData *data = ctx->rendering;
    InputData *input = ctx->input;

    //GBuffer binden
    glBindFramebuffer(GL_FRAMEBUFFER, data->fb.fbo);
    //Ergebnis in das Final-Attachment schreiben
    glDrawBuffer(data->fb.attachments[GBUFFER_COLORATTACH_FINAL]);

    //Blending aktivieren mit 1 zu 1 Addition
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    //Directional Light shader aktivieren
    shader_useShader(data->dirLight);
    //Texturen binden und an shader schicken
    shader_setInt(data->dirLight, "gPosition", 1);
    shader_setInt(data->dirLight, "gNormal", 2);
    shader_setInt(data->dirLight, "gAlbedoSpec", 3);
    //Kamera-Position senden
    shader_setVec3(data->dirLight, "camPos", camera_getCameraPos(ctx->input->mainCamera));
    light_activateDirLight(&input->rendering.dirLight, data->dirLight);
    //Viewport füllendes Quad rendern
    mesh_drawMeshTris(data->displayQuad, data->dirLight);
    //Blending deaktivieren
    glDisable(GL_BLEND);
}

/**
 * Aktiviert die benoetigten Texturen fuer den Point-Pass und DirLight-Pass
 * 
 * @param data Rendering Data
 */
void deferredShader_activateTexturesLighting(RenderingData *data)
{
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, data->fb.textures[GBUFFER_COLORATTACH_POSITION]);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, data->fb.textures[GBUFFER_COLORATTACH_NORMAL]);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, data->fb.textures[GBUFFER_COLORATTACH_ALBEDOSPEC]);
}

/**
 * Aktiviert die benoetigten Texturen fuer die Extrahierung der hellen Bereiche
 * 
 * @param data Rendering Data
 */
void deferredShader_activateTexturesThreshhold(RenderingData *data)
{
    glActiveTexture(GL_TEXTURE10);
    glBindTexture(GL_TEXTURE_2D, data->fb.textures[GBUFFER_COLORATTACH_FINAL]);
    glActiveTexture(GL_TEXTURE11);
    glBindTexture(GL_TEXTURE_2D, data->fb.textures[GBUFFER_COLORATTACH_EMISSION]);
}

/**
 * Aktiviert die benoetigten Texturen fuer den finalen Render Pass
 * 
 * @param data Rendering Data
 */
void deferredShader_activateTexturesFinalRender(RenderingData *data, int horizontal)
{
    glActiveTexture(GL_TEXTURE15);
    glBindTexture(GL_TEXTURE_2D, data->fb.textures[GBUFFER_COLORATTACH_FINAL]);
    glActiveTexture(GL_TEXTURE16);
    glBindTexture(GL_TEXTURE_2D, data->pingPong.buffer[!horizontal]);
}