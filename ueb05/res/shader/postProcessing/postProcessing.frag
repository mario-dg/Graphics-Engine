#version 430 core

// Nur die Farbe wird ausgegeben.
layout (location = 0) out vec4 gFinal;

uniform sampler2D finalTex; 
uniform sampler2D bloomBlur;

uniform float gamma;
uniform float exposure;
uniform bool useBloom;

in vec2 outTexCoord;

void main(){
    vec3 hdrColor = texture(finalTex, outTexCoord).rgb;
    if(useBloom){
        vec3 bloomColor = texture(bloomBlur, outTexCoord).rgb;
        hdrColor += bloomColor;
    }
    //Fuer tone mapping mapped zurückgeben
    vec3 mapped = vec3(1.0) - exp(-hdrColor * exposure);
    //Gamme-Correction
    mapped = pow(mapped, vec3(1.0 / gamma));

    gFinal = vec4(mapped, 1.0);
}