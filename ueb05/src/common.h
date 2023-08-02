 /**
 * Wichtige Datentypen, Funktionen und Includes die
 * im gesammten Programm gebraucht werden.
 * 
 * Copyright (C) 2020, FH Wedel
 * Autor: Nicolas Hollmann
 */

#ifndef COMMON_H
#define COMMON_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <cglm/cglm.h>
#include <stdbool.h>
#include <stdlib.h>

////////////////////////////////// KONSTANTEN //////////////////////////////////

// Programmnamen setzten, wenn dies nicht durch das CMake Skript passiert ist.
#ifndef PROGRAM_NAME
    #define PROGRAM_NAME "SESP Unknown"
#endif

//////////////////////////// ÖFFENTLICHE DATENTYPEN ////////////////////////////

// Später genauer definierte Datentypen deklarieren,
// damit sie im Programmkontext als Zeiger verwendet werden können.
struct RenderingData;
struct GuiData;
struct InputData;

// Datentyp der allgemeine Informationen über das Fenster enthält.
struct WindowData {
    int width;                  // Die Framebuffer-Breite
    int height;                 // Die Framebuffer-Höhe
    int realWidth;              // Die Fensterbreite
    int realHeight;             // Die Fensterhöhe
    int cachedWidth;            // Die alte Fensterbreite (vor Fullscreen)
    int cachedHeight;           // Die alte Fensterhöhe (vor Fullscreen)
    int cachedPosX;             // Die alte X Fensterposition (v. Fullscreen)
    int cachedPosY;             // Die alte Y Fensterposition (v. Fullscreen)
    double deltaTime;           // Zeit die zwischen den Frames vergangen ist
    double lastFrameTime;       // Zeit des letzten Frames
    double lastFPSUpdateTime;   // Zeit der letzten FPS aktualisierung
    int fps;                    // Die gezählten FPS
    int frameCounter;           // Der aktuelle Frame-Zähler
};
typedef struct WindowData WindowData;

// Programmkontext-Datentyp. 
// Hier werden alle persistente Informationen gespeichert.
struct ProgContext {
    GLFWwindow* window;
    struct WindowData* winData;
    struct RenderingData* rendering;
    struct GuiData* gui;
    struct InputData* input;
    struct ParticleData* particles;
};
typedef struct ProgContext ProgContext;

//////////////////////////// ÖFFENTLICHE FUNKTIONEN ////////////////////////////

/**
 * Diese Funktion erstellt einen neuen, leeren Kontext.
 * Die einzelnen Module sind selbst dafür verantwortlich,
 * den Kontext zu füllen.
 * Der Kontext sollte mit common_deleteContext wieder
 * gelöscht werden, wenn er nicht mehr benötigt wird.
 * 
 * @return ein neuer, leerer Kontext.
 */
ProgContext* common_createContext(void);

/**
 * Diese Funktion löscht einen bestehenden Kontext wieder.
 * Zuvor MÜSSEN die anderen Module bereits ihre Informationen
 * wieder selbst gelöscht haben, sonst entsteht ein
 * Speicherleck.
 * 
 * @param ctx der Kontext, der gelöscht werden soll.
 */
void common_deleteContext(ProgContext* ctx);

#endif // COMMON_H
