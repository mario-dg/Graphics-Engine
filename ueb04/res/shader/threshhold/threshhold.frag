#version 410 core

layout (location = 0) out vec4 threshhold;

uniform sampler2D finalTex;
uniform sampler2D emissionTex;

uniform float colorWeight;
uniform float emissionWeight;
uniform float threshholdValue;

in vec2 outTexCoord;

void main(){
    vec3 bloomColor = 
        texture(finalTex, outTexCoord).rgb * colorWeight + 
        texture(emissionTex, outTexCoord).rgb * emissionWeight;
    float brightness = dot(bloomColor, vec3(0.299, 0.587, 0.114));
    if(brightness > threshholdValue){
        threshhold = vec4(bloomColor, 1.0);
    } else {
        threshhold = vec4(0.0, 0.0, 0.0, 1.0);
    }
}