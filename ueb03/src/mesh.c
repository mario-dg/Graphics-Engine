/**
 * Modul für das verarbeiten von einzelnen Meshes / 3D Modellen.
 * Ein Mesh ist dabei eine Sammlung von Vertices und Indecies die gerendert
 * werden können. Außerdem besitzt ein Mesh auch ein Material.
 * 
 * Copyright (C) 2020, FH Wedel
 * Autor: Nicolas Hollmann
 */

#include "mesh.h"

////////////////////////////// LOKALE DATENTYPEN ///////////////////////////////

// Datenstruktur für die Repräsentation eines Meshes.
struct Mesh
{
    Vertex *vertices;
    GLuint vertexCount;

    GLint *indices;
    GLuint indexCount;

    GLuint vao; // Vertex Array Object
    GLuint vbo; // Vertex Buffer Object
    GLuint ebo; // Element Buffer Object

    Material *material;
};

//////////////////////////// ÖFFENTLICHE FUNKTIONEN ////////////////////////////

Mesh *mesh_createMesh(Vertex *vertices, GLuint vertexCount,
                      GLint *indices, GLuint indexCount, Material *material)
{
    // Zuerst wird der Speicher reserviert.
    Mesh *mesh = malloc(sizeof(Mesh));

    // Danach werden die Vertices festgelegt.
    mesh->vertices = vertices;
    mesh->vertexCount = vertexCount;

    // Dann die Indices.
    mesh->indices = indices;
    mesh->indexCount = indexCount;


    // Außerdem übernehmen wir das Material.
    mesh->material = material;

    // Dann legen wir die benötigten Buffer und Objekte an.
    glGenVertexArrays(1, &mesh->vao);
    glGenBuffers(1, &mesh->vbo);
    glGenBuffers(1, &mesh->ebo);

    // Ab jetzt binden wir das VAO.
    glBindVertexArray(mesh->vao);

    // Die folgenden Befehle übertragen die Vertexdaten an OpenGL.
    glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo);
    glBufferData(
        GL_ARRAY_BUFFER,
        mesh->vertexCount * sizeof(Vertex),
        &mesh->vertices[0],
        GL_STATIC_DRAW);

    // Und diese Befehle legen die Indicies fest.
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->ebo);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        mesh->indexCount * sizeof(GLint),
        &mesh->indices[0],
        GL_STATIC_DRAW);

    // Vertex Position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0,                                 // Die Attribut-Position
        3,                                 // Anzahl der Komponenten
        GL_FLOAT,                          // Datentyp der Komponenten
        GL_FALSE,                          // Normalisierung der Daten
        sizeof(Vertex),                    // Größe eines Datensatzes/Vertex
        (void *)offsetof(Vertex, position) // Offset der Daten in einem Vertex
    );

    // Vertex Normal
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        1,                               // Die Attribut-Position
        3,                               // Anzahl der Komponenten
        GL_FLOAT,                        // Datentyp der Komponenten
        GL_FALSE,                        // Normalisierung der Daten
        sizeof(Vertex),                  // Größe eines Datensatzes/Vertex
        (void *)offsetof(Vertex, normal) // Offset der Daten in einem Vertex
    );

    // Vertex Tangente
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(
        2,                               // Die Attribut-Position
        3,                               // Anzahl der Komponenten
        GL_FLOAT,                        // Datentyp der Komponenten
        GL_FALSE,                        // Normalisierung der Daten
        sizeof(Vertex),                  // Größe eines Datensatzes/Vertex
        (void *)offsetof(Vertex, tangent) // Offset der Daten in einem Vertex
    );

    // Vertex Texturkoordinaten
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(
        3,                                 // Die Attribut-Position
        2,                                 // Anzahl der Komponenten
        GL_FLOAT,                          // Datentyp der Komponenten
        GL_FALSE,                          // Normalisierung der Daten
        sizeof(Vertex),                    // Größe eines Datensatzes/Vertex
        (void *)offsetof(Vertex, texCoord) // Offset der Daten in einem Vertex
    );

    return mesh;
}

Mesh* mesh_createQuad(void)
{
    // Vertices anlegen.
    Vertex verticesOrig[4] = {
        {{-1.0f,  1.0f,  0.0f}, {0, 0, 1}, {0, 0, 1}, {0, 1}},
        {{ 1.0f,  1.0f,  0.0f}, {0, 0, 1}, {0, 0, 1}, {1, 1}},
        {{-1.0f, -1.0f,  0.0f}, {0, 0, 1}, {0, 0, 1}, {0, 0}},
        {{ 1.0f, -1.0f,  0.0f}, {0, 0, 1}, {0, 0, 1}, {1, 0}},
    };

    // Indices anlegen.
    GLint indicesOrig[24] = {
        0, 2, 1,
        1, 2, 3
    };

    // Vertices kopieren.
    unsigned int vertexCount = 4;
    Vertex* vertices = malloc(vertexCount * sizeof(Vertex));
    memcpy(vertices, verticesOrig, vertexCount * sizeof(Vertex));

    // Indices kopieren.
    unsigned int indexCount = 24;
    GLint* indices = malloc(indexCount * sizeof(GLint));
    memcpy(indices, indicesOrig, indexCount * sizeof(GLint));

    // Dummy-Material erzeugen.
    Material* mat = material_createMaterial(
        MATERIAL_DEFAULT_AMBIENT,
        MATERIAL_DEFAULT_DIFFUSE,
        MATERIAL_DEFAULT_SPECULAR,
        MATERIAL_DEFAULT_EMISSION,
        MATERIAL_DEFAULT_SHININESS
    );

    // Zum Schluss erzeugen wir ein neues Mesh und geben es zurück.
    return mesh_createMesh(
        vertices, vertexCount, 
        indices, indexCount,
        mat
    );
}

void mesh_drawMesh(Mesh *mesh, Shader *shader)
{
    // Nur rendern, wenn auch ein Mesh existiert.
    if (mesh == NULL)
    {
        return;
    }

    // Material aktivieren.
    material_useMaterial(shader, mesh->material);

    // Mesh rendern.
    glBindVertexArray(mesh->vao);
    glPatchParameteri(GL_PATCH_VERTICES, 3);
    glDrawElements(GL_PATCHES, mesh->indexCount, GL_UNSIGNED_INT, 0);
}

void mesh_drawMeshTris(Mesh *mesh, Shader *shader)
{
    // Nur rendern, wenn auch ein Mesh existiert.
    if (mesh == NULL)
    {
        return;
    }

    // Material aktivieren.
    material_useMaterial(shader, mesh->material);

    // Mesh rendern.
    glBindVertexArray(mesh->vao);
    glDrawElements(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, 0);
}

void mesh_deleteMesh(Mesh *mesh)
{
    // Nur löschen, wenn auch ein Mesh existiert.
    if (mesh == NULL)
    {
        return;
    }

    // Die lokalen Kopien löschen
    free(mesh->vertices);
    free(mesh->indices);

    // Das Material löschen.
    material_deleteMaterial(mesh->material);

    // Alle OpenGL Buffer löschen
    glDeleteBuffers(1, &mesh->vbo);
    glDeleteBuffers(1, &mesh->ebo);
    glDeleteVertexArrays(1, &mesh->vao);

    // Das Mesh löschen
    free(mesh);
}
