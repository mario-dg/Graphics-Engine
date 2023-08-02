#version 410 core
//Nur die Position wird reingegeben
layout (location = 0) in vec3 position;


uniform mat4 mvpMatrix;

void main() {
    //Position berechnen
    gl_Position = mvpMatrix * vec4(position, 1.0);
}