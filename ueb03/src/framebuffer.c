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
static void framebuffer_createAttachment(GLuint *attachment, bool isAlbedo, GLint width, GLint height, GLenum offset)
{
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
void framebuffer_initFramebuffer(Framebuffer *fb, int width, int height)
{
    // Erst das FBO anlegen.
    glGenFramebuffers(1, &fb->fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fb->fbo);

    //Texturen aanlegen und binden
    glGenTextures(GBUFFER_NUM_COLORATTACH, fb->textures);
    for (GLuint i = 0; i < GBUFFER_NUM_COLORATTACH; i++)
    {
        framebuffer_createAttachment(&fb->textures[GBUFFER_COLORATTACH_POSITION + i], false, width, height, GL_COLOR_ATTACHMENT0 + i);
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

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
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
void framebuffer_initPingPongBuffer(PingPong *fbo, int width, int height)
{
    glGenFramebuffers(2, fbo->fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo->fbo[0]);
    framebuffer_createAttachment(&fbo->buffer[0], true, width, height, GL_COLOR_ATTACHMENT0);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo->fbo[1]);
    framebuffer_createAttachment(&fbo->buffer[1], true, width, height, GL_COLOR_ATTACHMENT0);
}

/**
 * Wird aufgerufen, um den aktuellen Framebuffer zu löschen und neu zu erstellen
 * 
 * @param PPfbo zu initialisierender Framebuffer
 * @param fbo zu initialisierender Framebuffer
 * @param width die Breite des Framebuffers
 * @param height die Höhe des Framebuffers
 */ 
void framebuffer_resizeFramebuffer(PingPong *PPfbo, Framebuffer *fbo, int width, int height)
{
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
void framebuffer_deleteFrameBuffer(Framebuffer *fb)
{
    glDeleteFramebuffers(1, &fb->fbo);
    glDeleteRenderbuffers(1, &fb->depthRbo);
    glDeleteTextures(GBUFFER_NUM_COLORATTACH, fb->textures);
}

/**
 * Löscht den PingPongFramebuffer
 * 
 * @param fb zu löschender Framebuffer
 */ 
void framebuffer_deletePingPongBuffer(PingPong *fb)
{
    glDeleteFramebuffers(2, fb->fbo);
    glDeleteTextures(2, fb->buffer);
}
