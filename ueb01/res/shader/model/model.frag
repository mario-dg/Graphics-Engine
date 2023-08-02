#version 410 core

/**
 * 3D Modell Shader.
 * 
 * Copyright (C) 2020, FH Wedel
 * Autor: Nicolas Hollmann
 */

// Nur die Farbe wird ausgegeben.
layout (location = 0) out vec4 fragColor;

// Eigenschaften, die von dem Vertextshader weitergegeben wurden.
in VS_OUT {
    vec2 TexCoords;
    vec3 Normal;
    vec3 eyeDir;
} fs_in;

in vec3 eyeDir;

struct LightSource {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

// Struktur für Materialeigenschaften.
struct Material {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    vec3 emission;
    float shininess;

    bool useDiffuseMap;
    sampler2D diffuseMap;

    bool useSpecularMap;
    sampler2D specularMap;

    bool useNormalMap;
    sampler2D normalMap;

    bool useEmissionMap;
    sampler2D emissionMap;

    bool useHeightMap;
    sampler2D heightMap;
};

// Aktives material.
uniform Material material;
uniform vec3 lightDir;
uniform vec3 lightColor;
uniform int displayOpt;

vec3 calcPhong() {
    float a;
    if(material.useDiffuseMap){
        a = texture(material.diffuseMap, fs_in.TexCoords).a;
        if(a < 0.1f) {
            discard;
        }
    }
    LightSource light;
    light.ambient = lightColor * 0.4f;
    light.diffuse = lightColor;
    light.specular = lightColor;

    vec3 E = normalize(fs_in.eyeDir);
    vec3 N = normalize(fs_in.Normal);
    vec3 L = normalize(lightDir);

    vec3 reflectDir = reflect(-L, N);
    reflectDir = normalize(reflectDir);
    float specularAngle = max(dot(reflectDir, E), 0.0);

    vec3 ambient = light.ambient;
    vec3 diffuse = light.diffuse * max(dot(L,N), 0);
    vec3 specular = light.specular * pow(specularAngle, material.shininess);


    ambient *= material.ambient;
    diffuse *= material.useDiffuseMap ? vec3(texture(material.diffuseMap, fs_in.TexCoords)) * material.diffuse : material.diffuse;
    specular *= material.useSpecularMap ? vec3(a) * material.specular : material.specular;

    return vec3(ambient + diffuse + specular);
}

/**
 * Hauptfunktion des Fragment-Shaders.
 * Hier wird die Farbe des Fragmentes bestimmt.
 */
void main()
{
    switch(displayOpt){
        case 0: 
            fragColor = vec4(calcPhong(), 1.0f);
            break;
        case 1: 
            fragColor = vec4(fs_in.Normal, 1.0f);
            break;
        case 2: 
            fragColor = vec4(fs_in.TexCoords, 1.0, 1.0);
            break;
        case 3: 
            fragColor = material.useDiffuseMap ? texture(material.diffuseMap, fs_in.TexCoords) : vec4(0);
            break;   
        case 4: 
            fragColor = material.useSpecularMap ? vec4(vec3(texture(material.specularMap, fs_in.TexCoords).b), 1.0f) : vec4(0);
            break;
        case 5: 
            fragColor = material.useEmissionMap ? texture(material.emissionMap, fs_in.TexCoords) : vec4(0);
            break;
        case 6: 
            fragColor = material.useNormalMap ? texture(material.normalMap, fs_in.TexCoords) : vec4(0);
            break;
        default: 
            fragColor = vec4(1.0f);
    }

}