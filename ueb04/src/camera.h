/**
 * Modul für die Steuerung einer frei beweglichen 3D Kamera.
 * 
 * Copyright (C) 2020, FH Wedel
 * Autor: Nicolas Hollmann
 */

#ifndef CAMERA_H
#define CAMERA_H

#include "common.h"

//////////////////////////// ÖFFENTLICHE DATENTYPEN ////////////////////////////

// Datenstruktur für die Repräsentation einer 3D Kamera.
struct Camera;
typedef struct Camera Camera;

// Aufzählungstyp für die Bewegung der Kamera.
enum CameraMovement
{
    CAMERA_FORWARD,
    CAMERA_BACKWARD,
    CAMERA_LEFT,
    CAMERA_RIGHT
};
typedef enum CameraMovement CameraMovement;

//////////////////////////// ÖFFENTLICHE FUNKTIONEN ////////////////////////////

/**
 * Erzeugt eine neue Kamera.
 * 
 * @return die neu erzeugte Kamera.
 */
Camera* camera_createCamera(void);

/**
 * Berechnet die View Matrix der Kamera und gibt sie über einen 
 * Parameter zurück.
 * 
 * @param camera die Kamera, dessen View Matrix bestimmt werden soll.
 * @param view die Ausgabematrix, in die die View Matrix geschrieben wird.
 */
void camera_getViewMatrix(Camera* camera, mat4 view);

/**
 * Gibt die Zoomeinstellung einer Kamera aus.
 * 
 * @param camera die Kamera dessen Zoom ausgelesen werden soll.
 * @return die Zoomeinstellung der übergebenen Kamera.
 */
float camera_getZoom(Camera* camera);
/**
 * Gibt die Yaw-Einstellung einer Kamera aus.
 * 
 * @param camera die Kamera dessen Zoom ausgelesen werden soll.
 * @return die Yaw-einstellung der übergebenen Kamera.
 */
float camera_getYaw(Camera* camera);
/**
 * Gibt die Pitch-Einstellung einer Kamera aus.
 * 
 * @param camera die Kamera dessen Zoom ausgelesen werden soll.
 * @return die Pitch-einstellung der übergebenen Kamera.
 */
float camera_getPitch(Camera* camera);
/**
 * Verarbeitet Bewegungseingaben für eine Kamera.
 * 
 * @param camera die Kamera, die bewegt werden soll.
 * @param movement die Art/Richtung der Bewegung.
 * @param deltaTime die Zeit seit dem letzten Frame.
 */
void camera_processKeyboardInput(Camera* camera, CameraMovement movement, 
                                 float deltaTime);

/**
 * Verarbeitet Mausbewegungen für eine Kamera.
 * 
 * @param camera die Kamera, die rotiert werden soll.
 * @param x die Mausbewegung in X-Richtung.
 * @param y die Mausbewegung in Y-Richtung.
 */
void camera_processMouseInput(Camera* camera, float x, float y);

/**
 * Verarbeitet Zoomen über die Maus für eine Kamera.
 * 
 * @param camera die Kamera, für die der Zoom geändert werden soll.
 * @param offset die Veränderung des Zooms.
 */
void camera_processMouseZoom(Camera* camera, float offset);

/**
 * Löscht eine Kamera.
 * 
 * @param camera die zu löschende Kamera.
 */
void camera_deleteCamera(Camera* camera);

vec3* camera_getCameraPos(Camera* camera);
#endif // CAMERA_H
