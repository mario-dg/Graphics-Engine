#version 410 core

layout(vertices = 3) out;

uniform bool useTessellation;
uniform float innerTessellation;
uniform float outerTessellation;

in VS_OUT  {
    vec3 FragPos;
    vec2 TexCoords;
    vec3 Normal;
    vec3 Tangent;
    vec3 Bitangent;
    mat3 TBN;
} cs_in[];

out VS_OUT  {
    vec3 FragPos;
    vec2 TexCoords;
    vec3 Normal;
    vec3 Tangent;
    vec3 Bitangent;
    mat3 TBN;
} cs_out[];

void main() {
    //Attribute fuer Weitergabe uebernehmen
    cs_out[gl_InvocationID].FragPos = cs_in[gl_InvocationID].FragPos;
    cs_out[gl_InvocationID].TexCoords = cs_in[gl_InvocationID].TexCoords;
    cs_out[gl_InvocationID].Normal = cs_in[gl_InvocationID].Normal;
    cs_out[gl_InvocationID].Tangent = cs_in[gl_InvocationID].Tangent;
    cs_out[gl_InvocationID].Bitangent = cs_in[gl_InvocationID].Bitangent;
    cs_out[gl_InvocationID].TBN = cs_in[gl_InvocationID].TBN;

    float inner = 1;
    float outer = 1;
    //Variablen ueberschrieben, falls tessellation statfinden soll
    if(useTessellation){
        inner = innerTessellation;
        outer = outerTessellation;
    }

    if(gl_InvocationID == 0) {
        //Tessellations Variablen uebergeben
        gl_TessLevelInner[0] = inner;
        gl_TessLevelOuter[0] = outer;
        gl_TessLevelOuter[1] = outer;
        gl_TessLevelOuter[2] = outer;
    }
}