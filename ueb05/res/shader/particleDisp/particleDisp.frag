#version 430 core

/**
 * Partikel-Anzeige-Shader.
 * 
 * Copyright (C) 2020, FH Wedel
 * Autor: Henrik Patjens, Jonas Sorgenfrei, Nicolas Hollmann, Mario Da Graca, Christopher Ploog
 */

// -----------------------------------------------------------------------------
// Attribute
// -----------------------------------------------------------------------------

out vec4 fragColor;
in float currLifeTime;
in float outTotalMaxLifeTime;
in vec2 TexCoord;

// -----------------------------------------------------------------------------
// Uniforms
// -----------------------------------------------------------------------------

uniform sampler2D partTexture;
uniform vec3 startColor;
uniform vec3 endColor;

/**
 * Hauptfunktion des Fragment-Shaders.
 * Hier wird die Farbe des Fragmentes bestimmt.
 */
void main()
{
    //Farbe ergibt sich aus Textur und einer Intapolation aus Start und End-Color mittels Lebenszeit 
    fragColor = texture(partTexture, TexCoord) * vec4(mix(endColor,startColor, currLifeTime / outTotalMaxLifeTime),1);
}
