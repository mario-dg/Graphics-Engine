#version 410 core

layout (triangles, equal_spacing, ccw) in;
uniform mat4 gVP;
uniform sampler2D gDisplacementMap;
uniform float gDispFactor;
uniform mat4 mvpMatrix;
uniform float displacementFactor;
uniform bool useDisplacement;

in VS_OUT  {
    vec3 FragPos;
    vec2 TexCoords;
    vec3 Normal;
    vec3 Tangent;
    vec3 Bitangent;
    mat3 TBN;
} es_in[];

out VS_OUT  {
    vec3 FragPos;
    vec2 TexCoords;
    vec3 Normal;
    vec3 Tangent;
    vec3 Bitangent;
    mat3 TBN;
} es_out;

//Vektoren im 2D berreich interpolieren
vec2 interpolate2D(vec2 v0, vec2 v1, vec2 v2)
{
    return vec2(gl_TessCoord.x) * v0 + vec2(gl_TessCoord.y) * v1 + vec2(gl_TessCoord.z) * v2;
}

//Vektoren im 3D berreich interpolieren
vec3 interpolate3D(vec3 v0, vec3 v1, vec3 v2)
{
    return vec3(gl_TessCoord.x) * v0 + vec3(gl_TessCoord.y) * v1 + vec3(gl_TessCoord.z) * v2;
}

void main() {
    // Interpolate the attributes of the output vertex using the barycentric coordinates
    es_out.TexCoords = interpolate2D(es_in[0].TexCoords, es_in[1].TexCoords, es_in[2].TexCoords);
    es_out.Normal = normalize(interpolate3D(es_in[0].Normal, es_in[1].Normal, es_in[2].Normal));
    es_out.Tangent = normalize(interpolate3D(es_in[0].Tangent, es_in[1].Tangent, es_in[2].Tangent));
    es_out.Bitangent = normalize(interpolate3D(es_in[0].Bitangent, es_in[1].Bitangent, es_in[2].Bitangent));
    es_out.FragPos = interpolate3D(es_in[0].FragPos, es_in[1].FragPos, es_in[2].FragPos);
    es_out.TBN = es_in[0].TBN;
    if(useDisplacement){
        // Displace the vertex along the normal
        float Displacement = texture(gDisplacementMap, es_out.TexCoords.xy).x;
        es_out.FragPos += es_out.Normal * Displacement * displacementFactor;
    }

    gl_Position = mvpMatrix * vec4(es_out.FragPos, 1.0);
}