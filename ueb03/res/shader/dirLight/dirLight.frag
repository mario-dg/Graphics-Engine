#version 410 core

layout (location = 0) out vec4 gFinal;

in vec2 outTexCoord;

struct DirLight{
    vec3 dir;
    vec3 amb;
    vec3 diff;
    vec3 spec;
};

uniform DirLight dirLight;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoSpec;

uniform vec3 camPos;

vec4 calcPhong() {
    //Aus dem GBuffer auslesen
    vec3 pos = texture(gPosition, outTexCoord).rgb;
    vec3 normal = texture(gNormal, outTexCoord).rgb;
    vec4 albedoSpec = texture(gAlbedoSpec, outTexCoord);
    vec3 diffuse = albedoSpec.rgb;
    float specular = albedoSpec.a;

    //Ambienter Anteil
    vec3 lighting = diffuse * 0.1f * dirLight.amb;
    
    vec3 viewDir = normalize(camPos - pos);

    //Diffuser Anteil
    vec3 diff = max(dot(normal, normalize(dirLight.dir)), 0.0) * diffuse * dirLight.diff;
    //Spekularer Anteil
    vec3 halfDir = normalize(dirLight.dir + viewDir);
    float spec = pow(max(dot(normal, halfDir), 0.0), 16.0) * dirLight.spec.b;

    return vec4(lighting + diff + spec, 1.0);
//    return vec4(dirLight.dir, 1);
}

void main() {
    gFinal = calcPhong();
}