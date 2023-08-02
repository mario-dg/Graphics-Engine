#version 410 core

/**
 * 3D Modell Shader.
 * 
 * Copyright (C) 2020, FH Wedel
 * Autor: Nicolas Hollmann
 */

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 3) in vec2 texCoord;

// Eigenschaften, die an den Fragmentshader weitergegeben werden sollen.
out VS_OUT  {
    vec2 TexCoords;
    vec3 Normal;
    vec3 eyeDir;
} vs_out;

// Model-View-Projection Matrix.
uniform mat4 mvpMatrix;
uniform mat4 projectionMatrix;
uniform mat4 modelViewMatrix;
uniform mat4 modelMatrix;


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

    //PHONG
    // ------------------- //
    mat4 normalMatrix = transpose(inverse(modelMatrix));
    vec3 vertexNormal = (normalMatrix * vec4(normal, 0.0)).xyz;
    vertexNormal = normalize(vertexNormal);
    vs_out.Normal = vertexNormal;

    vec3 vertexInCamSpace = (modelViewMatrix * vec4(position, 1.0)).xyz;
    vs_out.eyeDir = -vertexInCamSpace.xyz;
    // ------------------- //

    gl_Position = mvpMatrix * vec4(position, 1.0);
}
