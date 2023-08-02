#version 430 core

layout(vertices = 3) out;

uniform bool useTessellation;
uniform float innerTessellation;
uniform float outerTessellation;
uniform bool useDistanceTessellation;
uniform float tessellationAmount;
uniform vec3 camPos;

in VS_OUT  {
    vec3 FragPos;
    vec2 TexCoords;
    vec3 Normal;
    vec3 Tangent;
    vec3 Bitangent;
} cs_in[];

out VS_OUT  {
    vec3 FragPos;
    vec2 TexCoords;
    vec3 Normal;
    vec3 Tangent;
    vec3 Bitangent;
} cs_out[];

//Bestimmt den Level of Detail anhand einer Funktion
float LODFactor(float dist){
    return max(0.0, 600.0 / pow(dist, 1.8) + 0.3) / tessellationAmount;
}

void main() {
    //Attribute fuer Weitergabe uebernehmen
    cs_out[gl_InvocationID].FragPos = cs_in[gl_InvocationID].FragPos;
    cs_out[gl_InvocationID].TexCoords = cs_in[gl_InvocationID].TexCoords;
    cs_out[gl_InvocationID].Normal = cs_in[gl_InvocationID].Normal;
    cs_out[gl_InvocationID].Tangent = cs_in[gl_InvocationID].Tangent;
    cs_out[gl_InvocationID].Bitangent = cs_in[gl_InvocationID].Bitangent;

    float inner = 1;
    float outer[3];
    outer[0] = 1;
    outer[1] = 1;
    outer[2] = 1;
    //Variablen ueberschrieben, falls tessellation statfinden soll
    if(useTessellation){
        inner = innerTessellation;
        outer[0] = outerTessellation;
        outer[1] = outerTessellation;
        outer[2] = outerTessellation;
    }

    if(useDistanceTessellation){
        float distVert[3];
        //Berechnung der Kameraposition zu den Vertizes
        distVert[0] = distance(camPos, cs_in[0].FragPos);
        distVert[1] = distance(camPos, cs_in[1].FragPos);
        distVert[2] = distance(camPos, cs_in[2].FragPos);

        //Bestimmt Level of Detail einer Kante anhand der Kameraposition und seiner zwei Vertizes
        outer[0] = LODFactor((distVert[1] + distVert[2]) / 2.0);
        outer[1] = LODFactor((distVert[2] + distVert[0]) / 2.0);
        outer[2] = LODFactor((distVert[0] + distVert[1]) / 2.0);
    }

    if(gl_InvocationID == 0) {
        //Tessellations Variablen uebergeben
        gl_TessLevelOuter[0] = outer[0];
        gl_TessLevelOuter[1] = outer[1];
        gl_TessLevelOuter[2] = outer[2];
        gl_TessLevelInner[0] = useDistanceTessellation ? gl_TessLevelOuter[2] : inner;
    }
}