#include "framebuffer.h"

/**
 * Erstellt ein Framebuffer Color-Attachment und bindet es an den aktuellen Framebuffer
 * 
 * @param attachment zu erstellendes Attachment
 * @param isAlbedo Angabe zur Textur
 * @param width Breite der Textur
 * @param height Breite der Textur
 * @param offset GL_COLOR_ATTACHMENTX
 */
static void
framebuffer_createColorAttachment(GLuint *attachment, bool isAlbedo, GLint width, GLint height, GLenum offset) {
    // Dann muss das Attachment angelegt werden.
    glBindTexture(GL_TEXTURE_2D, *attachment);

    // Wir wollen keine Textur laden sondern nur Speicher allozieren.
    // Deshalb setzen wir den letzten Parameter auf NULL.
    // Nur Albedo-Texturen speichern auch einen Alpha-Wert
    glTexImage2D(
            GL_TEXTURE_2D,
            0,
            isAlbedo ? GL_RGBA16F : GL_RGB16F,
            width, height,
            0,
            isAlbedo ? GL_RGBA : GL_RGB,
            GL_FLOAT,
            NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(
            GL_FRAMEBUFFER,
            offset,
            GL_TEXTURE_2D, *attachment, 0);
}

/**
 * Initialisiert den Framebuffer für die Lichtberechnungen und das PostProcessing.
 * 
 * 
 * @param fb der Framebuffer der initialisiert werden soll.
 * @param width die Breite des Framebuffers
 * @param height die Höhe des Framebuffers
 */
void framebuffer_initFramebuffer(Framebuffer *fb, int width, int height) {
    // Erst das FBO anlegen.
    glGenFramebuffers(1, &fb->fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fb->fbo);

    //Texturen aanlegen und binden
    glGenTextures(GBUFFER_NUM_COLORATTACH, fb->textures);
    GLuint i = 0;
    for (i = 0; i < GBUFFER_NUM_COLORATTACH; i++) {
        bool isAlbedo = i == GBUFFER_COLORATTACH_ALBEDOSPEC;
        framebuffer_createColorAttachment(&fb->textures[GBUFFER_COLORATTACH_POSITION + i], isAlbedo, width, height,
                                          GL_COLOR_ATTACHMENT0 + i);
        fb->attachments[i] = GL_COLOR_ATTACHMENT0 + i;
    }

    // Depth-Renderbuffer-Object
    ////////////////////////////

    glGenRenderbuffers(1, &fb->depthRbo);
    glBindRenderbuffer(GL_RENDERBUFFER, fb->depthRbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);

    // Das RBO an den Framebuffer binden.
    glFramebufferRenderbuffer(
            GL_FRAMEBUFFER,
            GL_DEPTH_STENCIL_ATTACHMENT,
            GL_RENDERBUFFER, fb->depthRbo);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        fprintf(stderr, "Error: Framebuffer incomplete!\n");
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

/**
 * Initalisiert einen Ping-Pong Framebuffer für den Blur-PostFX
 * 
 * @param fbo zu initialisierender Framebuffer
 * @param width die Breite des Framebuffers
 * @param height die Höhe des Framebuffers
 */
void framebuffer_initPingPongBuffer(PingPong *fbo, int width, int height) {
    glGenFramebuffers(1, &fbo->fbo);
    glGenTextures(2, fbo->buffer);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo->fbo);
    framebuffer_createColorAttachment(&fbo->buffer[0], true, width, height, GL_COLOR_ATTACHMENT0);
    framebuffer_createColorAttachment(&fbo->buffer[1], true, width, height, GL_COLOR_ATTACHMENT1);

    glGenRenderbuffers(1, &fbo->depthRbo);
    glBindRenderbuffer(GL_RENDERBUFFER, fbo->depthRbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, width, height);

    // Das RBO an den Framebuffer binden.
    glFramebufferRenderbuffer(
            GL_FRAMEBUFFER,
            GL_DEPTH_ATTACHMENT,
            GL_RENDERBUFFER, fbo->depthRbo);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        fprintf(stderr, "Error: Ping-Pong-Framebuffer incomplete!\n");
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void framebuffer_initDepthCubeFBO(depthCubeFBO *fb, mat4 pointLightProj, int pointLightCount) {
    if (pointLightCount < 0) {
        glGenFramebuffers(1, &fb->fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, fb->fbo);

        //Depth-Cube Map fuer Point Lights
        glGenTextures(1, &fb->depthCubeMap);
        glBindTexture(GL_TEXTURE_CUBE_MAP, fb->depthCubeMap);
        for (unsigned int i = 0; i < 6; ++i) {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT16,
                         SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        }

        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

        glBindFramebuffer(GL_FRAMEBUFFER, fb->fbo);
        glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, fb->depthCubeMap, 0);

        glDrawBuffer(GL_NONE); // No color buffer is drawn to.
        glReadBuffer(GL_NONE);
        //Projektionsmatrix ist fuer alle 6 Seiten gleich
        //Einmal erstellen
        float aspect = (float) SHADOW_WIDTH / (float) SHADOW_HEIGHT;
        float near = 1.0f;
        float far = 25.0f;
        glm_perspective(glm_rad(90.0f), aspect, near, far, pointLightProj);
    } else {
        glBindFramebuffer(GL_FRAMEBUFFER, fb->fbo);
        fb->cubeMaps = (GLuint*) calloc(pointLightCount, sizeof(GLuint));
        //Depth-Cube Map fuer Point Lights
        glGenTextures(pointLightCount, fb->cubeMaps);
        for (int k = 0; k < pointLightCount; ++k) {
            glBindTexture(GL_TEXTURE_CUBE_MAP, fb->cubeMaps[k]);
            for (unsigned int i = 0; i < 6; ++i) {
                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT16,
                             SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
            }
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        }

    }
    // Always check that our framebuffer is ok
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        fprintf(stderr, "Error: Depth-CubeMap-Framebuffer incomplete!\n");
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void framebuffer_initDepthFBO(depthFBO *fb) {
    // The framebuffer, which regroups 0, 1, or more textures, and 0 or 1 depth buffer.
    glGenFramebuffers(1, &fb->fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fb->fbo);

    // Depth texture. Slower than a depth buffer, but you can sample it later in your shader
    glGenTextures(1, &fb->depthMap);
    glBindTexture(GL_TEXTURE_2D, fb->depthMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT,
                 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, fb->depthMap, 0);

    glDrawBuffer(GL_NONE); // No color buffer is drawn to.
    glReadBuffer(GL_NONE);

    // Always check that our framebuffer is ok
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        fprintf(stderr, "Error: Depth-Map-Framebuffer incomplete!\n");
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

/**
 * Wird aufgerufen, um den aktuellen Framebuffer zu löschen und neu zu erstellen
 * 
 * @param PPfbo zu initialisierender Framebuffer
 * @param fbo zu initialisierender Framebuffer
 * @param width die Breite des Framebuffers
 * @param height die Höhe des Framebuffers
 */
void framebuffer_resizeFramebuffer(PingPong *PPfbo, Framebuffer *fbo, int width, int height) {
    framebuffer_deleteFrameBuffer(fbo);
    framebuffer_deletePingPongBuffer(PPfbo);
    framebuffer_initFramebuffer(fbo, width, height);
    framebuffer_initPingPongBuffer(PPfbo, width, height);
}

/**
 * Löscht den Framebuffer
 * 
 * @param fb zu löschender Framebuffer
 */
void framebuffer_deleteFrameBuffer(Framebuffer *fb) {
    glDeleteFramebuffers(1, &fb->fbo);
    glDeleteRenderbuffers(1, &fb->depthRbo);
    glDeleteTextures(GBUFFER_NUM_COLORATTACH, fb->textures);
}

/**
 * Löscht den PingPongFramebuffer
 * 
 * @param fb zu löschender Framebuffer
 */
void framebuffer_deletePingPongBuffer(PingPong *fb) {
    glDeleteFramebuffers(1, &fb->fbo);
    glDeleteTextures(2, fb->buffer);
    glDeleteRenderbuffers(1, &fb->depthRbo);
}

void framebuffer_deleteDepthFrameBuffer(depthFBO *fb) {
    glDeleteFramebuffers(1, &fb->fbo);
    glDeleteTextures(1, &fb->depthMap);
}

void framebuffer_deleteDepthCubeFrameBuffer(depthCubeFBO *fb) {
    glDeleteFramebuffers(1, &fb->fbo);
    glDeleteTextures(1, &fb->depthCubeMap);
}