/**
 * Modul für das Laden und Schreiben von Texturen.
 * 
 * Copyright (C) 2020, FH Wedel
 * Autor: Nicolas Hollmann
 */

#include "texture.h"

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sesp/stb_image.h>

#include "utils.h"

// Wir prüfen ersteinaml, ob die Extension überhaupt gesetzt ist. Das heißt
// nicht, dass sie geladen wurde, nur dass sie überhaupt definiert ist.
#ifndef GL_COMPRESSED_RGBA_S3TC_DXT3_EXT
#error "The GL_EXT_texture_compression_s3tc extension must be active!"
#endif

////////////////////////////////// KONSTANTEN //////////////////////////////////

// DDS Konstanten
#define FOURCC_DXT1 0x31545844 //(MAKEFOURCC('D','X','T','1'))
#define FOURCC_DXT3 0x33545844 //(MAKEFOURCC('D','X','T','3'))
#define FOURCC_DXT5 0x35545844 //(MAKEFOURCC('D','X','T','5'))
#define FOURCC_ATI2 0x32495441 //(MAKEFOURCC('A','T','I','2'))

// Maximale Länge des Screenshot-Dateinamens
#define SCREENSHOT_FILENAME_SIZE 40

////////////////////////////// LOKALE DATENTYPEN ///////////////////////////////
// DDS Pixelformat
typedef struct
{
    int dwSize;
    int dwFlags;
    int dwFourCC;
    int dwRGBBitCount;
    int dwRBitMask;
    int dwGBitMask;
    int dwBBitMask;
    int dwABitMask;
} DDS_PIXELFORMAT;

// DDS Header Format
typedef struct
{
    int dwSize;
    int dwFlags;
    int dwHeight;
    int dwWidth;
    int dwLinearSize;
    int dwDepth;
    int dwMipMapCount;
    int dwReserved1[11];
    DDS_PIXELFORMAT ddpfPixelFormat;
    int dwCaps1;
    int dwCaps2;
    int dwReserved2[3];
} DDSURFACEDESC2;

typedef struct
{
    GLuint textureId;
    char *filename;
} TextureCacheEntry;

typedef struct
{
    TextureCacheEntry *data;
    int count;
} TextureCache;

TextureCache g_textureCache = {NULL, 0};
////////////////////////////// LOKALE FUNKTIONEN ///////////////////////////////

/**
 * Lädt eine DDS Textur aus einer Datei.
 * Diese Funktion modifiziert das übergebene Textur-Objekt und gibt deshalb
 * nicht zurück.
 * 
 * @param textureId eine valide OpenGL Textur-ID
 * @param filename der Dateiname aus der die Bilddaten geladen werden sollen
 */
static void texture_loadFromDDS(GLuint textureId, const char *filename, GLboolean diffuse)
{
    // Zuerst prüfen wir, ob die DDS Extension überhaupt geladen werden
    // konnte. Wenn nicht liegt dies an der fehlenden Treiberunterstützung und
    // wir können nichts dagegen tun außer eine Fehlermeldung auszugeben.
    if (!GLAD_GL_EXT_texture_compression_s3tc)
    {
        fprintf(stderr, "Error: No support for DDS textures!\n");
        return;
    }

    // Die Datei zum Lesen öffnen.
    FILE *f = fopen(filename, "rb");
    if (f == NULL)
    {
        fprintf(stderr, "Error: Could not open image file \"%s\"!\n", filename);
        return;
    }

    // Den Datentyp der Datei verifizieren.
    char filecode[4];
    fread(filecode, 1, 4, f);
    if (strncmp(filecode, "DDS ", 4) != 0)
    {
        fprintf(
            stderr,
            "Error: Could not verifiy image file \"%s\"!\n",
            filename);
        fclose(f);
        return;
    }

    // Den Datei-Header auslesen.
    DDSURFACEDESC2 ddsDesc;
    fread(&ddsDesc, sizeof(DDSURFACEDESC2), 1, f);

    // Die benötigte Größe des Datenbuffers feststellen.
    // Wenn Mipmaps verfügbar sind, wird die Größe der Datei doppelt sein.
    size_t bufferSize = ddsDesc.dwLinearSize;
    if (ddsDesc.dwMipMapCount > 1)
    {
        bufferSize *= 2;
    }

    // Den Speicher für die Bilddaten reservieren und diese einlesen.
    unsigned char *data = malloc(bufferSize);
    fread(data, 1, bufferSize, f);

    // Da sie nicht mehr benötigt wird, kann die Datei geschlossen werden.
    fclose(f);

    // Als nächstes muss das Format der Bilddaten bestimmt werden.
    GLenum format;
    switch (ddsDesc.ddpfPixelFormat.dwFourCC)
    {
    case FOURCC_DXT1:
        format = diffuse ? GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT : GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
        break;

    case FOURCC_DXT3:
        format = diffuse ? GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT : GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
        break;

    case FOURCC_DXT5:
        format = diffuse ? GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT : GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
        break;

    case FOURCC_ATI2:
        format = GL_COMPRESSED_RG_RGTC2;
        break;

    default:
        fprintf(
            stderr,
            "Error: Unsupported image format in image file \"%s\"!\n",
            filename);
        free(data);
        return;
    }

    // Das neue Textur-Objekt binden/aktivieren.
    glBindTexture(GL_TEXTURE_2D, textureId);

    // Als nächstes extrahieren wir relevante Informationen, um die
    // Textur und die Mipmaps an OpenGL zu übergeben.
    GLsizei blockSize = (format == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT) || (format == GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT) ? 8 : 16;
    GLsizei width = ddsDesc.dwWidth;
    GLsizei height = ddsDesc.dwHeight;
    unsigned int offset = 0;

    // In dieser Schleife wird die Textur und alle Mipmaps an OpenGL übergeben,
    for (
        int level = 0;
        (level <= ddsDesc.dwMipMapCount) && (width || height);
        level++)
    {
        // Verhindern, dass nur width oder nur height 0 wird.
        width = utils_maxInt(width, 1);
        height = utils_maxInt(height, 1);

        // Die Größe der Daten bestimmen und diese an OpenGL übergeben.
        GLsizei size = ((width + 3) / 4) * ((height + 3) / 4) * blockSize;
        glCompressedTexImage2D(
            GL_TEXTURE_2D, // Das Ziel
            level,         // Das zu setzende Mipmap Level
            format,        // Das interne Datenformat
            width, height, // Die Bildgröße
            0,             // "border" muss laut Dokumentation auf 0 stehen
            size,          // Die Größe der komprimierten Daten
            data + offset  // Ein Zeiger auf die Daten ab dem Offset
        );

        // Den Offset verschieben und die Größe anpassen.
        offset += size;
        width /= 2;
        height /= 2;
    }

    // Wenn nötig, automatisch die Mipmaps erstellen lassen.
    if (ddsDesc.dwMipMapCount <= 1)
    {
        glGenerateMipmap(GL_TEXTURE_2D);
    }

    // Zum Schluss muss noch der Speicher für die Bilddaten freigegeben werden.
    free(data);
}

/**
 * Lädt eine Textur aus einer Datei (aber nicht DDS).
 * Diese Funktion modifiziert das übergebene Textur-Objekt und gibt deshalb
 * nicht zurück.
 * 
 * @param textureId eine valide OpenGL Textur-ID
 * @param filename der Dateiname aus der die Bilddaten geladen werden sollen
 */
static void texture_loadFromImage(GLuint textureId, const char *filename, GLboolean diffuse)
{
    // Wir aktivieren vertikales Spiegeln für das Laden von Bildern.
    stbi_set_flip_vertically_on_load(true);

    // Dann laden wir die Textur aus der angegebenen Datei.
    int width, height, channels;
    unsigned char *data = stbi_load(filename, &width, &height, &channels, 0);
    if (!data)
    {
        fprintf(stderr, "Error: Could not read image file \"%s\"!\n", filename);
        return;
    }

    // Als nächstes bestimmen wir das OpenGL Bilddatenformat anhand der Anzahl
    // der Kanäle.
    GLenum format;
    switch (channels)
    {
    case 1:
        format = GL_RED;
        break;

    case 2:
        format = GL_RG;
        break;

    case 3:
        format = diffuse ? GL_SRGB : GL_RGB;
        break;

    case 4:
        format = diffuse ? GL_SRGB_ALPHA : GL_RGBA;
        break;

    default:
        fprintf(
            stderr,
            "Error: Unsupported num. of channels (%d) in image file \"%s\"!\n",
            channels, filename);
        stbi_image_free(data);
        return;
    }

    // Das neue Textur-Objekt binden/aktivieren.
    glBindTexture(GL_TEXTURE_2D, textureId);

    // Die Texturdaten an OpenGL übergeben.
    glTexImage2D(
        GL_TEXTURE_2D,    // Das Ziel
        0,                // Das zu setzende Mipmap Level
        format,           // Das interne Datenformat
        width, height,    // Die Bildgröße
        0,                // "border" muss laut Dokumentation auf 0 stehen
        format,           // Das Format der übergebenen Pixeldaten
        GL_UNSIGNED_BYTE, // Der Datentyp der übergebenen Daten
        data              // Die Bilddaten
    );

    // Automatisch die Mipmaps erstellen lassen.
    glGenerateMipmap(GL_TEXTURE_2D);

    // Zum Schluss müssen die geladenen Bilddaten wieder freigegeben.
    // OpenGL hat selbst eine Kopie der Daten angelegt.
    stbi_image_free(data);
}

//////////////////////////// ÖFFENTLICHE FUNKTIONEN ////////////////////////////

void deleteTextureCache(){
    if(g_textureCache.count > 0){
        for (GLint i = 0; i < g_textureCache.count; i++)
        {
            free(g_textureCache.data[i].filename);
        }
        free(g_textureCache.data);
        g_textureCache.data = NULL;
        g_textureCache.count = 0;
    }
}

GLuint texture_loadTexture(const char *filename, GLenum wrapping, GLboolean diffuse)
{
    for (GLint i = 0; i < g_textureCache.count; i++)
    {
        if (strcmp(g_textureCache.data[i].filename, filename) == 0)
        {
            return g_textureCache.data[i].textureId;
        }
    }

    // Zuerst erstellen wir ein Textur-Objekt, damit wir immer eine valide
    // ID zurückgeben können.
    GLuint textureId;
    glGenTextures(1, &textureId);

    // Danach muss geprüft werden, ob eine DDS Datei oder ein anderes Format
    // vorliegt, da DDS Dateien anders geladen werden müssen.
    if (utils_hasSuffix(filename, ".dds"))
    {
        texture_loadFromDDS(textureId, filename, diffuse);
    }
    else
    {
        texture_loadFromImage(textureId, filename, diffuse);
    }

    // Wir stellen noch einmal sicher, dass die Textur auch gebunden ist.
    // Eigentlich sollte sie bereits in den Ladefunktionen gebunden worden sein.
    // Wenn jedoch ein Fehler aufgetreten ist, findet das Binden nicht statt.
    glBindTexture(GL_TEXTURE_2D, textureId);

    // Danach stellen wir ein, welcher Texture-Wrapping Modus verwendet werden
    // soll. Dieser findet verwendung, wenn Texturdaten an Koordinaten
    // ausgelesen werden, die außerhalb von 0 und 1 liegen.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapping);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapping);

    // Desweiteren setzen wir die Filter für weit entfernte und nahe Ansichten.
    // GL_LINEAR heißt, dass zwischen den Farbwerten interpoliert werden soll.
    // Wir benutzen diesen Modus, wenn die Textur größer als möglich angezeigt
    // wird.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Wenn die Textur kleiner angezeigt wird, verwenden wir Mipmaps.
    // Das sind spezielle verkleinerte Texturen. Explizit verwenden wir den
    // Modus GL_LINEAR_MIPMAP_LINEAR. Dieser interpoliert zwischen den beiden
    // Mipmaps, die am besten passen, und interpoliert dann nochmal den
    // korrekten Farbwert.
    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_MIN_FILTER,
        GL_LINEAR_MIPMAP_LINEAR);

    g_textureCache.count++;
    g_textureCache.data = realloc(g_textureCache.data, sizeof(TextureCacheEntry) * g_textureCache.count);
    g_textureCache.data[g_textureCache.count - 1].textureId = textureId;
    g_textureCache.data[g_textureCache.count - 1].filename = malloc(strlen(filename) + 1);
    strcpy(g_textureCache.data[g_textureCache.count - 1].filename, filename);

    return textureId;
}

void texture_deleteTexture(GLuint textureId)
{
    // Aktuell ist die Funktion nur ein Wrapper um die native OpenGL Funktion.
    // Dadurch entsteht aber ein einheitliches Interface. Außerdem kann diese
    // Funktion später auch genutzt werden, um zusätzliche Ressourcen frei zu
    // geben (z.B. das entfernen der Textur aus einem Cache).
    glDeleteTextures(1, &textureId);
}

void texture_saveScreenshot(ProgContext *ctx)
{
    // Wir brauchen die Größe des Framebuffers.
    int width = ctx->winData->width;
    int height = ctx->winData->height;

    // Als nächstes muss der notwendige Speicher für die Bilddaten reserviert
    // werden.
    char *imageData = malloc(width * height * 3);

    // Die folgende Anweisung entfernt ein mögliches Padding der Daten.
    glPixelStorei(GL_PACK_ALIGNMENT, 1);

    // Mit glReadPixels können wir den aktiven Framebuffer auslesen.
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, imageData);

    // Als nächstes muss der Dateiname des neuen Screenshots bestimmt werden.
    char filename[SCREENSHOT_FILENAME_SIZE];
    time_t now = time(NULL);
    strftime(
        filename,
        SCREENSHOT_FILENAME_SIZE - 1,
        "screenshot_%Y-%m-%d_%H-%M-%S.png",
        localtime(&now));

    // Wir aktivieren hier vertikales Spiegeln, falls eine andere Funktion es
    // zuvor deaktiviert hat. Die Spiegelung ist nötig, da OpenGL ein anderes
    // Koordinatensystem als PNG bzw. stb_image_write verwendet.
    stbi_flip_vertically_on_write(true);

    // Als nächstes schreiben wir die Bilddaten. Sollte es zu einem Fehler
    // kommen, wird eine Fehlermeldung ausgegeben.
    if (!stbi_write_png(filename, width, height, 3, imageData, 0))
    {
        fprintf(stderr, "Error on saving screenshot: Could not write file!");
    }

    // Zum Schluss muss noch der belegte Speicher wieder freigegeben werden.
    free(imageData);
}

static void load_cube_map_side(GLenum side_target, const char *file_name, GLuint *texture)
{
    glBindTexture(GL_TEXTURE_CUBE_MAP, *texture);

    int x, y, n;
    int force_channels = 4;
    unsigned char *image_data = stbi_load(
        file_name, &x, &y, &n, force_channels);
    if (!image_data)
    {
        fprintf(stderr, "ERROR: could not load %s\n", file_name);
        return;
    }
    // non-power-of-2 dimensions check
    if ((x & (x - 1)) != 0 || (y & (y - 1)) != 0)
    {
        fprintf(stderr,
                "WARNING: image %s is not power-of-2 dimensions\n",
                file_name);
    }

    // copy image data into 'target' side of cube map
    glTexImage2D(
        side_target,
        0,
        GL_RGBA,
        x,
        y,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        image_data);
    free(image_data);
}

void texture_create_cube_map(
    const char *front,
    const char *back,
    const char *top,
    const char *bottom,
    const char *left,
    const char *right,
    GLuint *texture)
{
    // generate a cube-map texture to hold all the sides
    glActiveTexture(GL_TEXTURE0);
    glGenTextures(1, texture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, *texture);

    // load each image and copy into a side of the cube-map texture
    load_cube_map_side(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, front, texture);
    load_cube_map_side(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, back, texture);
    load_cube_map_side(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, top, texture);
    load_cube_map_side(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, bottom, texture);
    load_cube_map_side(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, left, texture);
    load_cube_map_side(GL_TEXTURE_CUBE_MAP_POSITIVE_X, right, texture);
    // format cube map texture
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}
