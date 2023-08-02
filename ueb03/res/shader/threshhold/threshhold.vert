#version 410 core
layout (location = 0) in vec3 position;
layout (location = 3) in vec2 texCoord;

out vec2 outTexCoord;

void main(){
    outTexCoord = texCoord;
    gl_Position = vec4(position, 1.0);
}