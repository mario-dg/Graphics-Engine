#version 430 core

/**
 * Partikel-Anzeige-Shader.
 * 
 * Copyright (C) 2020, FH Wedel
 * Autor: Henrik Patjens, Jonas Sorgenfrei, Nicolas Hollmann
 */

// -----------------------------------------------------------------------------
// Attribute
// -----------------------------------------------------------------------------

layout (location = 0) in vec4 position;
layout (location = 2) in float lifeLeftArray;

out float lifeLeft;
/**
 * Hauptfunktion des Vertex-Shaders.
 * Hier werden die Daten weitergereicht.
 */
void main()
{
    gl_Position = vec4(position.xyz, 1.0f);
    lifeLeft = lifeLeftArray;
}
