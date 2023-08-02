#include "shadowMapping.h"
#include "rendering.h"
#include "input.h"

/**
 * generiert die LightSpaceMatrix des Richtungslichtes
 * 
 * @param[in/out] matrix
 * @param[in] lightDir Richtungslicht
 */ 
void shadowMapping_createDirLightSpaceMat(mat4 lightSpaceMat, vec3 lightDir)
{
    mat4 proj;
    //Projektionsmatrix
    //Dimensionen für diee SchattenBox
    glm_ortho(-12.0f, 12.0f, -12.0f, 12.0f, -25.0f, 25.0f, proj);
    vec3 lightDirCpy;
    glm_vec3_copy(lightDir, lightDirCpy);
    glm_vec3_norm(lightDirCpy);
    
    vec3 center = {0.0f, 0.0f, 0.0f};
    vec3 up = {0, 1, 0};
    //Licht direkt über der Szene ->
    //Upvektor ändern, weil sonst keine Schatten unter Objekten
    //generiert werden
    if(lightDir[0] == 0 && lightDir[1] > 0 && lightDir[2] == 0){
        up[1] = 0;
        up[2] = 1;
    }

    mat4 viewMat;
    glm_lookat(lightDirCpy, center, up, viewMat);

    glm_mat4_mul(proj, viewMat, lightSpaceMat);
}

/**
 * Rendert die Schatten des Richtungslichtes
 *
 * @param ctx Programmkontext
 * @param objectMatrix Objekt-Matrix der Szene
 * @param lightSpaceMat aktuelle LightSpaceMatrix
 */ 
void shadowMapping_renderDirLightShadowMap(ProgContext *ctx, mat4 *objectMatrix, mat4 *lightSpaceMat)
{
    RenderingData *data = ctx->rendering;
    InputData *input = ctx->input;

    //Directional Shadow Shader aktivieren
    shader_useShader(data->dirShadow);
    //Uniforms übergeben
    shader_setMat4(data->dirShadow, "lightSpaceMat", lightSpaceMat);
    shader_setMat4(data->dirShadow, "modelMat", objectMatrix);
    //Viewport auf Texturdimensionen der Shadowmap setzen
    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    //Framebuffer aktiveren
    glBindFramebuffer(GL_FRAMEBUFFER, data->depthFBO.fbo);

    //Depth Test aktievieren
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glClear(GL_DEPTH_BUFFER_BIT);

    //Tiefeninformationen aus Sicht des Lichts in Depth-Textur schreiben
    model_drawModelTris(input->rendering.userScene->model, data->dirShadow);
    printf("Loaded Directional Shadow-Map\n");
    //ViewPort zurücksetzen
    glViewport(0, 0, ctx->winData->width, ctx->winData->height);
    //ShadowMap nur einmal beim Laden der Szene erstellen oder auf Knofpdruck
    input->shadows.createDirShadows = false;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
}

void shadowMapping_renderPointLightShadowMap(ProgContext *ctx, mat4 *objectMatrix, mat4 pointLightTransforms[6], PointLight *currPointLight, int ptLightIndex)
{
    RenderingData *data = ctx->rendering;
    InputData *input = ctx->input;

    //Directional Shadow Shader aktivieren
    shader_useShader(data->pointShadow);
    //Uniforms übergeben
    shader_setMat4(data->pointShadow, "model", objectMatrix);
    for (int i = 0; i < 6; i++)
    {
        char buffer[64];
        sprintf(buffer, "pointLightTransforms[%i]", i);
        shader_setMat4(data->pointShadow, buffer, &pointLightTransforms[i]);
    }
    shader_setFloat(data->pointShadow, "farPlane", 25.0f);
    shader_setVec3(data->pointShadow, "lightPos", &currPointLight->position);
    //Viewport auf Texturdimensionen der Shadowmap setzen
    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    //Framebuffer aktiveren
    glBindFramebuffer(GL_FRAMEBUFFER, data->depthCubeFBO.fbo);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, data->depthCubeFBO.cubeMaps[ptLightIndex], 0);

    glDrawBuffer(GL_NONE); // No color buffer is drawn to.
    glReadBuffer(GL_NONE);
    //Depth Test aktivieren
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glClear(GL_DEPTH_BUFFER_BIT);

    //Tiefeninformationen aus Sicht des Lichts in Depth-Textur schreiben
    model_drawModelTris(input->rendering.userScene->model, data->pointShadow);
    //ViewPort zurücksetzen
    glViewport(0, 0, ctx->winData->width, ctx->winData->height);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
}

/**
 * LightSpace Matrizen für alle 6 Seiten des Cubes bestimmen
 * 
 * @param currLight aktuelles Punktlicht
 * @param g_pointLightProj Projektionsmatrix für die Seiten
 */ 
void shadowMapping_createPointLightTransforms(PointLight *currLight, mat4 g_pointLightProj, mat4 pointLightTransforms[6])
{
    //Alle richtungen
    vec3 x = {1.0, 0.0, 0.0};
    vec3 y = {0.0, 1.0, 0.0};
    vec3 z = {0.0, 0.0, 1.0};
    vec3 nx = {-1.0, 0.0, 0.0};
    vec3 ny = {0.0, -1.0, 0.0};
    vec3 nz = {0.0, 0.0, -1.0};

    //Lightposition
    vec3 lightPos;
    glm_vec3_copy(currLight->position, lightPos);

    mat4 view;
    vec3 center;

    glm_vec3_add(lightPos, x, center);
    glm_lookat(lightPos, center, ny, view);
    glm_mat4_mul(g_pointLightProj, view, pointLightTransforms[0]);
    glm_vec3_zero(center);
    glm_mat4_zero(view);

    glm_vec3_add(lightPos, nx, center);
    glm_lookat(lightPos, center, ny, view);
    glm_mat4_mul(g_pointLightProj, view, pointLightTransforms[1]);
    glm_vec3_zero(center);
    glm_mat4_zero(view);

    glm_vec3_add(lightPos, y, center);
    glm_lookat(lightPos, center, z, view);
    glm_mat4_mul(g_pointLightProj, view, pointLightTransforms[2]);
    glm_vec3_zero(center);
    glm_mat4_zero(view);

    glm_vec3_add(lightPos, ny, center);
    glm_lookat(lightPos, center, nz, view);
    glm_mat4_mul(g_pointLightProj, view, pointLightTransforms[3]);
    glm_vec3_zero(center);
    glm_mat4_zero(view);

    glm_vec3_add(lightPos, z, center);
    glm_lookat(lightPos, center, ny, view);
    glm_mat4_mul(g_pointLightProj, view, pointLightTransforms[4]);
    glm_vec3_zero(center);
    glm_mat4_zero(view);

    glm_vec3_add(lightPos, nz, center);
    glm_lookat(lightPos, center, ny, view);
    glm_mat4_mul(g_pointLightProj, view, pointLightTransforms[5]);
    glm_vec3_zero(center);
    glm_mat4_zero(view);
}