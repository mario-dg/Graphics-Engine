#version 410 core
layout (location = 0) in vec3 aPosition;

uniform mat4 lightMVP;

void main () {
    gl_Position = lightMVP * vec4(aPosition, 1.0);
}
