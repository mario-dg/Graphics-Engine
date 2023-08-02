#ifndef FRAMBUFFER_H
#define FRAMBUFFER_H
#include "common.h"

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
    GLuint fbo[2];
    GLuint buffer[2];
} PingPong;


void framebuffer_initFramebuffer(Framebuffer *fb, int width, int height);
void framebuffer_deleteFrameBuffer(Framebuffer *fb);

void framebuffer_initPingPongBuffer(PingPong *fbo, int width, int height);
void framebuffer_deletePingPongBuffer(PingPong *fb);

void framebuffer_resizeFramebuffer(PingPong *PPfbo, Framebuffer *fbo, int width, int height);

#endif // FRAMBUFFER_H