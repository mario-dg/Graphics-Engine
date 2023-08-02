/**
 * Graphisches Nutzerinterface für die Software.
 * 
 * Copyright (C) 2020, FH Wedel
 * Autor: Nicolas Hollmann
 */

#include "gui.h"

#include <string.h>
#include <sesp/nuklear.h>

#include "window.h"
#include "input.h"
#include "rendering.h"

////////////////////////////////// KONSTANTEN //////////////////////////////////

#define MAX_VERTEX_BUFFER 512 * 1024
#define MAX_ELEMENT_BUFFER 128 * 1024

#define STATS_WIDTH (80)
#define STATS_HEIGHT (30)

// Definitionen der Fenster IDs
#define GUI_WINDOW_HELP "window_help"
#define GUI_WINDOW_MENU "window_menu"
#define GUI_WINDOW_STATS "window_stats"

////////////////////////////// LOKALE DATENTYPEN ///////////////////////////////

// Datentyp für die GUI Daten
struct GuiData
{
    struct nk_glfw glfw;
    struct nk_context *nk;
};
typedef struct GuiData GuiData;

vec3 g_DirLightCol = {1.0f, 1.0f, 1.0f};
vec3 g_DirLightDir = {1.0f, 1.0f, 1.0f};
/////////////////////////////// LOKALE CALLBACKS ///////////////////////////////

/**
 * Callback Funktion, die aufgerufen wird, wenn eine Taste gedrückt wurde.
 * Die Events werden direkt an Nuklear weitergegeben.
 * 
 * @param win GLFW Fensterhandle.
 * @param codepoint 32bit Zeichencode.
 */
static void callback_glfwChar(GLFWwindow *win, unsigned int codepoint)
{
    ProgContext *ctx = (ProgContext *)glfwGetWindowUserPointer(win);
    nk_glfw3_char_callback(&ctx->gui->glfw, codepoint);
}

/**
 * Callback Funktion, die aufgerufen wird, wenn das Maus-Scollrad gedreht wird.
 * Die Events werden direkt an Nuklear weitergegeben.
 * 
 * @param win GLFW Fensterhandle.
 * @param xoff die Veränderung in X-Richtung (z.B. bei Touchpads).
 * @param yoff die Veränderung in Y-Richtung (z.B. klassisches Mausrad).
 */
static void callback_glfwScroll(GLFWwindow *win, double xoff, double yoff)
{
    ProgContext *ctx = (ProgContext *)glfwGetWindowUserPointer(win);

    // Prüfen, ob aktuell die GUI aktiv ist.
    if (nk_item_is_any_active(ctx->gui->nk))
    {
        // Wenn ja, Event an die GUI weiterleiten.
        nk_gflw3_scroll_callback(&ctx->gui->glfw, xoff, yoff);
    }
    else
    {
        // Wenn nicht, Event an das Input-Modul weiterleiten.
        input_scroll(ctx, xoff, yoff);
    }
}

/**
 * Callback Funktion, die aufgerufen wird, wenn eine Maustaste gedrückt wurde.
 * Die Events werden direkt an Nuklear weitergegeben.
 * 
 * @param win GLFW Fensterhandle.
 * @param button die gedrückte Taste.
 * @param action die Aktion, die das Event ausgelöst hat (z.B. loslassen).
 * @param mods aktivierte Modifikatoren.
 */
static void callback_glfwMouseButton(GLFWwindow *win, int button,
                                     int action, int mods)
{
    ProgContext *ctx = (ProgContext *)glfwGetWindowUserPointer(win);

    // Prüfen, ob aktuell die GUI aktiv ist.
    if (nk_item_is_any_active(ctx->gui->nk))
    {
        // Wenn ja, Event an die GUI weiterleiten.
        nk_glfw3_mouse_button_callback(
            &ctx->gui->glfw, win,
            button, action, mods);
    }
    else
    {
        // Wenn nicht, Event an das Input-Modul weiterleiten.
        input_mouseAction(ctx, button, action, mods);
    }
}

////////////////////////////// LOKALE FUNKTIONEN ///////////////////////////////

/**
 * Hilfswidget um Slider anzuzeigen, der die Skalierung
 * des 3D Modells als float speichert.
 * 
 * @param nk der Nuklear Kontext
 * @param name der anzuzeigende Name
 * @param col die einstellbare Skalierung
 */
static void gui_widgetScale(struct nk_context *nk, const char *name, float *scale)
{
    // Dann wird der Titel angezeigt.
    nk_layout_row_dynamic(nk, 20, 2);
    nk_label(nk, name, NK_TEXT_ALIGN_LEFT);

    nk_layout_row_static(nk, 15, 100, 2);
    nk_labelf(nk, NK_TEXT_LEFT, "Scale:");
    nk_slider_float(nk, 0.1f, scale, 4, 0.1f);
}

/**
 * Hilfswidget um einen 3D Vektor anzupassen.
 * 
 * @param nk der Nuklear Kontext
 * @param name der anzuzeigende Name
 * @param val der einstellbare Vektor
 */
static void gui_widgetVec3(struct nk_context *nk, const char *name, vec3 val)
{
    // Zuerst wird der Titel angezeigt.
    nk_layout_row_dynamic(nk, 20, 1);
    nk_label(nk, name, NK_TEXT_LEFT);

    // Danach zeigen wir die Auswahlfelder an.
    nk_layout_row_dynamic(nk, 25, 1);
    nk_property_float(nk, "#X:", -100.0f, &val[0], 100.0f, 1.0f, 0.1f);
    nk_property_float(nk, "#Y:", -100.0f, &val[1], 100.0f, 1.0f, 0.1f);
    nk_property_float(nk, "#Z:", -100.0f, &val[2], 100.0f, 1.0f, 0.1f);
}

/**
 * Hilfswidget um einen Colorpicker anzuzeigen, der die Farbe des Lichts
 * als vec4 speichert.
 * 
 * @param nk der Nuklear Kontext
 * @param name der anzuzeigende Name
 * @param col die einstellbare Farbe
 */
 void gui_widgetLightColor(struct nk_context *nk, const char *name, vec3 col)
{
    // Zuerst muss zwischen den Datentypen vec4 und
    // nk_colorf konvertiert werden.
    struct nk_colorf nkColor;
    nkColor.r = col[0];
    nkColor.g = col[1];
    nkColor.b = col[2];

    // Dann wird der Titel angezeigt.
    nk_layout_row_dynamic(nk, 20, 1);
    nk_label(nk, name, NK_TEXT_LEFT);

    // Danach zeigen wir den Colorpicker selbst an.
    nk_layout_row_dynamic(nk, 25, 1);
    if (nk_combo_begin_color(nk, nk_rgb_cf(nkColor),
                             nk_vec2(nk_widget_width(nk), 400)))
    {
        nk_layout_row_dynamic(nk, 120, 1);
        nkColor = nk_color_picker(nk, nkColor, NK_RGBA);
        nk_layout_row_dynamic(nk, 25, 1);
        nkColor.r = nk_propertyf(nk, "#R:", -1.0f, nkColor.r, 1.0f, 0.01f, 0.005f);
        nkColor.g = nk_propertyf(nk, "#G:", -1.0f, nkColor.g, 1.0f, 0.01f, 0.005f);
        nkColor.b = nk_propertyf(nk, "#B:", -1.0f, nkColor.b, 1.0f, 0.01f, 0.005f);
        nk_combo_end(nk);
    }

    // Zum Schluss wird wieder zurück konvertiert.
    col[0] = nkColor.r;
    col[1] = nkColor.g;
    col[2] = nkColor.b;
}

/**
 * Hilfswidget um einen Colorpicker anzuzeigen, der die Farbe des Lichts
 * als vec4 speichert.
 * 
 * @param nk der Nuklear Kontext
 * @param name der anzuzeigende Name
 * @param col die einstellbare Farbe
 */
static void gui_widgetLightColor3(struct nk_context *nk, const char *name, vec3 amb, vec3 diff, vec3 spec)
{
    // Dann wird der Titel angezeigt.
    nk_layout_row_dynamic(nk, 20, 1);
    nk_label(nk, name, NK_TEXT_LEFT);
    for (int i = 0; i < 3; i++)
    {

        // Zuerst muss zwischen den Datentypen vec4 und
        // nk_colorf konvertiert werden.
        struct nk_colorf nkColor;

        switch (i)
        {
        case 0:
            name = "Ambient";
            nkColor.r = amb[0];
            nkColor.g = amb[1];
            nkColor.b = amb[2];
            break;
        case 1:
            name = "Diffuse";
            nkColor.r = diff[0];
            nkColor.g = diff[1];
            nkColor.b = diff[2];
            break;
        default:
            name = "Specular";
            nkColor.r = spec[0];
            nkColor.g = spec[1];
            nkColor.b = spec[2];
            break;
        }
        // Dann wird der Titel angezeigt.
        nk_layout_row_dynamic(nk, 20, 1);
        nk_label(nk, name, NK_TEXT_LEFT);

        // Danach zeigen wir den Colorpicker selbst an.
        nk_layout_row_dynamic(nk, 25, 1);
        if (nk_combo_begin_color(nk, nk_rgb_cf(nkColor),
                                 nk_vec2(nk_widget_width(nk), 400)))
        {
            nk_layout_row_dynamic(nk, 120, 1);
            nkColor = nk_color_picker(nk, nkColor, NK_RGBA);
            nk_layout_row_dynamic(nk, 25, 1);
            nkColor.r = nk_propertyf(nk, "#R:", -1.0f, nkColor.r, 1.0f, 0.01f, 0.005f);
            nkColor.g = nk_propertyf(nk, "#G:", -1.0f, nkColor.g, 1.0f, 0.01f, 0.005f);
            nkColor.b = nk_propertyf(nk, "#B:", -1.0f, nkColor.b, 1.0f, 0.01f, 0.005f);
            nk_combo_end(nk);
        }

        switch (i)
        {
        case 0:
            amb[0] = nkColor.r;
            amb[1] = nkColor.g;
            amb[2] = nkColor.b;
            break;
        case 1:
            name = "Diffuse";
            diff[0] = nkColor.r;
            diff[1] = nkColor.g;
            diff[2] = nkColor.b;
            break;
        case 2:
            name = "Specular";
            spec[0] = nkColor.r;
            spec[1] = nkColor.g;
            spec[2] = nkColor.b;
            break;
        }
    }
}

/**
 * Hilfswidget um einen Colorpicker anzuzeigen, der die Farbe
 * als vec4 speichert.
 * 
 * @param nk der Nuklear Kontext
 * @param name der anzuzeigende Name
 * @param col die einstellbare Farbe
 */
static void gui_widgetColor(struct nk_context *nk, const char *name, vec4 col)
{
    // Zuerst muss zwischen den Datentypen vec4 und
    // nk_colorf konvertiert werden.
    struct nk_colorf nkColor;
    nkColor.r = col[0];
    nkColor.g = col[1];
    nkColor.b = col[2];
    nkColor.a = col[3];

    // Dann wird der Titel angezeigt.
    nk_layout_row_dynamic(nk, 20, 1);
    nk_label(nk, name, NK_TEXT_LEFT);

    // Danach zeigen wir den Colorpicker selbst an.
    nk_layout_row_dynamic(nk, 25, 1);
    if (nk_combo_begin_color(nk, nk_rgb_cf(nkColor),
                             nk_vec2(nk_widget_width(nk), 400)))
    {
        nk_layout_row_dynamic(nk, 120, 1);
        nkColor = nk_color_picker(nk, nkColor, NK_RGBA);
        nk_layout_row_dynamic(nk, 25, 1);
        nkColor.r = nk_propertyf(nk, "#R:", 0, nkColor.r, 1.0f, 0.01f, 0.005f);
        nkColor.g = nk_propertyf(nk, "#G:", 0, nkColor.g, 1.0f, 0.01f, 0.005f);
        nkColor.b = nk_propertyf(nk, "#B:", 0, nkColor.b, 1.0f, 0.01f, 0.005f);
        nkColor.a = nk_propertyf(nk, "#A:", 0, nkColor.a, 1.0f, 0.01f, 0.005f);
        nk_combo_end(nk);
    }

    // Zum Schluss wird wieder zurück konvertiert.
    col[0] = nkColor.r;
    col[1] = nkColor.g;
    col[2] = nkColor.b;
    col[3] = nkColor.a;
}

/**
 * Zeigt ein Hilfefenster an, in dem alle Maus- und Tastaturbefehle aufgelistet
 * werden.
 * 
 * @param ctx Programmkontext.
 * @param nk Abkürzung für das GUI Handle.
 */
static void gui_renderHelp(ProgContext *ctx, struct nk_context *nk)
{
    // Prüfen, ob die Hilfe überhaupt angezeigt werden soll.
    if (ctx->input->showHelp)
    {
        // Größe und Position des Fensters beim Öffnen bestimmen.
        float width = ctx->winData->realWidth * 0.25f;
        float height = ctx->winData->realHeight * 0.5f;
        float x = width * 1.5f;
        float y = height * 0.5f;

        // Fenster öffnen.
        if (nk_begin_titled(nk, GUI_WINDOW_HELP,
                            "Hilfe", nk_rect(x, y, width, height),
                            NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE |
                                NK_WINDOW_MINIMIZABLE | NK_WINDOW_TITLE))
        {
            // Vorlage für die Darstellung der Zeilen anlegen.
            nk_layout_row_template_begin(nk, 15);
            nk_layout_row_template_push_dynamic(nk);
            nk_layout_row_template_push_static(nk, 40);
            nk_layout_row_template_end(nk);

// Hilfsmakro zum einfachen füllen des Fensters.
#define HELP_LINE(dsc, key)                       \
    {                                             \
        nk_label(nk, (dsc), NK_TEXT_ALIGN_LEFT);  \
        nk_label(nk, (key), NK_TEXT_ALIGN_RIGHT); \
    }

            HELP_LINE("Programm beenden", "ESC");
            HELP_LINE("Hilfe umschalten", "F1");
            HELP_LINE("Fullscreen umschalten", "F2");
            HELP_LINE("Wireframe umschalten", "F3");
            HELP_LINE("Menü umschalten", "F4");
            HELP_LINE("Statistiken umschalten", "F5");
            HELP_LINE("Screenshot anfertigen", "F6");
            HELP_LINE("Shader neu kompilieren", "F7");
            HELP_LINE("Debug-Modus umschalten", "F8");
            HELP_LINE("Kamera vorwärst", "W");
            HELP_LINE("Kamera links", "A");
            HELP_LINE("Kamera zurück", "S");
            HELP_LINE("Kamera rechts", "D");
            HELP_LINE("Umsehen", "LMB");
            HELP_LINE("Zoomen", "Scroll");

// Makro wieder löschen, da es nicht mehr gebraucht wird.
#undef HELP_LINE

            // Vorlage für den Schließen-Button.
            nk_layout_row_template_begin(nk, 25);
            nk_layout_row_template_push_dynamic(nk);
            nk_layout_row_template_push_static(nk, 130);
            nk_layout_row_template_end(nk);

            // Ein leeres Textfeld zum Ausrichten des Buttons.
            nk_label(nk, "", NK_TEXT_ALIGN_LEFT);

            // Button zum Schließen des Fensters.
            if (nk_button_label(nk, "Hilfe schließen"))
            {
                ctx->input->showHelp = false;
            }
        }
        nk_end(nk);
    }
}

/**
 * Zeigt ein Konfigurationsmenü an, das benutzt werden kann, um die Szene oder
 * das Rendering an sich zu beeinflussen.
 * 
 * @param ctx Programmkontext.
 * @param nk Abkürzung für das GUI Handle.
 */
static void gui_renderMenu(ProgContext *ctx, struct nk_context *nk)
{
    InputData *input = ctx->input;

    // Prüfen, ob das Menü überhaupt angezeigt werden soll.
    if (input->showMenu)
    {
        // Größe des Fensters beim Öffnen bestimmen.
        float height = ctx->winData->realHeight * 0.7f;

        // Fenster öffnen.
        if (nk_begin_titled(nk, GUI_WINDOW_MENU, "Szenen-Einstellungen",
                            nk_rect(15, 15, 300, height),
                            NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE |
                                NK_WINDOW_MINIMIZABLE | NK_WINDOW_TITLE))
        {
            // Allgemeine Einstellungen anzeigen.
            if (nk_tree_push(nk, NK_TREE_TAB, "Allgemein", NK_MAXIMIZED))
            {
                nk_layout_row_dynamic(nk, 45, 2);

                if (nk_button_label(nk, "Hilfe umschalten"))
                {
                    input->showHelp = !input->showHelp;
                }

                if (nk_button_label(nk,
                                    input->isFullscreen ? "Fenstermodus" : "Vollbild"))
                {
                    input->isFullscreen = !input->isFullscreen;
                    window_updateFullscreen(ctx);
                }

                if (nk_button_label(nk,
                                    input->showGBuffer ? "Finales Bild" : "Debug Bilder"))
                {
                    input->showGBuffer = !input->showGBuffer;
                }

                if (nk_button_label(nk, "Shader re-rendern"))
                {
                    nk_layout_row_dynamic(nk, 15, 1);
                    rendering_reRenderShaders(ctx);
                }

                nk_tree_pop(nk);
            }

            // Einstellungen bezüglich der Darstellung.
            if (nk_tree_push(nk, NK_TREE_TAB, "Darstellung", NK_MINIMIZED))
            {
                // Wireframe
                nk_layout_row_dynamic(nk, 25, 2);
                nk_bool wireframe = input->showWireframe;
                if (nk_checkbox_label(nk, "Wireframe", &wireframe))
                {
                    input->showWireframe = wireframe;
                }

                // Clear Color
                gui_widgetColor(nk, "Clear Color:", input->rendering.clearColor);

                //Model Scale
                gui_widgetScale(nk, "Model Scale:", &(input->rendering.scale));

                //Model Rotation
                gui_widgetVec3(nk, "Model Rotation:", input->rendering.modelRotation);

                nk_tree_pop(nk);
            }

            // Einstellungen bezüglich der Shader.
            if (nk_tree_push(nk, NK_TREE_TAB, "Light", NK_MINIMIZED))
            {
                //DirLight aktivieren
                nk_bool dirLight = input->rendering.dirLightActive;
                if (nk_checkbox_label(nk, "DirLight (de-)aktivieren", &dirLight))
                {
                    input->rendering.dirLightActive = dirLight;
                }

                //Light Direction
                gui_widgetVec3(nk, "DirLight Direction:", input->rendering.dirLight.direction);
                //Light Color
                gui_widgetLightColor3(nk, "DirLight Color", input->rendering.dirLight.ambient, input->rendering.dirLight.diffuse, input->rendering.dirLight.specular);

                //DirLight aktivieren
                nk_bool pointLight = input->rendering.pointLightActive;
                if (nk_checkbox_label(nk, "Punktlichter (de-)aktivieren", &pointLight))
                {
                    input->rendering.pointLightActive = pointLight;
                }
                nk_tree_pop(nk);
            }

            //Einstellung bezüglich des Mappings
            if (nk_tree_push(nk, NK_TREE_TAB, "Mapping", NK_MINIMIZED))
            {
                //Nomralmapping An/Aus schalten
                nk_bool normalMapping = input->rendering.useNormalMapping;
                if (nk_checkbox_label(nk, "Normalmapping", &normalMapping))
                {
                    input->rendering.useNormalMapping = normalMapping;
                }

                //Parallaxmapping An/Aus schalten
                nk_bool parallaxMapping = input->rendering.useParallax;
                if (nk_checkbox_label(nk, "Parallaxmapping", &parallaxMapping))
                {
                    input->rendering.useParallax = parallaxMapping;
                }
                //Height Scale einstellen
                nk_property_float(nk, "Height Scale", 0.001f, &input->rendering.heightScale, 2.0f, input->rendering.heightScale, 0.001f);

                //Tessellation An/Aus schalten
                nk_bool useTessellation = input->rendering.useTessellation;
                if (nk_checkbox_label(nk, "Manual-Tessellation", &useTessellation))
                {
                    input->rendering.useTessellation = useTessellation;
                    if (useTessellation)
                    {
                        input->rendering.useDistanceTessellation = false;
                    }
                }
                //Innere Tesselation einstellen
                nk_labelf(nk, NK_TEXT_LEFT, "Inner Tessellation:");
                nk_slider_float(nk, 1.0f, &input->rendering.innerTessellation, 50.0f, 1.0f);

                //Aussere Tesselation einstellen
                nk_labelf(nk, NK_TEXT_LEFT, "Outer Tessellation:");
                nk_slider_float(nk, 1.0f, &input->rendering.outerTessellation, 50.0f, 1.0f);

                //Tessellation An/Aus schalten
                nk_bool useDistTessellation = input->rendering.useDistanceTessellation;
                if (nk_checkbox_label(nk, "Distance-Tessellation", &useDistTessellation))
                {
                    input->rendering.useDistanceTessellation = useDistTessellation;
                    if (useDistTessellation)
                    {
                        input->rendering.useTessellation = false;
                    }
                }
                //Tessellation Amount einstellen
                nk_property_float(nk, "LoD:", 1.0f, &input->rendering.tessellationAmount, 128.0f, input->rendering.tessellationAmount, 0.5f);

                //Displacement An/Aus schlaten
                nk_bool displacement = input->rendering.useDisplacement;
                if (nk_checkbox_label(nk, "Displacement", &displacement))
                {
                    input->rendering.useDisplacement = displacement;
                }
                //Displacement Faktor einstellen
                nk_labelf(nk, NK_TEXT_LEFT, "Displacement Factor:");
                nk_slider_float(nk, -0.1f, &input->rendering.displacementFactor, 0.1f, 0.001f);
                nk_tree_pop(nk);
            }
            // Einstellungen bezüglich des Post-Processing.
            if (nk_tree_push(nk, NK_TREE_TAB, "Post-Processing", NK_MINIMIZED))
            {
                //DirLight aktivieren
                nk_bool useBloom = input->rendering.useBloom;
                if (nk_checkbox_label(nk, "Bloom (de-)aktivieren", &useBloom))
                {
                    input->rendering.useBloom = useBloom;
                }
                //Threshhold-Value einstellen
                nk_labelf(nk, NK_TEXT_LEFT, "Threshhold value:");
                nk_slider_float(nk, 0.0f, &input->rendering.threshhold, 10.0f, 0.01f);
                //Color-Weight einstellen
                nk_labelf(nk, NK_TEXT_LEFT, "Bloom color weight:");
                nk_slider_float(nk, 0.0f, &input->rendering.colorWeight, 15.0f, 0.01f);
                //Emission-Weight einstellen
                nk_labelf(nk, NK_TEXT_LEFT, "Bloom emission weight:");
                nk_slider_float(nk, 0.0f, &input->rendering.emissionWeight, 15.0f, 0.01f);
                //Blur-Iterationen einstellen
                nk_labelf(nk, NK_TEXT_LEFT, "Blur Iterationen:");
                nk_slider_int(nk, 0, &input->rendering.blurIterations, 10, 1);
                //Gamma-Value einstellen
                nk_labelf(nk, NK_TEXT_LEFT, "Gamma value:");
                nk_slider_float(nk, 0.0f, &input->rendering.gamma, 10.0f, 0.01f);
                //Exposure-Value einstellen
                nk_labelf(nk, NK_TEXT_LEFT, "Exposure value:");
                nk_slider_float(nk, 0.0f, &input->rendering.exposure, 10.0f, 0.01f);

                nk_tree_pop(nk);
            }
        }
        nk_end(nk);
    }
}

/**
 * Zeigt allgemeine Zustandsinformationen über das Programm an.
 * 
 * @param ctx Programmkontext.
 * @param nk Abkürzung für das GUI Handle.
 */
static void gui_renderStats(ProgContext *ctx, struct nk_context *nk)
{
    InputData *input = ctx->input;
    WindowData *win = ctx->winData;

    // Prüfen, ob das Menü überhaupt angezeigt werden soll.
    if (input->showStats)
    {
        float x = (float)win->realWidth - STATS_WIDTH;

        // Fenster öffnen.
        if (nk_begin(nk, GUI_WINDOW_STATS,
                     nk_rect(x, 0, STATS_WIDTH, STATS_HEIGHT),
                     NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_BACKGROUND |
                         NK_WINDOW_NO_INPUT))
        {
            // FPS Anzeigen
            nk_layout_row_dynamic(nk, 25, 1);
            char fpsString[15];
            snprintf(fpsString, 14, "FPS: %d", win->fps);
            nk_label(nk, fpsString, NK_TEXT_LEFT);
        }
        nk_end(nk);
    }
}

//////////////////////////// ÖFFENTLICHE FUNKTIONEN ////////////////////////////

void gui_setStartingColDir(DirLight *light)
{
    //glm_vec3_copy(light->diffuse, g_DirLightCol);
    glm_vec3_copy(light->direction, g_DirLightDir);
}

void gui_init(ProgContext *ctx)
{
    ctx->gui = malloc(sizeof(GuiData));
    GuiData *data = ctx->gui;

    // Der neu erzeugte Speicher muss zuerst geleert werden.
    memset(&data->glfw, 0, sizeof(struct nk_glfw));

    // Danach muss ein neuer Nuklear-GLFW3 Kontext erzeugt werden.
    data->nk = nk_glfw3_init(
        &data->glfw,
        ctx->window);

    // Nuklear Callbacks registrieren
    glfwSetScrollCallback(ctx->window, callback_glfwScroll);
    glfwSetCharCallback(ctx->window, callback_glfwChar);
    glfwSetMouseButtonCallback(ctx->window, callback_glfwMouseButton);

    // Als nächstes müssen wir einen neuen, leeren Font-Stash erzeugen.
    // Dadurch wird die Default-Font aktiviert.
    struct nk_font_atlas *atlas;
    nk_glfw3_font_stash_begin(&data->glfw, &atlas);
    nk_glfw3_font_stash_end(&data->glfw);
}

void gui_render(ProgContext *ctx)
{
    GuiData *data = ctx->gui;

    // Wir signalisieren Nuklear, das ein neuer Frame gezeichnet wird.
    nk_glfw3_new_frame(&data->glfw);

    // Ab hier können alle unterschiedlichen GUIs aufgebaut werden
    gui_renderHelp(ctx, data->nk);
    gui_renderMenu(ctx, data->nk);
    gui_renderStats(ctx, data->nk);

    // Als letztes rendern wir die GUI
    nk_glfw3_render(
        &data->glfw,
        NK_ANTI_ALIASING_ON,
        MAX_VERTEX_BUFFER,
        MAX_ELEMENT_BUFFER);
}

void gui_cleanup(ProgContext *ctx)
{
    nk_glfw3_shutdown(&ctx->gui->glfw);
    free(ctx->gui);
}
