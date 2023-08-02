#version 410 core

/**
 * 3D Modell Shader.
 * 
 * Copyright (C) 2020, FH Wedel
 * Autor: Nicolas Hollmann
 */

layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gAlbedoSpec;
layout (location = 3) out vec3 gEmission;

// Eigenschaften, die von dem Vertextshader weitergegeben wurden.
in VS_OUT {
    vec3 FragPos;
    vec2 TexCoords;
    vec3 Normal;
    vec3 Tangent;
    vec3 Bitangent;
} fs_in;

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

uniform bool useParallaxMapping;
uniform float heightScale;
uniform sampler2D depthMap;

uniform bool useNormalMapping;
uniform vec3 camPos;

vec2 ParallaxMapping(vec2 texCoords, vec3 viewDir)
{ 
    // number of depth layers
    const float minLayers = 5;
    const float maxLayers = 50;
    float numLayers = mix(maxLayers, minLayers, max(dot(vec3(0.0, 0.0, 1.0), viewDir), 0.0));  
    // calculate the size of each layer
    float layerDepth = 1.0 / numLayers;
    // depth of current layer
    float currentLayerDepth = 0.0;
    // the amount to shift the texture coordinates per layer (from vector P)
    vec2 P = viewDir.xy / viewDir.z * heightScale; 
    vec2 deltaTexCoords = P / numLayers;
  
    // get initial values
    vec2  currentTexCoords     = texCoords;
    float currentDepthMapValue = 1.0f - texture(depthMap, currentTexCoords).r;
      
    while(currentLayerDepth < currentDepthMapValue)
    {
        // shift texture coordinates along direction of P
        currentTexCoords -= deltaTexCoords;
        // get depthmap value at current texture coordinates
        currentDepthMapValue = 1.0f - texture(depthMap, currentTexCoords).r;  
        // get depth of next layer
        currentLayerDepth += layerDepth;  
    }
    
    // get texture coordinates before collision (reverse operations)
    vec2 prevTexCoords = currentTexCoords + deltaTexCoords;

    // get depth after and before collision for linear interpolation
    float afterDepth  = currentDepthMapValue - currentLayerDepth;
    float beforeDepth = 1.0f - texture(depthMap, prevTexCoords).r - currentLayerDepth + layerDepth;
 
    // interpolation of texture coordinates
    float weight = afterDepth / (afterDepth - beforeDepth);
    vec2 finalTexCoords = prevTexCoords * weight + currentTexCoords * (1.0 - weight);

    return finalTexCoords;
}

/**
 * Hauptfunktion des Fragment-Shaders.
 */
void main()
{
    mat3 TBN = mat3(fs_in.Tangent, fs_in.Bitangent, fs_in.Normal);
    // offset texture coordinates with Parallax Mapping
    vec3 viewDir = transpose(TBN) * normalize(camPos - fs_in.FragPos);
    viewDir.y *= -1.0;
    vec2 texCoords = fs_in.TexCoords;

    //Parallaxmapping
    if(useParallaxMapping){
        texCoords = ParallaxMapping(fs_in.TexCoords,  viewDir);       
        if(texCoords.x > 1.0 || texCoords.y > 1.0 || texCoords.x < 0.0 || texCoords.y < 0.0)
            discard;
    }

    vec3 normal;
    //Normal mapping
    if(material.useNormalMap && useNormalMapping){
        normal = texture(material.normalMap, texCoords).rgb;
		normal.b = sqrt(1 - pow(normal.r, 2) - pow(normal.g, 2));
        normal = normal * 2.0 - 1.0;
        normal = TBN * normal;
        normal = normalize(normal);
    } else {
        normal = normalize(fs_in.Normal);
    }

    // store the fragment position vector in the first gbuffer texture
    gPosition = fs_in.FragPos;
    // also store the per-fragment normals into the gbuffer
    gNormal = normal;
    // and the diffuse per-fragment color
    if(material.useDiffuseMap){
        vec4 diffTex = texture(material.diffuseMap, texCoords);
        gAlbedoSpec.rgb = diffTex.rgb * material.diffuse;
        if(diffTex.a < 0.1f) {
            discard;
        }
    } else {
        gAlbedoSpec.rgb = material.diffuse;
    }
    // store specular intensity in gAlbedoSpec's alpha component
    gAlbedoSpec.a = material.useSpecularMap ? texture(material.specularMap, texCoords).b * material.specular.r: material.specular.r;
    ///store the emission per-fragment color
    gEmission = material.useEmissionMap ? texture(material.emissionMap, texCoords).rgb * material.emission: material.emission;
}