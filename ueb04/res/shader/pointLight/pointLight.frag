#version 410 core

layout (location = 0) out vec4 gFinal;

struct PointLight
{
    vec3 pos;
    vec3 amb;
    vec3 diff;
    vec3 spec;
    float constant;
    float linear;
    float quadratic;
};

uniform PointLight pointLight;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoSpec;
uniform samplerCube gShadowCube;

uniform vec3 camPos;
uniform int viewPortWidth;
uniform int viewPortHeight;

uniform float farPlane;
uniform mat4 lightMVP;

uniform bool useShadows;
uniform bool usePCF;
uniform int PCFAmount;
uniform bool useBilinearFiltering;

//20 zufaellige komplett unterschiedliche Richtungen
vec3 sampleOffsetDirections[20] = vec3[]
(
vec3( 1,  1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1,  1,  1), 
vec3( 1,  1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1,  1, -1),
vec3( 1,  1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1,  1,  0),
vec3( 1,  0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1,  0, -1),
vec3( 0,  1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0,  1, -1)
); 

//Liest Textur Wert aus der ShadowMap aus und vergleicht ihn mit
//dem übergebenen wert
float sampleShadowMap(vec3 coords, float compare, float farPlane) {
	return step(texture(gShadowCube, coords).r * farPlane, compare);
}

float calcShadow(vec3 fragPos)
{
    // get vector between fragment position and light position
    vec3 fragToLight = fragPos - pointLight.pos;
    // use the light to fragment vector to sample from the depth map    
    float closestDepth = texture(gShadowCube, fragToLight).r;
    // it is currently in linear range between [0,1]. Re-transform back to original value
    closestDepth *= farPlane;
    // now get current linear depth as the length between the fragment and light position
    float currentDepth = length(fragToLight);

    //Shadow Acne entgegenwirken
    float bias = 0.15; 
    float shadow = 0.0f;
    //Entspricht der Anzahl an zufaelligen Richtungen
    int samples = 20;

    if(usePCF){ 
        float viewDistance = length(camPos - fragPos);
        //Radius in dem gesampled werden soll
        float diskRadius = (1.0 + (viewDistance / farPlane)) / (25.0f / PCFAmount);

        for(int i = 0; i < samples; ++i)
        {
            shadow += sampleShadowMap(fragToLight + sampleOffsetDirections[i] * diskRadius, currentDepth - bias, farPlane);
        }
        //Mittelwert bilden
        shadow /= float(samples); 
    } else {
        shadow = step(closestDepth, currentDepth - bias);
    }

    return shadow;
    //return closestDepth / farPlane;
} 

vec4 calcPointLight()
{
    vec2 outTexCoord = gl_FragCoord.xy / vec2(viewPortWidth, viewPortHeight);
    //Aus dem GBuffer auslesen
    vec3 pos = texture(gPosition, outTexCoord).rgb;
    vec3 normal = texture(gNormal, outTexCoord).rgb;
    vec4 albedoSpec = texture(gAlbedoSpec, outTexCoord);
    vec3 diffuseTex = albedoSpec.rgb;
    float specularTex = albedoSpec.a;

    vec3 viewDir = normalize(camPos - pos);
    vec3 lightDir = normalize(pointLight.pos - pos);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 16.0);
    // attenuation
    float distance    = length(pointLight.pos - pos);
    float attenuation = 1.0 / (pointLight.constant + pointLight.linear * distance + 
  			     pointLight.quadratic * (distance * distance));    
    // combine results
    vec3 ambient  = pointLight.amb  * diffuseTex;
    vec3 diffuse  = pointLight.diff  * diff * diffuseTex;
    vec3 specular = pointLight.spec * spec * vec3(specularTex);
    
    diffuse  *= attenuation;
    specular *= attenuation;

    //Schatten berechnen
    float shadow = 0.0f;
    if(useShadows){
        shadow = calcShadow(pos);
    }                      
    return vec4(((1.0 - shadow) * (diffuse + specular)), 1.0);
    //return vec4(vec3(shadow), 1.0);
}

void main() {
    gFinal = calcPointLight();
}