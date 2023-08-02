#version 430 core

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
} vs_out;

// Model-View-Projection Matrix.
uniform mat4 projectionMatrix;
uniform mat4 viewMatrix;
uniform mat4 modelMatrix;

/**
 * Hauptfunktion des Vertex-Shaders.
 * Hier werden die Daten weiter gereicht.
 */
void main()
{
    vs_out.TexCoords = texCoord;

    //neue FragPos + Normalen mit Rotation berechenen
    vs_out.FragPos = (modelMatrix * vec4(position, 1.0)).xyz;

    mat3 TIMM = mat3(transpose(inverse(modelMatrix)));
    vec3 T = TIMM * tangent;
    vec3 N = TIMM * normal;
    //Neu orthogonnalisieren in Bezug zu N (Gram-Schmidt Verfahren)
    T = normalize(T - dot(N, T) * N);
    vec3 B = cross(N, T);
    if(dot(cross(N,T),B) < 0.0){
        T = T * -1.0;
    }
    
    vs_out.Normal = N;
    vs_out.Bitangent = B;
    vs_out.Tangent = T;
}
