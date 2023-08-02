/**
 * Diese Datei implementiert alle Single-Header-Libraries (SHL) [1] die in diesem
 * Projekt verwendet werden. Wenn zusätzliche SHLs eingebunden werden, können diese
 * hier ebenfalls implementiert werden.
 * 
 * Würden die SHLs dort implementiert werden, wo sie genutzt werden, würde dies dazu
 * führen, dass bei jeder Änderung an der eigenen Quellcode-Datei auch die Implementierung
 * der Library neu kompiliert werden müsste. Dadurch, dass alle Implementierungen in diese
 * Datei ausgelagert werden, die vergleichsweise sehr selten geändert wird, kann Zeit bei der
 * Compilierung gespart werden.
 * 
 * DIE VERWENDUNG VON ZUSÄTZLICHEN BIBLIOTHEKEN MUSS IM VORRAUS ABGESPROCHEN WERDEN.
 * 
 * [1]: https://nicolashollmann.de/de/blog/single-header-libraries/
 * 
 * Copyright (C) 2020, FH Wedel
 * Autor: Nicolas Hollmann
 */

/* Implementierung von stb_image & stb_image_write */
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <sesp/stb_image.h>

#define STB_DS_IMPLEMENTATION
#include <sesp/stb_ds.h>

/* Implementierung von nuklear */
#define NK_IMPLEMENTATION
#define NK_GLFW_GL3_IMPLEMENTATION
#include <sesp/nuklear.h>
