#version 430 core
/**
 * Partikel-Geometrie-Shader.
 * 
 * Copyright (C) 2020, FH Wedel
 * Autor: Henrik Patjens, Jonas Sorgenfrei, Nicolas Hollmann, Mario Da Graca, Christopher Ploog
 */


// -----------------------------------------------------------------------------
// Attribute
// -----------------------------------------------------------------------------
layout (points) in;
layout (triangle_strip) out;
layout (max_vertices = 4) out;

out vec2 TexCoord;
out float currLifeTime;
out float outTotalMaxLifeTime;
in float lifeLeft[];

// -----------------------------------------------------------------------------
// Uniforms
// -----------------------------------------------------------------------------
uniform mat4 vpMat;
uniform vec3 camPos;

// Start- und Endgroesse der Partikel uebergeben
uniform float startSize;
uniform float endSize;
uniform float totalMaxLifeTime;



void main()
{
    currLifeTime = lifeLeft[0];
    outTotalMaxLifeTime = totalMaxLifeTime;
    //Groesse des Partikels mit Lebenszeit skaliert
    float sizeFactor = mix(endSize, startSize, currLifeTime / totalMaxLifeTime);

    vec3 Pos = gl_in[0].gl_Position.xyz;

    //Vektoren zum Aufspannen der Flaeche
    vec3 toCamera = normalize(camPos - Pos);
    vec3 up = vec3(0.0, 1.0, 0.0);
    vec3 right = cross(toCamera, up) * sizeFactor;

    //Unterer linke Ecke
    Pos -= (right * 0.5);
    gl_Position = vpMat * vec4(Pos, 1.0);
    TexCoord = vec2(0.0, 0.0);
    EmitVertex();

    //Obere linke Ecke
    Pos.y += 1.0 * sizeFactor;
    gl_Position = vpMat * vec4(Pos, 1.0);
    TexCoord = vec2(0.0, 1.0);
    EmitVertex();

    //untere rechte Ecke
    Pos.y -= 1.0 * sizeFactor;
    Pos += right;
    gl_Position = vpMat * vec4(Pos, 1.0);
    TexCoord = vec2(1.0, 0.0);
    EmitVertex();

    //Obere rechte Ecke
    Pos.y += 1.0 * sizeFactor;
    gl_Position = vpMat * vec4(Pos, 1.0);
    TexCoord = vec2(1.0, 1.0);
    EmitVertex();
    
    EndPrimitive();
    
}