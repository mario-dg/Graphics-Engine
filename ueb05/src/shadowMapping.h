#ifndef SHADOWMAPPING_H
#define SHADOWMAPPING_H

#include "common.h"
#include "scene.h"

void shadowMapping_createDirLightSpaceMat(mat4 lightSpaceMat, vec3 lightDir);
void shadowMapping_renderDirLightShadowMap(ProgContext* ctx, mat4 *objectMatrix, mat4 *lightSpaceMat);
void shadowMapping_createPointLightTransforms(PointLight *currLight, mat4 g_pointLightProj, mat4 pointLightTransforms[6]);
void shadowMapping_renderPointLightShadowMap(ProgContext *ctx, mat4 *objectMatrix, mat4 pointLightTransforms[6], PointLight *currPointLight);
#endif //SHADOWMAPPING_H