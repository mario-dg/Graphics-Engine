#ifndef POSTPROCESSING_H
#define POSTPROCESSING_H
#include "framebuffer.h"
#include "shader.h"
#include "rendering.h"
#include "input.h"

void postProcessing_extractBrightColors(ProgContext *ctx);

int postProcessing_blur(ProgContext *ctx);

void postProcessing_finalRender(ProgContext *ctx);
#endif //POSTPROCESSING_H