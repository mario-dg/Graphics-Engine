#version 410 core

layout (location = 0) out vec4 fragColor;

in vec2 outTexCoord;

uniform sampler2D brightTex;

uniform bool horizontal;
float weights[5] = float[] (0.06136, 0.24477, 0.38774, 0.24477, 0.06136);

void main() {
    vec2 texOffset = 1.0 / textureSize(brightTex, 0);
    vec3 res = texture(brightTex, outTexCoord).rgb * weights[0];

    if(horizontal){
        for(int i = 0; i < 5; ++i){
            res += texture(brightTex, outTexCoord + vec2(texOffset.x * i, 0.0)).rgb * weights[i];
            res += texture(brightTex, outTexCoord - vec2(texOffset.x * i, 0.0)).rgb * weights[i];
        }
    } else {
        for(int i = 0; i < 5; ++i){
            res += texture(brightTex, outTexCoord + vec2(0.0, texOffset.y * i)).rgb * weights[i];
            res += texture(brightTex, outTexCoord - vec2(0.0, texOffset.y * i)).rgb * weights[i];
        }
    }
    fragColor = vec4(1.0);
}