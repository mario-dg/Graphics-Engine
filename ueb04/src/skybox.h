
#ifndef SKYBOX_H
#define SKYBOX_H
#include "common.h"
#include "utils.h"

typedef struct SkyBox {
    GLuint vao;
    GLuint vbo;
    GLuint texCube;
} SkyBox;


void skybox_initSkyBox(SkyBox *skybox);
void skybox_renderSkyBox(ProgContext *ctx, mat4 *projectionMatrix, mat4 *viewMatrix);
void skybox_deleteSkyBox(SkyBox *skybox);
#endif //SKYBOX_H