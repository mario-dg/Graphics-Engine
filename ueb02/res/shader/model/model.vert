#version 410 core

/**
 * 3D Modell Shader.
 * 
 * Copyright (C) 2020, FH Wedel
 * Autor: Nicolas Hollmann
 */

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 tangent;
layout (location = 3) in vec2 texCoord;

// Eigenschaften, die an den Fragmentshader weitergegeben werden sollen.
out VS_OUT  {
    vec3 FragPos;
    vec2 TexCoords;
    vec3 Normal;
    vec3 Tangent;
    vec3 Bitangent;
    mat3 TBN;
} vs_out;

// Model-View-Projection Matrix.
uniform mat4 mvpMatrix;
uniform mat4 projectionMatrix;
uniform mat4 modelViewMatrix;
uniform mat4 modelMatrix;


uniform vec3 rotation;
//PHONG
// ------------------- //
uniform vec3 lightPos;
// ------------------- //
/**
 * Hauptfunktion des Vertex-Shaders.
 * Hier werden die Daten weiter gereicht.
 */
void main()
{
    vs_out.TexCoords = texCoord;

    // RotationsMatrix in X,Y,Z berrechnen und vereinen
    mat3 rotateX = mat3(1, 0, 0,
                    0, cos(rotation[0]), -sin(rotation[0]),
                    0, sin(rotation[0]), cos(rotation[0]));
    mat3 rotateY = mat3(cos(rotation[1]), 0, sin(rotation[1]),
                    0, 1, 0,
                    -sin(rotation[1]), 0, cos(rotation[1]));
    mat3 rotateZ = mat3(cos(rotation[2]), -sin(rotation[2]), 0,
                    sin(rotation[2]), cos(rotation[2]), 0,
                    0, 0, 1);
    mat3 rotationMat = rotateX * rotateY * rotateZ;

    //neue FragPos + Normalen mit Rotation berechenen
    vs_out.FragPos = (vec4(position, 1.0) * modelMatrix * mat4(rotationMat)).xyz;
    vs_out.Normal = normal * rotationMat;
    vec3 tan = tangent * rotationMat;

    mat4 TIMM = transpose(inverse(modelMatrix));
    vec3 T = normalize(vec3(TIMM * vec4(tan, 1.0)));
    vec3 N = normalize(vec3(TIMM * vec4(vs_out.Normal, 1.0)));
    //Neu orthogonnalisieren in Bezug zu N (Gram-Schmidt Verfahren)
    T = normalize(T - dot(N, T) * N);
    vec3 B = cross(N, T);
    if(dot(cross(N,T),B) < 0.0)
        T = T * -1.0;
    vs_out.TBN = mat3(T,B,N);

    vs_out.Normal = N;
    vs_out.Bitangent = B;
    vs_out.Tangent = T;

    gl_Position = mvpMatrix * vec4(vs_out.FragPos, 1.0);
}
