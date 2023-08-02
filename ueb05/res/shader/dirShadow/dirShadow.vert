#version 430 core
layout (location = 0) in vec3 position;

uniform mat4 lightSpaceMat;
uniform mat4 modelMat;

void main() {
    //Szene aus sicht des Richtungslichtes rendern
    //Tiefeninformationen in Textur speichern
    gl_Position = lightSpaceMat * modelMat * vec4(position, 1.0);
}