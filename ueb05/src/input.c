/**
 * Modul zum Abfragen von Benutzereingaben.
 * 
 * Copyright (C) 2020, FH Wedel
 * Autor: Nicolas Hollmann, Mario Da Graca, Christopher Ploog
 */

#include "input.h"

#include "window.h"
#include "texture.h"
#include "utils.h"
#include "scene.h"
#include "rendering.h"
#include "gui.h"

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
    data->showGBuffer = false;

    // Rendering Werte initialisieren
    glm_vec4_zero(data->rendering.clearColor);
    data->rendering.clearColor[3] = 1.0f;
    glm_vec3_zero(data->rendering.modelRotation);
    data->rendering.scale = 1.0f;
    data->rendering.userScene = NULL;

    //Lighting Inputs
    data->lighting.dirLightActive = true;
    glm_vec3_one(data->lighting.dirLight.direction);
    glm_vec3_one(data->lighting.dirLight.ambient);
    glm_vec3_one(data->lighting.dirLight.diffuse);
    glm_vec3_one(data->lighting.dirLight.specular);
    data->lighting.pointLightActive = true;
    data->lighting.lightVolSphere = NULL;

    //Mapping Inputs
    data->mapping.useDisplacement = false;
    data->mapping.displacementFactor = 0.01f;
    data->mapping.useNormalMapping = false;
    data->mapping.useParallax = false;
    data->mapping.heightScale = 0.15f;

    //Tessellation Inputs
    data->tessellation.useTessellation = false;
    data->tessellation.innerTessellation = 1.0f;
    data->tessellation.outerTessellation = 4.0f;
    data->tessellation.useDistanceTessellation = true;
    data->tessellation.tessellationAmount = 64.0f;

    //PostProcessing Inputs
    data->postProcessing.colorWeight = 1.0f;
    data->postProcessing.emissionWeight = 1.0f;
    data->postProcessing.threshhold = 1.0f;
    data->postProcessing.gamma = 2.2f;
    data->postProcessing.exposure = 1.0f;
    data->postProcessing.useBloom = true;
    data->postProcessing.blurIterations = 3;

    //Schatten Inputs
    data->shadows.createDirShadows = true;
    data->shadows.createPointLightShadows = false;
    data->shadows.showDirShadows = true;
    data->shadows.showPointShadows = false;
    data->shadows.realtimeDirShadows = false;
    data->shadows.useBilinearFiltering = true;
    data->shadows.PCFAmount = 1;

    //Particle Inputs
    glm_vec3_zero(data->particles.startPos);
    glm_vec3_zero(data->particles.startDir);
    data->particles.startDir[1] = 5.0f;
    data->particles.lifeTime = 5.0f;
    data->particles.lifeTimeRand = 1.0f;
    data->particles.startDirRand = 1.0f;
    glm_vec3_zero(data->particles.gravity);
    data->particles.gravity[1] = -1.0f;
    data->particles.startSize = 1.0f;
    data->particles.endSize = 1.0f;
    glm_vec3_one(data->particles.startColor);
    glm_vec3_one(data->particles.endColor);
    data->particles.pauseSim = false;

    Model *newSphere = NULL;
    newSphere = model_loadModel("..\\res\\models\\unitRadiusSphere.fbx");

    // Wir tauschen das Modell nur aus, wenn es erfolgreich geladen werden
    // konnte.
    if (newSphere != NULL)
    {
        ctx->input->lighting.lightVolSphere = newSphere;
    }

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

        /* Debug-Mode umschalten */
        case GLFW_KEY_F8:
            data->showGBuffer = !data->showGBuffer;
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

static void input_copyFirstSceneDirLight(ProgContext *ctx, Scene *scene)
{
    glm_vec3_copy(scene->dirLights[0]->direction, ctx->input->lighting.dirLight.direction);
    glm_vec3_copy(scene->dirLights[0]->ambient, ctx->input->lighting.dirLight.ambient);
    glm_vec3_copy(scene->dirLights[0]->diffuse, ctx->input->lighting.dirLight.diffuse);
    glm_vec3_copy(scene->dirLights[0]->specular, ctx->input->lighting.dirLight.specular);
}

void input_userSelectedFile(ProgContext *ctx, const char *path)
{
    // Sollte zuvor ein Modell geladen worden sein, muss es gelöscht werden.
    if (ctx->input->rendering.userScene)
    {
        scene_deleteScene(ctx->input->rendering.userScene);
        ctx->input->rendering.userScene = NULL;
        deleteTextureCache();
    }

    // Dann laden wir die neue Szene/das neue Modell.
    Scene *newScene = NULL;
    if (utils_hasSuffix(path, ".json"))
    {
        newScene = scene_loadScene(path);
        if (newScene != NULL)
        {
            input_copyFirstSceneDirLight(ctx, newScene);
            gui_setStartingColDir(&ctx->input->lighting.dirLight);
        }
    }
    else
    {
        newScene = scene_fromModel(path);
    }

    ctx->input->shadows.createDirShadows = true;
    ctx->input->shadows.createPointLightShadows = false;
    ctx->input->shadows.showPointShadows = false;
    ctx->input->rendering.userScene = newScene;
}

void input_cleanup(ProgContext *ctx)
{
    // Wenn eine Modelldatei geladen ist, muss diese gelöscht werden.
    if (ctx->input->rendering.userScene != NULL)
    {
        scene_deleteScene(ctx->input->rendering.userScene);
    }

    camera_deleteCamera(ctx->input->mainCamera);
    deleteTextureCache();

    free(ctx->input);
}
