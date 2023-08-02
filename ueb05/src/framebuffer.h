#ifndef FRAMBUFFER_H
#define FRAMBUFFER_H
#include "common.h"

#define SHADOW_WIDTH 1024
#define SHADOW_HEIGHT 1024
// Aufzählungstyp für die unterschiedlichen Color Attachments des GBuffers.
typedef enum {
    GBUFFER_COLORATTACH_POSITION,
    GBUFFER_COLORATTACH_NORMAL,
    GBUFFER_COLORATTACH_ALBEDOSPEC,
    GBUFFER_COLORATTACH_EMISSION,
    GBUFFER_COLORATTACH_FINAL,
    GBUFFER_COLORATTACH_BRIGHTNESS,
    GBUFFER_COLORATTACH_RESULT,

    GBUFFER_NUM_COLORATTACH
} GBUFFER_TEXTURE_TYPE;

// Datenrepräsentation des hier verwendeten Framebuffers.
typedef struct Framebuffer
{
    GLuint fbo;
    GLuint textures[GBUFFER_NUM_COLORATTACH];
    GLuint depthRbo;
    GLuint attachments[GBUFFER_NUM_COLORATTACH];
} Framebuffer;

// Datenrepräsentation des hier verwendeten PingPong-Framebuffers.
typedef struct PingPong
{
    GLuint fbo;
    GLuint buffer[2];
    GLuint depthRbo;
} PingPong;

typedef struct depthFBO
{
    GLuint fbo;
    GLuint depthMap;
    GLuint depthCubeMap;
} depthFBO;

typedef struct depthCubeFBO
{
    GLuint fbo;
    GLuint *depthCubeMap;
} depthCubeFBO;


void framebuffer_initFramebuffer(Framebuffer *fb, int width, int height);
void framebuffer_deleteFrameBuffer(Framebuffer *fb);

void framebuffer_initPingPongBuffer(PingPong *fbo, int width, int height);
void framebuffer_deletePingPongBuffer(PingPong *fb);

void framebuffer_resizeFramebuffer(PingPong *PPfbo, Framebuffer *fbo, int width, int height);

void framebuffer_initDepthFBO(depthFBO *fb);
void framebuffer_initDepthCubeFBO(depthCubeFBO *fb, mat4 pointLightProj);

void framebuffer_deleteDepthFrameBuffer(depthFBO *fb);
void framebuffer_deleteDepthCubeFrameBuffer(depthCubeFBO *fb);
#endif // FRAMEBUFFER_H