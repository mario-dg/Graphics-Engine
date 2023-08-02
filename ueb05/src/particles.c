/**
 * Partikelsimulationsmodul.
 * 
 * Copyright (C) 2020, FH Wedel
 * Autor: Nicolas Hollmann
 */

#include "particles.h"

#include <string.h>
#include "shader.h"
#include "input.h"
#include "utils.h"
#include "texture.h"

////////////////////////////////// KONSTANTEN //////////////////////////////////

// Gesammtzahl der Partikel
#define NUM_PARTICLES 1000

// Partikel pro lokaler Workgroup.
// Achtung: Dieser Wert muss identisch mit dem im Compute-Shader sein!
#define NUM_PARTICLES_PER_LOCAL_WORK_GROUP 100

//////////////////////////////////// MAKROS ////////////////////////////////////

#define rand01() ((float)rand() / (float)(RAND_MAX))

////////////////////////////// LOKALE DATENTYPEN ///////////////////////////////

// Datentyp für alle persistenten Daten des Renderers.
struct ParticleData {
    Shader* particleSimShader; //Simulations Shader
    Shader* particleDispShader; //Render Shader
    GLuint particlePosBuffer; //Position
    GLuint particleVelBuffer; //Velocity
    GLuint particleLifeBuffer; //Derzeitige Lebenszeit des Partikels
    GLuint particleVAO;
    GLuint lookupTexture;
    GLfloat lifeTime; //Maximale Lebenszeit 
};
typedef struct ParticleData ParticleData;

////////////////////////////// LOKALE FUNKTIONEN ///////////////////////////////

/**
 * Diese Funktion gibt relevante Informationen zu den Compute-Fähigkeiten auf
 * dem Computer aus.
 */
void particles_printComputeInfo(void)
{
    // Ermitteln der maximalen Anzahl an Workgroups im
    // Compute Shader.
    int workGroupCount[3];
    for (int i = 0; i < 3; i++)
    {
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, i, &workGroupCount[i]);
    }
    printf("Max global (total) work group size: x: %i, y: %i, z: %i\n",
            workGroupCount[0], workGroupCount[1], workGroupCount[2]);

    // Ermitteln der maximalen Groesse an lokalen Workgroups im
    // Compute Shader.
    int workGroupSize[3];
    for (int i = 0; i < 3; i++)
    {
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, i, &workGroupSize[i]);
    }
    printf("Max local (in one shader) work group sizes x: %i y: %i z: %i\n",
            workGroupSize[0], workGroupSize[1], workGroupSize[2]);

    // Ermitteln der maximalen Anzahl an workgroup units die eine 
    // lokale workgroup im copmute Shader erlaubt.
    int workGroupInv;
    glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &workGroupInv);
    printf("Max local work group invocations: %i\n", workGroupInv);
}

/**
 * Initialisiert den Positions-Partikelbuffer.
 * 
 * @param data Zugirff auf das Partikel-Datenobjekt.
 */
static void particles_initPosBuffer(ParticleData* data )
{
    // Zuerst wird der Positionsbuffer erstellt.
    glGenBuffers(1, &data->particlePosBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, data->particlePosBuffer);

    // Wir initialisieren einen leeren Buffer.
    glBufferData(
        GL_ARRAY_BUFFER, 
        NUM_PARTICLES * sizeof(vec4), 
        NULL, GL_STREAM_DRAW
    );
    
    // In dem folgendem Block werden die Buffer-Daten auf einen Zeiger gemappt.
    // Dieser Zeiger ist nur bis glUnmapBuffer gültig und kann benutzt werden,
    // um den Speicher zu initialsieren.
    vec4* positions = glMapBuffer(GL_ARRAY_BUFFER, GL_READ_WRITE);
    for (int i = 0; i < NUM_PARTICLES; i++)
    {
        memcpy(
            positions[i], 
            (vec4){0.0f, 0.0f, 0.0f, 1.0f}, 
            sizeof(vec4)
        );
    }
    glUnmapBuffer(GL_ARRAY_BUFFER);

    // Zum Schluss binden wir unseren Buffer an den Simulations-Shader.
    shader_useShader(data->particleSimShader);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, data->particlePosBuffer);
}

/**
 * Initialisiert den Geschwindigkeits-Partikelbuffer.
 * 
 * @param data Zugirff auf das Partikel-Datenobjekt.
 */
static void particles_initVelBuffer(ParticleData* data)
{
    // Zuerst wird der Buffer erstellt.
    glGenBuffers(1, &data->particleVelBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, data->particleVelBuffer);

    // Wir initialisieren einen leeren Buffer.
    glBufferData(
        GL_ARRAY_BUFFER, 
        NUM_PARTICLES * sizeof(vec4), 
        NULL, GL_STREAM_DRAW
    );
    
    // In dem folgendem Block werden die Buffer-Daten auf einen Zeiger gemappt.
    // Dieser Zeiger ist nur bis glUnmapBuffer gültig und kann benutzt werden,
    // um den Speicher zu initialsieren.
    vec4* velocities = glMapBuffer(GL_ARRAY_BUFFER, GL_READ_WRITE);
    for (int i = 0; i < NUM_PARTICLES; i++)
    {
        memcpy(
            velocities[i], 
            (vec4){ 0.0f, 5.0f, 0.0f , 1.0f}, 
            sizeof(vec4)
        );
    }
    glUnmapBuffer(GL_ARRAY_BUFFER);

    // Zum Schluss binden wir unseren Buffer an den Simulations-Shader.
    shader_useShader(data->particleSimShader);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, data->particleVelBuffer);
}

/**
 * Initialisiert den Positions-Partikelbuffer.
 * 
 * @param data Zugirff auf das Partikel-Datenobjekt.
 */
static void particles_initLife(ParticleData* data)
{
    // Zuerst wird der Lebenszeitsbuffer erstellt.
    glGenBuffers(1, &data->particleLifeBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, data->particleLifeBuffer);

    // Wir initialisieren einen leeren Buffer.
    glBufferData(
        GL_ARRAY_BUFFER, 
        NUM_PARTICLES * sizeof(float), 
        NULL, GL_STREAM_DRAW
    );
    
    // In dem folgendem Block werden die Buffer-Daten auf einen Zeiger gemappt.
    // Dieser Zeiger ist nur bis glUnmapBuffer gültig und kann benutzt werden,
    // um den Speicher zu initialsieren.
    float *lifeLeft = glMapBuffer(GL_ARRAY_BUFFER, GL_READ_WRITE);
    for (int i = 0; i < NUM_PARTICLES; i++)
    {
        lifeLeft[i] = 5.0f;
    }
    glUnmapBuffer(GL_ARRAY_BUFFER);

    // Zum Schluss binden wir unseren Buffer an den Simulations-Shader.
    shader_useShader(data->particleSimShader);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, data->particleLifeBuffer);
}

/**
 * Initialisiert das Partikel-VAO, das zum Rendern verwendet wird.
 * 
 * @param data Zugirff auf das Partikel-Datenobjekt.
 */
static void particles_initVAO(ParticleData* data)
{
    // VAO erzeugen.
    glGenVertexArrays(1, &data->particleVAO);
    glBindVertexArray(data->particleVAO);

    // Partikel Position
    glBindBuffer(GL_ARRAY_BUFFER, data->particlePosBuffer);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0,                                    // Die Attribut-Position
        4,                                    // Anzahl der Komponenten
        GL_FLOAT,                             // Datentyp der Komponenten
        GL_FALSE,                             // Normalisierung der Daten
        sizeof(vec4),                                    // Größe eines Datensatzes/Vertex
        NULL                                  // Offset der Daten in einem Struct
    );

    // Partikel Geschwindigkeit
    glBindBuffer(GL_ARRAY_BUFFER, data->particleVelBuffer);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        1,                                    // Die Attribut-Position
        4,                                    // Anzahl der Komponenten
        GL_FLOAT,                             // Datentyp der Komponenten
        GL_FALSE,                             // Normalisierung der Daten
        sizeof(vec4),                                    // Größe eines Datensatzes/Vertex
        NULL                                  // Offset der Daten in einem Struct
    );

    // Partikel Lebenszeit
    glBindBuffer(GL_ARRAY_BUFFER, data->particleLifeBuffer);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(
        2,                                    // Die Attribut-Position
        1,                                    // Anzahl der Komponenten
        GL_FLOAT,                             // Datentyp der Komponenten
        GL_FALSE,                             // Normalisierung der Daten
        sizeof(float),                                    // Größe eines Datensatzes/Vertex
        NULL                                  // Offset der Daten in einem Struct
    );
}

//////////////////////////// ÖFFENTLICHE FUNKTIONEN ////////////////////////////

void particles_init(ProgContext* ctx)
{
    //Speicher fuer alle Partikel anlegen
    ctx->particles = malloc(sizeof(ParticleData));
    ParticleData* data = ctx->particles;

    // Speicher bereinigen.
    memset(data, 0, sizeof(ParticleData));
    

    // Shader laden.
    data->particleDispShader = shader_createVeGeomFrShader(
        UTILS_CONST_RES("shader/particleDisp/particleDisp.vert"),
        UTILS_CONST_RES("shader/particleDisp/particleDisp.geom"),
        UTILS_CONST_RES("shader/particleDisp/particleDisp.frag")
    );

    data->particleSimShader = shader_createCompShader(
        UTILS_CONST_RES("shader/particleSim/particleSim.comp")
    );
    //Textur der partikel laden
    data->lookupTexture = texture_loadTexture(UTILS_CONST_RES("textures/particle.png"),GL_REPEAT, GL_FALSE);
    glBindTexture(GL_TEXTURE_2D, data->lookupTexture);

    // Buffer initialisieren.
    particles_initPosBuffer(data);
    particles_initVelBuffer(data);
    particles_initLife(data);
    particles_initVAO(data);
}

void particles_update(ProgContext* ctx)
{
    ParticleData* data = ctx->particles;


    // Shader aktivieren und Uniforms setzen.
    shader_useShader(data->particleSimShader);
    shader_setFloat(
        data->particleSimShader, "Dt", 
        (float)ctx->winData->deltaTime
    );
    shader_setVec3(data->particleSimShader, "Gravity", &ctx->input->particles.gravity);
    shader_setVec3(data->particleSimShader, "startPos", &ctx->input->particles.startPos);
    shader_setVec3(data->particleSimShader, "startDir", &ctx->input->particles.startDir);
    shader_setFloat(data->particleSimShader, "lifeTimeTotal", ctx->input->particles.lifeTime);
    shader_setFloat(data->particleSimShader, "lifeTimeRand", ctx->input->particles.lifeTimeRand);
    shader_setFloat(data->particleSimShader, "startDirRand", ctx->input->particles.startDirRand);

    // Der nächste Befehl startet die Berechnung auf der GPU.
    glDispatchCompute(NUM_PARTICLES / NUM_PARTICLES_PER_LOCAL_WORK_GROUP, 1, 1);

    // Dieser Befehl blockiert das Host-Programm, bis die Simulation abgeschlossen ist.
    // Grundsätzlich kann der Befehl auch erst später erfolgen, um weiterhin auf der CPU
    // zu arbeiten. Er muss aber aufgerufen werden, bevor die Partikel gerendert werden.
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void particles_draw(ProgContext* ctx, mat4 viewProjMat)
{
    ParticleData* data = ctx->particles;
    // Alpha-Blending aktivieren.
    glEnable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);


    // Shader aktivieren.
    shader_useShader(data->particleDispShader);

    //Kamera Position ermitteln und uebergeben
    vec3 *camPos;
    camPos = camera_getCameraPos(ctx->input->mainCamera);
    shader_setVec3(data->particleDispShader, "camPos", camPos);

    //Start und Endfarbe ubergeben
    shader_setVec3(data->particleDispShader, "startColor", &ctx->input->particles.startColor);
    shader_setVec3(data->particleDispShader, "endColor", &ctx->input->particles.endColor);

    //Start und Endgroesse uebergeben
    shader_setFloat(data->particleDispShader, "startSize", ctx->input->particles.startSize);
    shader_setFloat(data->particleDispShader, "endSize", ctx->input->particles.endSize);

    //Insgesammt Maximal moegliche Lebenszeit uebergeben
    float temp = ctx->input->particles.lifeTime + ctx->input->particles.lifeTimeRand;
    shader_setFloat(data->particleDispShader, "totalMaxLifeTime", temp);

    //View-Projektions-Matrix uebergeben
    mat4 vpMat;
    glm_mat4_copy(viewProjMat, vpMat);
    shader_setMat4(data->particleDispShader, "vpMat", &vpMat);

    // Color-Lookup Textur aktivieren und binden.
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, data->lookupTexture);
    shader_setInt(data->particleDispShader, "partTexture", 0);


    // Punkte rendern.
    glBindVertexArray(data->particleVAO);
    glDrawArrays(GL_POINTS, 0, NUM_PARTICLES);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
}

void particles_cleanup(ProgContext* ctx)
{
    ParticleData* data = ctx->particles;

    shader_deleteShader(data->particleDispShader);
    shader_deleteShader(data->particleSimShader);
    glDeleteBuffers(1, &data->particlePosBuffer);
    glDeleteBuffers(1, &data->particleVelBuffer);
    glDeleteVertexArrays(1, &data->particleVAO);
    glDeleteTextures(1, &data->lookupTexture);

    free(data);
}
