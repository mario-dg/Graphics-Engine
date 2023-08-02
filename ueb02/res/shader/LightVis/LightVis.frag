#version 410 core 

// Nur die Farbe wird ausgegeben.
layout (location = 0) out vec4 fragColor;

uniform vec3 color;

void main() {
    //Farbe zuweisen
    fragColor = vec4(color, 1.0);
}
