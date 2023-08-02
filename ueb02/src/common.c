/**
 * Wichtige Datentypen, Funktionen und Includes die
 * im gesammten Programm gebraucht werden.
 * 
 * Copyright (C) 2020, FH Wedel
 * Autor: Nicolas Hollmann
 */

#include "common.h"

#include <stdlib.h>
#include <string.h>

//////////////////////////// ÖFFENTLICHE FUNKTIONEN ////////////////////////////

ProgContext* common_createContext(void)
{
    ProgContext* ctx = malloc(sizeof(ProgContext));

    ctx->winData = malloc(sizeof(WindowData));
    memset(ctx->winData, 0, sizeof(WindowData));

    ctx->window = NULL;
    ctx->input = NULL;
    ctx->rendering = NULL;
    ctx->gui = NULL;

    return ctx;
}

void common_deleteContext(ProgContext* ctx)
{
    free(ctx->winData);
    free(ctx);
}
