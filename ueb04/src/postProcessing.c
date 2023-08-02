#include "postProcessing.h"

/**
 * Extrahiert die hellen Bereiche im Bild
 * 
 * @param ctx Programmkontext
 */
void postProcessing_extractBrightColors(ProgContext *ctx)
{
    // ---------------------- Threshhold - SHADER ---------------------------------- //
    RenderingData *data = ctx->rendering;
    InputData *input = ctx->input;

    //GBuffer binden
    glBindFramebuffer(GL_FRAMEBUFFER, data->fb.fbo);
    //Ins Brightness Attachment schreiben
    glDrawBuffer(data->fb.attachments[GBUFFER_COLORATTACH_BRIGHTNESS]);
    glClear(GL_COLOR_BUFFER_BIT);

    //Threshhold Shader aktivieren
    shader_useShader(data->threshhold);

    //Daten an Shader senden
    shader_setInt(data->threshhold, "finalTex", 10);
    shader_setInt(data->threshhold, "emissionTex", 11);
    shader_setFloat(data->threshhold, "colorWeight", input->postProcessing.colorWeight);
    shader_setFloat(data->threshhold, "emissionWeight", input->postProcessing.emissionWeight);
    //Threshhold Value, bestimmt ab welchem Grauwert extrahiert wird
    shader_setFloat(data->threshhold, "threshholdValue", input->postProcessing.threshhold);

    //Viewport fuellendes Quad rendern
    mesh_drawMeshTris(data->displayQuad, data->threshhold);
}

/**
 * Fuehrt einen Blur-PostFX anhand des PingPong Systems durch
 * 
 * @param ctx Programmkontext
 */
void postProcessing_blur(ProgContext *ctx)
{
    // ---------------------- Blur - SHADER ---------------------------------- //
    RenderingData *data = ctx->rendering;
    bool horizontal = true, first_iteration = true;
    int amount = ctx->input->postProcessing.blurIterations;
    shader_useShader(data->blur);
    glActiveTexture(GL_TEXTURE10);
    shader_setInt(data->blur, "brightTex", 10);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    
    glBindFramebuffer(GL_FRAMEBUFFER, data->pingPong.fbo);

    for (int i = 0; i < amount * 2; i++)
    {
        glDrawBuffer(GL_COLOR_ATTACHMENT0 + horizontal);
        shader_setBool(data->blur, "horizontal", horizontal);
        glBindTexture(
            GL_TEXTURE_2D, first_iteration ? data->fb.textures[GBUFFER_COLORATTACH_BRIGHTNESS] : data->pingPong.buffer[!horizontal]);
        mesh_drawMeshTris(data->displayQuad, data->blur);
        horizontal = !horizontal;
        if (first_iteration)
            first_iteration = false;
    }
}

/**
 * Finale Render Stage der Szene
 * 
 * @param ctx Programmkontext
 */
void postProcessing_finalRender(ProgContext *ctx)
{
    // ---------------------- PostProcessing - SHADER ---------------------------------- //
    RenderingData *data = ctx->rendering;
    InputData *input = ctx->input;

    //GBuffer aktivieren
    glBindFramebuffer(GL_FRAMEBUFFER, data->fb.fbo);
    //Ergebnis ins Result Attachment schreiben
    glDrawBuffer(data->fb.attachments[GBUFFER_COLORATTACH_RESULT]);

    //PostProcessing Shader aktivieren
    shader_useShader(data->postProcessing);
    //Daten an Shader senden
    shader_setInt(data->postProcessing, "finalTex", 15);
    shader_setInt(data->postProcessing, "bloomBlur", 16);
    shader_setFloat(data->postProcessing, "gamma", input->postProcessing.gamma);
    shader_setFloat(data->postProcessing, "exposure", input->postProcessing.exposure);
    shader_setBool(data->postProcessing, "useBloom", input->postProcessing.useBloom);
    //Display fuellendes Quad rendern
    mesh_drawMeshTris(data->displayQuad, data->postProcessing);
}