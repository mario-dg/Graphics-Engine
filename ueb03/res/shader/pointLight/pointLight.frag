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

uniform vec3 camPos;
uniform int viewPortWidth;
uniform int viewPortHeight;

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
    return vec4(diffuse + specular, 1.0);
}

void main() {
    gFinal = calcPointLight();
}