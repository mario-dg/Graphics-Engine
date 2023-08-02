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
    vec3 FragPos;
    vec2 TexCoords;
    vec3 Normal;
    vec3 Tangent;
    vec3 Bitangent;
    mat3 TBN;
} fs_in;

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

// Struktur für Punktlichteigenschaften.
struct PointLight {    
    vec3 position;
    vec3 middle;
    
    float constant;
    float linear;
    float quadratic;  

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};  

uniform PointLight pointLight;

uniform vec3 camPos;
uniform vec3 lightColor;
uniform vec3 lightDir;
uniform int displayOpt;
uniform bool useNormalMapping;
uniform bool useParallaxMapping;
uniform bool usePointLight;

uniform sampler2D depthMap;
uniform float heightScale;


vec3 calcPointLight(vec3 normal, vec3 newViewDir, vec2 texCoords, vec4 diffTex, vec4 specMap)
{
    vec3 lightDir = normalize(pointLight.position - pointLight.middle);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(newViewDir, reflectDir), 0.0), material.shininess);
    // attenuation
    float distance    = length(pointLight.position - fs_in.FragPos);
    float attenuation = 1.0 / (pointLight.constant + pointLight.linear * distance + 
  			     pointLight.quadratic * (distance * distance));    
    // combine results
    vec3 ambient  = pointLight.ambient * 0.3 * material.ambient;
    vec3 diffuse  = pointLight.diffuse * 0.6 * diff;
    diffuse *=  diffTex.rgb * material.diffuse;
    vec3 specular = pointLight.specular * spec;
    specular *= (material.useSpecularMap ? specMap.b * material.specular : material.specular);
    ambient  *= attenuation;
    diffuse  *= attenuation;
    specular *= attenuation;
    return (ambient + diffuse + specular);
} 

vec3 calcPhong(vec3 normal, vec3 newLightDir, vec3 newViewDir, vec2 texCoords) {
    vec4 diffTex;
    if(material.useDiffuseMap){
        diffTex = texture(material.diffuseMap, texCoords);
        if(diffTex.a < 0.1f) {
            discard;
        }
    }
    //Lichtquelle initialisieren
    LightSource light;
    light.ambient = lightColor * 0.4f;
    light.diffuse = lightColor * 0.7f;
    light.specular = lightColor;

    //Eye, Nomral und Lichtrichtung normalisieren
    vec3 E = normalize(newViewDir);
    vec3 N = normalize(normal);
    vec3 L = normalize(newLightDir);

    vec3 reflectDir = reflect(-L, N);
    reflectDir = normalize(reflectDir);
    float specularAngle = max(dot(reflectDir, E), 0.0);

    //Ambient, Defuse, Spekularen Anteil berrechnen / uebernehmen
    vec3 ambient = light.ambient;
    vec3 diffuse = light.diffuse * max(dot(L,N), 0);
    vec3 specular = light.specular * pow(specularAngle, material.shininess);

    ambient *= material.ambient;
    diffuse *= material.useDiffuseMap ? diffTex.rgb * material.diffuse : material.diffuse;

    vec4 specMap;
    if(material.useSpecularMap){
        specMap = texture(material.specularMap, texCoords);
    }
    specular *= material.useSpecularMap ? specMap.b * material.specular : material.specular;

    //Phong ohne Punktlichtquelle Berrechenen
    vec3 phong = vec3(ambient + diffuse + specular);

    //Falls gefordert, Punktlichteinfluss berrechnen
    if(usePointLight){
        phong += calcPointLight(normal, newViewDir, texCoords, diffTex, specMap);
    }

    return phong;
}

vec2 parallaxMapping(vec2 texCoords, vec3 viewDir)
{
    const float minLayers = 5.0;
    const float maxLayers = 20.0;
    float numLayers = mix(maxLayers, minLayers, max(dot(vec3(0.0, 0.0, 1.0), viewDir), 0.0));
    // calculate the size of each layer
    float layerDepth = 1.0 / numLayers;
    // depth of current layer
    float currentLayerDepth = 0.0;
    // the amount to shift the texture coordinates per layer (from vector P)
    vec2 P = viewDir.xy * heightScale; 
    vec2 deltaTexCoords = P / numLayers;

    // get initial values
    vec2  currentTexCoords     = texCoords;
    float currentDepthMapValue = 1 - texture(depthMap, currentTexCoords).r;
  
    while(currentLayerDepth < currentDepthMapValue)
    {
        // shift texture coordinates along direction of P
        currentTexCoords -= deltaTexCoords;
        // get depthmap value at current texture coordinates
        currentDepthMapValue = 1 - texture(depthMap, currentTexCoords).r;  
        // get depth of next layer
        currentLayerDepth += layerDepth;  
    }
    
    return currentTexCoords;
}

/**
 * Hauptfunktion des Fragment-Shaders.
 * Hier wird die Farbe des Fragmentes bestimmt.
 */
void main()
{
    //Blick-/Licht-Richtung bestimmen
    vec3 newViewDir = normalize(camPos - fs_in.FragPos);
    vec3 newLightDir = lightDir;

    //Parallax mapping
    vec2 texCoords = fs_in.TexCoords;
    
    if(useParallaxMapping){
        texCoords = parallaxMapping(fs_in.TexCoords, newViewDir);
        if(texCoords.x > 1.0 || texCoords.y > 1.0 || texCoords.x < 0.0 || texCoords.y < 0.0)
            discard;
    }
    
    //Normal mapping
    vec3 normal = texture(material.normalMap, texCoords).xyz;
    normal = normal * 2.0 - 1.0;
    normal = normalize(fs_in.TBN * normal);
    if(!useNormalMapping){
        normal = fs_in.Normal;
    }

    switch(displayOpt){
        case 0: 
            fragColor = vec4(calcPhong(normal, newLightDir, newViewDir, texCoords), 1.0f);
            break;
        case 1: 
            fragColor = vec4(fs_in.Normal, 1.0f);
            break;
        case 2: 
            fragColor = vec4(fs_in.TexCoords, 1.0, 1.0);
            break;
        case 3: 
            fragColor = material.useDiffuseMap ? texture(material.diffuseMap, texCoords) : vec4(0);
            break;   
        case 4: 
            fragColor = material.useSpecularMap ? vec4(vec3(texture(material.specularMap, texCoords).b), 1.0f) : vec4(0);
            break;
        case 5: 
            fragColor = material.useEmissionMap ? texture(material.emissionMap, texCoords) : vec4(0);
            break;
        case 6: 
            fragColor = material.useNormalMap ? vec4(fs_in.TBN * (texture(material.normalMap, fs_in.TexCoords) * 2.0 - 1.0).rgb, 1.0) : vec4(0);
            break;
        case 7:
            fragColor = vec4(vec3(texture(depthMap, fs_in.TexCoords).r), 1.0);
            break;
        default: 
            fragColor = vec4(1.0f);
    }

}