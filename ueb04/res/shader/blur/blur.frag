#version 410 core

layout (location = 0) out vec4 FragColor;

in vec2 outTexCoord;

uniform sampler2D brightTex;

uniform bool horizontal;
uniform float weights[5] = float[] (0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

void main() {
    vec2 tex_offset = 1.0 / textureSize(brightTex, 0); // gets size of single texel
    vec3 result = texture(brightTex, outTexCoord).rgb * weights[0]; // current fragment's contribution
    if(horizontal)
    {
        for(int i = 1; i < 5; ++i)
        {
            result += texture(brightTex, outTexCoord + vec2(tex_offset.x * i, 0.0)).rgb * weights[i];
            result += texture(brightTex, outTexCoord - vec2(tex_offset.x * i, 0.0)).rgb * weights[i];
        }
    }
    else
    {
        for(int i = 1; i < 5; ++i)
        {
            result += texture(brightTex, outTexCoord + vec2(0.0, tex_offset.y * i)).rgb * weights[i];
            result += texture(brightTex, outTexCoord - vec2(0.0, tex_offset.y * i)).rgb * weights[i];
        }
    }
    FragColor = vec4(result, 1.0);
}