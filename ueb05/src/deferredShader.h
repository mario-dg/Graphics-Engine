
#ifndef DEFERREDSHADER_H
#define DEFERREDSHADER_H

#include "common.h"
#include "light.h"
#include "rendering.h"

void deferredShader_doGeometryPass(ProgContext *ctx, mat4 *modelMatrix, mat4 *projectionMatrix, mat4 *viewMatrix);

void deferredShader_doStencilPass(ProgContext *ctx, PointLight *currPtLight, mat4 projectionMatrix, mat4 viewMatrix, mat4 *lightMVP);

void deferredShader_doPointPass(ProgContext *ctx, mat4 *lightMVP, PointLight* currPointLight);

void deferredShader_doDirLightPass(ProgContext *ctx, mat4 *lightSpaceMat);

void deferredShader_activateTexturesLighting(RenderingData *data);

void deferredShader_activateTexturesThreshhold(RenderingData *data);

void deferredShader_activateTexturesFinalRender(RenderingData *data);
#endif // DEFERREDSHADER_H
