/**
 * Modul zum Abfragen von Benutzereingaben.
 * 
 * Copyright (C) 2020, FH Wedel
 * Autor: Nicolas Hollmann
 */

#include "input.h"

#include "window.h"
#include "texture.h"
#include "utils.h"
#include "rendering.h"

//////////////////////////// ÖFFENTLICHE FUNKTIONEN ////////////////////////////

void input_init(ProgContext *ctx)
{
    ctx->input = malloc(sizeof(InputData));
    InputData *data = ctx->input;

    // Werte initialisieren
    data->isFullscreen = false;
    data->showHelp = false;
    data->showMenu = true;
    data->showWireframe = false;
    data->showStats = true;

    // Rendering Werte initialisieren
    glm_vec4_zero(data->rendering.clearColor);
    data->rendering.clearColor[3] = 1.0f;

    glm_vec3_one(data->rendering.lightDirection);

    glm_vec3_one(data->rendering.lightColor);

    glm_vec3_zero(data->rendering.modelRotation);

    data->rendering.scale = 1.0f;

    data->rendering.displayOpt = PHONG;

    data->rendering.usePhong = false;

    data->rendering.userModel = NULL;

    // Kamera initialisieren
    data->mainCamera = camera_createCamera();
    glfwGetCursorPos(ctx->window, &data->mouseLastX, &data->mouseLastY);
    data->mouseLooking = false;
}

void input_process(ProgContext *ctx)
{
    // Kamerabewegung verarbeiten
    Camera *mainCamera = ctx->input->mainCamera;
    float deltaTime = (float)ctx->winData->deltaTime;

    if (glfwGetKey(ctx->window, GLFW_KEY_W) == GLFW_PRESS)
    {
        camera_processKeyboardInput(mainCamera, CAMERA_FORWARD, deltaTime);
    }
    if (glfwGetKey(ctx->window, GLFW_KEY_S) == GLFW_PRESS)
    {
        camera_processKeyboardInput(mainCamera, CAMERA_BACKWARD, deltaTime);
    }
    if (glfwGetKey(ctx->window, GLFW_KEY_A) == GLFW_PRESS)
    {
        camera_processKeyboardInput(mainCamera, CAMERA_LEFT, deltaTime);
    }
    if (glfwGetKey(ctx->window, GLFW_KEY_D) == GLFW_PRESS)
    {
        camera_processKeyboardInput(mainCamera, CAMERA_RIGHT, deltaTime);
    }
}

void input_event(ProgContext *ctx, int key, int action, int mods)
{
    // Mods wird aktuell nicht benutzt.
    (void)mods;

    // Input-Events verarbeiten.
    InputData *data = ctx->input;
    if (action == GLFW_PRESS)
    {
        switch (key)
        {
        /* Programm beenden */
        case GLFW_KEY_ESCAPE:
            glfwSetWindowShouldClose(ctx->window, true);
            break;

        /* Hilfe anzeigen/ausblenden */
        case GLFW_KEY_F1:
            data->showHelp = !data->showHelp;
            break;

        /* Fullscreen umschalten */
        case GLFW_KEY_F2:
            data->isFullscreen = !data->isFullscreen;
            window_updateFullscreen(ctx);
            break;

        /* Wireframe anzeigen/ausblenden */
        case GLFW_KEY_F3:
            data->showWireframe = !data->showWireframe;
            break;

        /* Programm-Menü anzeigen/ausblenden */
        case GLFW_KEY_F4:
            data->showMenu = !data->showMenu;
            break;

        /* Programmstatistiken anzeigen/ausblenden */
        case GLFW_KEY_F5:
            data->showStats = !data->showStats;
            break;

        /* Screenshot anfertigen */
        case GLFW_KEY_F6:
            texture_saveScreenshot(ctx);
            break;
        
        /* Shader neu kompilieren */
        case GLFW_KEY_F7:
            rendering_reRenderShaders(ctx);
            break;

        default:
            break;
        }
    }
}

void input_mouseMove(ProgContext *ctx, double x, double y)
{
    InputData *data = ctx->input;

    // Wir rotieren die Kamera nur, wenn zuvor der Bewegungsmodus mit der
    // linken Maustaste aktiviert wurde.
    if (data->mouseLooking)
    {
        // Veränderung berechnen.
        double xoff = x - data->mouseLastX;
        double yoff = data->mouseLastY - y;

        data->mouseLastX = x;
        data->mouseLastY = y;

        // Kamera rotieren.
        camera_processMouseInput(data->mainCamera, (float)xoff, (float)yoff);

        // Nocheinmal prüfen, ob die Maustaste wirklich gedrückt ist.
        // Es kann passieren, dass das RELEASE Event verschluckt wurde, z.B.
        // druch die GUI.
        data->mouseLooking = (glfwGetMouseButton(ctx->window, GLFW_MOUSE_BUTTON_LEFT) ==
                              GLFW_PRESS);
    }
}

void input_mouseAction(ProgContext *ctx, int button, int action, int mods)
{
    // Die Modifikatioren werden nicht benutzt.
    (void)mods;

    InputData *data = ctx->input;

    // Prüfen, ob die linke Maustaste verändert wurde.
    if (button == GLFW_MOUSE_BUTTON_LEFT)
    {
        if (action == GLFW_PRESS)
        {
            // Mausmodus aktivieren.
            glfwGetCursorPos(ctx->window, &data->mouseLastX, &data->mouseLastY);
            data->mouseLooking = true;
        }
        else if (action == GLFW_RELEASE)
        {
            // Mausmodus deaktivieren.
            data->mouseLooking = false;
        }
    }
}

void input_scroll(ProgContext *ctx, double xoff, double yoff)
{
    // Horizontales scrollen wird nicht benötigt.
    (void)xoff;

    // Zoom der Kamera anpassen.
    camera_processMouseZoom(ctx->input->mainCamera, (float)yoff);
}

void input_userSelectedFile(ProgContext *ctx, const char *path)
{
    Model *newModel = NULL;
    newModel = model_loadModel(path);

    // Wir tauschen das Modell nur aus, wenn es erfolgreich geladen werden
    // konnte.
    if (newModel != NULL)
    {
        // Sollte zuvor ein Modell geladen worden sein, muss es gelöscht werden.
        if (ctx->input->rendering.userModel != NULL)
        {
            model_deleteModel(ctx->input->rendering.userModel);
        }
        ctx->input->rendering.userModel = newModel;
    }
}

void input_cleanup(ProgContext *ctx)
{
    // Wenn eine Modelldatei geladen ist, muss diese gelöscht werden.
    if (ctx->input->rendering.userModel != NULL)
    {
        model_deleteModel(ctx->input->rendering.userModel);
    }

    camera_deleteCamera(ctx->input->mainCamera);

    free(ctx->input);
}
