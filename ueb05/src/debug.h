#include "common.h"

void debug_printMat4f(mat4 mat)
{
    for (int y = 0; y < 4; y++)
    {
        for (int x = 0; x < 4; x++)
        {
            printf("%f, ", mat[y][x]);
        }
        printf("\n");
    }
}

void debug_printVec4f(vec3 vec){
    printf("%f, %f, %f, %f\n", vec[0], vec[1], vec[2], vec[3]);
}

void debug_printVec3f(vec3 vec){
    printf("%f, %f, %f\n", vec[0], vec[1], vec[2]);
}

void debug_printVec2f(vec2 vec){
    printf("%f, %f\n", vec[0], vec[1]);
}