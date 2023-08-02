#include "skybox.h"
#include "model.h"
#include "texture.h"
#include "rendering.h"


static void skybox_createCube(GLuint *vbo, GLuint *vao)
{
    //Array fuer CubeMap Positionen
    float points[] = {
        -1.0f, 1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f, 1.0f, -1.0f,
        -1.0f, 1.0f, -1.0f,

        -1.0f, -1.0f, 1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f, 1.0f, -1.0f,
        -1.0f, 1.0f, -1.0f,
        -1.0f, 1.0f, 1.0f,
        -1.0f, -1.0f, 1.0f,

        1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f, 1.0f,
        -1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        1.0f, -1.0f, 1.0f,
        -1.0f, -1.0f, 1.0f,

        -1.0f, 1.0f, -1.0f,
        1.0f, 1.0f, -1.0f,
        1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        -1.0f, 1.0f, 1.0f,
        -1.0f, 1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f, 1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f, 1.0f,
        1.0f, -1.0f, 1.0f};

    glGenVertexArrays(1, vao);
    glBindVertexArray(*vao);

    glGenBuffers(1, vbo);
    glBindBuffer(GL_ARRAY_BUFFER, *vbo);
    glBufferData(GL_ARRAY_BUFFER, 3 * 36 * sizeof(float), &points, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
}

/**
 * Initialisiert die SkyBox
 * 
 * @param ctx Programmkontext
 */
void skybox_initSkyBox(SkyBox *skybox)
{
    //Wuerfel erstellen
    skybox_createCube(&skybox->vbo, &skybox->vao);

    //TexCube Textur erstellen
    texture_create_cube_map(UTILS_CONST_RES("textures/nz.png"),
                            UTILS_CONST_RES("textures/pz.png"),
                            UTILS_CONST_RES("textures/py.png"),
                            UTILS_CONST_RES("textures/ny.png"),
                            UTILS_CONST_RES("textures/nx.png"),
                            UTILS_CONST_RES("textures/px.png"), &skybox->texCube);
}

/**
 * Rendert die SkyBox
 * 
 * @param ctx Programmkontext
 * @param projectionMatrix ProjektionsMatrix der Szene
 * @param viewMatrix ViewMatrix der Szene
 */ 
void skybox_renderSkyBox(ProgContext *ctx, mat4 *projectionMatrix, mat4 *viewMatrix)
{
    RenderingData *data = ctx->rendering;
    /* ---------------------- Skybox - SHADER ---------------------------------- */
    if (data->skyboxShader)
    {
        glEnable(GL_DEPTH_TEST);
        //Depth-Buffer aus dem gBuffer in den Standard Frambuffer kopieren
        glBindFramebuffer(GL_READ_FRAMEBUFFER, data->fb.fbo);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glBlitFramebuffer(
            0, 0, ctx->winData->width, ctx->winData->height,
            0, 0, ctx->winData->width, ctx->winData->height,
            GL_DEPTH_BUFFER_BIT, GL_NEAREST);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        glDepthFunc(GL_LEQUAL);
        shader_useShader(data->skyboxShader);
        shader_setMat4(data->skyboxShader, "projection", projectionMatrix);
        shader_setMat4(data->skyboxShader, "view", viewMatrix);
        glActiveTexture(GL_TEXTURE10);
        glBindTexture(GL_TEXTURE_CUBE_MAP, data->skyBox.texCube);
        shader_setInt(data->skyboxShader, "skybox", 10);
        model_drawCubeMap(data->skyboxShader, &data->skyBox.vao, &data->skyBox.texCube);
        glDepthFunc(GL_LESS);
    }
}

/**
 * Loescht die Skybox
 * 
 * @param skybox zu loeschende SkyBox
 */ 
void skybox_deleteSkyBox(SkyBox *skybox){
    glDeleteTextures(1, &skybox->texCube);
}