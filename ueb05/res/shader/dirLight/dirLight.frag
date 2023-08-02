#version 430 core

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
uniform sampler2D gShadowMap;

uniform vec3 camPos;

uniform mat4 lightSpaceMatrix;

uniform bool useShadows;
uniform bool usePCF;
uniform int PCFAmount;
uniform bool useBilinearFiltering;

//Liest Textur Wert aus der ShadowMap aus und vergleicht ihn mit
//dem übergebenen wert
float sampleShadowMap(vec2 coords, float compare) {
	return step(texture(gShadowMap, coords).r, compare);
}

//bilineares Filtering der Schattenwerte
float sampleShadowMapLinear(vec2 coords, float compare, vec2 texelSize) {
    //Position des Pixels in der Textur
	vec2 pixelPos = coords / texelSize + vec2(0.5);
    //Nachkommastellen
	vec2 fracPart = fract(pixelPos);
    //Position links oberhalb unseres eigentlichen Fragments
	vec2 start = (pixelPos - fracPart) * texelSize;
	
    //4 Punkte im Quadrat aus der ShadowMap samplen
	float botLeft = sampleShadowMap(start, compare);
	float botRight = sampleShadowMap(start + vec2(texelSize.x, 0.0), compare);
	float topLeft = sampleShadowMap(start + vec2(0.0, texelSize.y), compare);
	float topRight = sampleShadowMap(start + texelSize, compare);
	
    //Zuerst in y Richtung interpolieren
	float a = mix(botLeft, topLeft, fracPart.y);
	float b = mix(botRight, topRight, fracPart.y);
	
    //Ergebnis in x Richtung interpolieren
	return mix(a, b, fracPart.x);
}

float calcShadow(vec4 FragPos, vec3 normal)
{
	//Fragment Pos in LightSpace
    vec4 FragPosLightSpace = lightSpaceMatrix * FragPos;
    vec3 shadowCoord = FragPosLightSpace.xyz / FragPosLightSpace.w;
	
	//von -1,1 nach 0,1 verschieben
    shadowCoord.xyz = shadowCoord.xyz * 0.5f + 0.5f;
    float closest = texture(gShadowMap, shadowCoord.xy).r;
    float current = shadowCoord.z;

    if(current > 1.0) {
        return 0.0;
    }

    //Gleiche Tiefenwerte aus der ShadowMap gesampled
    //Shadow Acne entgegenwirken
    float bias = max(0.05 * (1.0f - dot(normal, dirLight.dir)), 0.005);
    float shadow = 0.0;

    if(usePCF) {
		//Schatten Werte aus der Umgebung zusammen addieren und Mittelwert bilden
        vec2 texelSize = 1.0 / textureSize(gShadowMap, 0);

        for(int x = -PCFAmount; x <= PCFAmount; ++x)
        {
            for(int y = -PCFAmount; y <= PCFAmount; ++y)
            {   
                //Bilinear Filtern -> Interpolation von der Umgebung
                //erhöht Samplingrate drastisch (*4)
                if(useBilinearFiltering){
				    shadow += sampleShadowMapLinear(shadowCoord.xy + vec2(x,y) * texelSize, current - bias, texelSize);
                } else {
                    shadow += sampleShadowMap(shadowCoord.xy + vec2(x, y) * texelSize, current - bias);
                }
            }    
        }
        shadow /= pow((2 * PCFAmount + 1), 2);

    } else {
        shadow = step(closest, current - bias);
    }

    return shadow;
}  

vec4 calcPhong() {
    //Aus dem GBuffer auslesen
    vec4 pos = texture(gPosition, outTexCoord);
    vec3 normal = texture(gNormal, outTexCoord).rgb;
    vec4 albedoSpec = texture(gAlbedoSpec, outTexCoord);
    vec3 diffuse = albedoSpec.rgb;
    float specular = albedoSpec.a;

    //Ambienter Anteil
    vec3 lighting = diffuse * 0.15f * dirLight.amb;
    
    vec3 viewDir = normalize(camPos - pos.xyz);

    //Diffuser Anteil
    vec3 diff = max(dot(normal, normalize(dirLight.dir)), 0.0) * diffuse * dirLight.diff;
    //Spekularer Anteil
    vec3 halfDir = normalize(dirLight.dir + viewDir);
    float spec = pow(max(dot(normal, halfDir), 0.0), 16.0) * dirLight.spec.b;

    //Schatten berechnen
    float shadow = 0.0f;
    if(useShadows){
        shadow = calcShadow(pos, normal);
    }
    return vec4((lighting + (1.0 - shadow) * (diff + spec) * diffuse), 1.0);
}

void main() {
    gFinal = calcPhong();
}