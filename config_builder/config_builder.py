'''
/**
 * @file lv_conf.h
 * Configuration file for v9.0.0-dev
 */

/*
 * Copy this file as `lv_conf.h`
 * 1. simply next to the `lvgl` folder
 * 2. or any other places and
 *    - define `LV_CONF_INCLUDE_SIMPLE`
 *    - add the path as include path
 */

/* clang-format off */
#if 0 /*Set it to "1" to enable content*/

#ifndef LV_CONF_H
#define LV_CONF_H

/*====================
   COLOR SETTINGS
 *====================*/

/*Color depth: 8 (A8), 16 (RGB565), 24 (RGB888), 32 (XRGB8888)*/
#define LV_COLOR_DEPTH 16

/*=========================
   STDLIB WRAPPER SETTINGS
 *=========================*/

/* Possible values
 * - LV_STDLIB_BUILTIN:     LVGL's built in implementation
 * - LV_STDLIB_CLIB:        Standard C functions, like malloc, strlen, etc
 * - LV_STDLIB_MICROPYTHON: MicroPython implementation
 * - LV_STDLIB_RTTHREAD:    RT-Thread implementation
 * - LV_STDLIB_CUSTOM:      Implement the functions externally
 */
#define LV_USE_STDLIB_MALLOC    LV_STDLIB_BUILTIN
#define LV_USE_STDLIB_STRING    LV_STDLIB_BUILTIN
#define LV_USE_STDLIB_SPRINTF   LV_STDLIB_BUILTIN


#if LV_USE_STDLIB_MALLOC == LV_STDLIB_BUILTIN
    /*Size of the memory available for `lv_malloc()` in bytes (>= 2kB)*/
    #define LV_MEM_SIZE (256 * 1024U)          /*[bytes]*/

    /*Size of the memory expand for `lv_malloc()` in bytes*/
    #define LV_MEM_POOL_EXPAND_SIZE 0

    /*Set an address for the memory pool instead of allocating it as a normal array. Can be in external SRAM too.*/
    #define LV_MEM_ADR 0     /*0: unused*/
    /*Instead of an address give a memory allocator that will be called to get a memory pool for LVGL. E.g. my_malloc*/
    #if LV_MEM_ADR == 0
        #undef LV_MEM_POOL_INCLUDE
        #undef LV_MEM_POOL_ALLOC
    #endif
#endif  /*LV_USE_MALLOC == LV_STDLIB_BUILTIN*/

/*====================
   HAL SETTINGS
 *====================*/

/*Default display refresh, input device read and animation step period.*/
#define LV_DEF_REFR_PERIOD  33      /*[ms]*/

/*Default Dot Per Inch. Used to initialize default sizes such as widgets sized, style paddings.
 *(Not so important, you can adjust it to modify default sizes and spaces)*/
#define LV_DPI_DEF 130     /*[px/inch]*/

/*========================
 * RENDERING CONFIGURATION
 *========================*/

/*Align the stride of all layers and images to this bytes*/
#define LV_DRAW_BUF_STRIDE_ALIGN                1

/*Align the start address of draw_buf addresses to this bytes*/
#define LV_DRAW_BUF_ALIGN                       4

#define LV_USE_DRAW_SW 1
#if LV_USE_DRAW_SW == 1
    /* Set the number of draw unit.
     * > 1 requires an operating system enabled in `LV_USE_OS`
     * > 1 means multiply threads will render the screen in parallel */
    #define LV_DRAW_SW_DRAW_UNIT_CNT    1

    /* If a widget has `style_opa < 255` (not `bg_opa`, `text_opa` etc) or not NORMAL blend mode
     * it is buffered into a "simple" layer before rendering. The widget can be buffered in smaller chunks.
     * "Transformed layers" (if `transform_angle/zoom` are set) use larger buffers
     * and can't be drawn in chunks. */

    /*The target buffer size for simple layer chunks.*/
    #define LV_DRAW_SW_LAYER_SIMPLE_BUF_SIZE          (24 * 1024)   /*[bytes]*/

    /* 0: use a simple renderer capable of drawing only simple rectangles with gradient, images, texts, and straight lines only
     * 1: use a complex renderer capable of drawing rounded corners, shadow, skew lines, and arcs too */
    #define LV_DRAW_SW_COMPLEX          1

    #if LV_DRAW_SW_COMPLEX == 1
        /*Allow buffering some shadow calculation.
        *LV_DRAW_SW_SHADOW_CACHE_SIZE is the max. shadow size to buffer, where shadow size is `shadow_width + radius`
        *Caching has LV_DRAW_SW_SHADOW_CACHE_SIZE^2 RAM cost*/
        #define LV_DRAW_SW_SHADOW_CACHE_SIZE 0

        /* Set number of maximally cached circle data.
        * The circumference of 1/4 circle are saved for anti-aliasing
        * radius * 4 bytes are used per circle (the most often used radiuses are saved)
        * 0: to disable caching */
        #define LV_DRAW_SW_CIRCLE_CACHE_SIZE 4
    #endif

    #define  LV_USE_DRAW_SW_ASM     LV_DRAW_SW_ASM_NONE

    #if LV_USE_DRAW_SW_ASM == LV_DRAW_SW_ASM_CUSTOM
        #define  LV_DRAW_SW_ASM_CUSTOM_INCLUDE ""
    #endif
#endif

/* Use Arm-2D on Cortex-M based devices. Please only enable it for Helium Powered devices for now */
#define LV_USE_DRAW_ARM2D 0

/* Use NXP's VG-Lite GPU on iMX RTxxx platforms. */
#define LV_USE_DRAW_VGLITE 0

/* Use NXP's PXP on iMX RTxxx platforms. */
#define LV_USE_DRAW_PXP 0

/* Use Renesas Dave2D on RA  platforms. */
#define LV_USE_DRAW_DAVE2D 0

/* Draw using cached SDL textures*/
#define LV_USE_DRAW_SDL 0

/* Use VG-Lite GPU. */
#define LV_USE_DRAW_VG_LITE 0

#if LV_USE_DRAW_VG_LITE
/* Enbale VG-Lite custom external 'gpu_init()' function */
#define LV_VG_LITE_USE_GPU_INIT 0

/* Enable VG-Lite assert. */
#define LV_VG_LITE_USE_ASSERT 0
#endif

/*=================
 * OPERATING SYSTEM
 *=================*/
/*Select an operating system to use. Possible options:
 * - LV_OS_NONE
 * - LV_OS_PTHREAD
 * - LV_OS_FREERTOS
 * - LV_OS_CMSIS_RTOS2
 * - LV_OS_RTTHREAD
 * - LV_OS_WINDOWS
 * - LV_OS_CUSTOM */
#define LV_USE_OS   LV_OS_NONE

#if LV_USE_OS == LV_OS_CUSTOM
    #define LV_OS_CUSTOM_INCLUDE <stdint.h>
#endif

/*=======================
 * FEATURE CONFIGURATION
 *=======================*/

/*-------------
 * Logging
 *-----------*/

/*Enable the log module*/
#define LV_USE_LOG 0
#if LV_USE_LOG

    /*How important log should be added:
    *LV_LOG_LEVEL_TRACE       A lot of logs to give detailed information
    *LV_LOG_LEVEL_INFO        Log important events
    *LV_LOG_LEVEL_WARN        Log if something unwanted happened but didn't cause a problem
    *LV_LOG_LEVEL_ERROR       Only critical issue, when the system may fail
    *LV_LOG_LEVEL_USER        Only logs added by the user
    *LV_LOG_LEVEL_NONE        Do not log anything*/
    #define LV_LOG_LEVEL LV_LOG_LEVEL_WARN

    /*1: Print the log with 'printf';
    *0: User need to register a callback with `lv_log_register_print_cb()`*/
    #define LV_LOG_PRINTF 0

    /*1: Enable print timestamp;
     *0: Disable print timestamp*/
    #define LV_LOG_USE_TIMESTAMP 1

    /*1: Print file and line number of the log;
     *0: Do not print file and line number of the log*/
    #define LV_LOG_USE_FILE_LINE 1

    /*Enable/disable LV_LOG_TRACE in modules that produces a huge number of logs*/
    #define LV_LOG_TRACE_MEM        1
    #define LV_LOG_TRACE_TIMER      1
    #define LV_LOG_TRACE_INDEV      1
    #define LV_LOG_TRACE_DISP_REFR  1
    #define LV_LOG_TRACE_EVENT      1
    #define LV_LOG_TRACE_OBJ_CREATE 1
    #define LV_LOG_TRACE_LAYOUT     1
    #define LV_LOG_TRACE_ANIM       1
    #define LV_LOG_TRACE_CACHE      1

#endif  /*LV_USE_LOG*/

/*-------------
 * Asserts
 *-----------*/

/*Enable asserts if an operation is failed or an invalid data is found.
 *If LV_USE_LOG is enabled an error message will be printed on failure*/
#define LV_USE_ASSERT_NULL          1   /*Check if the parameter is NULL. (Very fast, recommended)*/
#define LV_USE_ASSERT_MALLOC        1   /*Checks is the memory is successfully allocated or no. (Very fast, recommended)*/
#define LV_USE_ASSERT_STYLE         0   /*Check if the styles are properly initialized. (Very fast, recommended)*/
#define LV_USE_ASSERT_MEM_INTEGRITY 0   /*Check the integrity of `lv_mem` after critical operations. (Slow)*/
#define LV_USE_ASSERT_OBJ           0   /*Check the object's type and existence (e.g. not deleted). (Slow)*/

/*Add a custom handler when assert happens e.g. to restart the MCU*/
#define LV_ASSERT_HANDLER_INCLUDE <stdint.h>
#define LV_ASSERT_HANDLER while(1);   /*Halt by default*/

/*-------------
 * Debug
 *-----------*/

/*1: Draw random colored rectangles over the redrawn areas*/
#define LV_USE_REFR_DEBUG 0

/*1: Draw a red overlay for ARGB layers and a green overlay for RGB layers*/
#define LV_USE_LAYER_DEBUG 0

/*1: Draw overlays with different colors for each draw_unit's tasks.
 *Also add the index number of the draw unit on white background.
 *For layers add the index number of the draw unit on black background.*/
#define LV_USE_PARALLEL_DRAW_DEBUG 0

/*------------------
 * STATUS MONITORING
 *------------------*/

/*1: Show CPU usage and FPS count
 * Requires `LV_USE_SYSMON = 1`*/
#define LV_USE_PERF_MONITOR 0
#if LV_USE_PERF_MONITOR
    #define LV_USE_PERF_MONITOR_POS LV_ALIGN_BOTTOM_RIGHT

    /*0: Displays performance data on the screen, 1: Prints performance data using log.*/
    #define LV_USE_PERF_MONITOR_LOG_MODE 0
#endif

/*1: Show the used memory and the memory fragmentation
 * Requires `LV_USE_BUILTIN_MALLOC = 1`
 * Requires `LV_USE_SYSMON = 1`*/
#define LV_USE_MEM_MONITOR 0
#if LV_USE_MEM_MONITOR
    #define LV_USE_MEM_MONITOR_POS LV_ALIGN_BOTTOM_LEFT
#endif

/*-------------
 * Others
 *-----------*/

/*Maximum buffer size to allocate for rotation.
 *Only used if software rotation is enabled in the display driver.*/
#define LV_DISPLAY_ROT_MAX_BUF (10*1024)

#define LV_ENABLE_GLOBAL_CUSTOM 0
#if LV_ENABLE_GLOBAL_CUSTOM
    /*Header to include for the custom 'lv_global' function"*/
    #define LV_GLOBAL_CUSTOM_INCLUDE <stdint.h>
#endif

/*Default cache size in bytes.
 *Used by image decoders such as `lv_lodepng` to keep the decoded image in the memory.
 *If size is not set to 0, the decoder will fail to decode when the cache is full.
 *If size is 0, the cache function is not enabled and the decoded mem will be released immediately after use.*/
#define LV_CACHE_DEF_SIZE       0

/*Number of stops allowed per gradient. Increase this to allow more stops.
 *This adds (sizeof(lv_color_t) + 1) bytes per additional stop*/
#define LV_GRADIENT_MAX_STOPS   2

/* Adjust color mix functions rounding. GPUs might calculate color mix (blending) differently.
 * 0: round down, 64: round up from x.75, 128: round up from half, 192: round up from x.25, 254: round up */
#define LV_COLOR_MIX_ROUND_OFS  0

/* Add 2 x 32 bit variables to each lv_obj_t to speed up getting style properties */
#define LV_OBJ_STYLE_CACHE      0

/* Add `id` field to `lv_obj_t` */
#define LV_USE_OBJ_ID           0

/* Use lvgl builtin method for obj ID */
#define LV_USE_OBJ_ID_BUILTIN   0

/*Use obj property set/get API*/
#define LV_USE_OBJ_PROPERTY 0

/*=====================
 *  COMPILER SETTINGS
 *====================*/

/*For big endian systems set to 1*/
#define LV_BIG_ENDIAN_SYSTEM 0

/*Define a custom attribute to `lv_tick_inc` function*/
#define LV_ATTRIBUTE_TICK_INC

/*Define a custom attribute to `lv_timer_handler` function*/
#define LV_ATTRIBUTE_TIMER_HANDLER

/*Define a custom attribute to `lv_display_flush_ready` function*/
#define LV_ATTRIBUTE_FLUSH_READY

/*Required alignment size for buffers*/
#define LV_ATTRIBUTE_MEM_ALIGN_SIZE 1

/*Will be added where memories needs to be aligned (with -Os data might not be aligned to boundary by default).
 * E.g. __attribute__((aligned(4)))*/
#define LV_ATTRIBUTE_MEM_ALIGN

/*Attribute to mark large constant arrays for example font's bitmaps*/
#define LV_ATTRIBUTE_LARGE_CONST

/*Compiler prefix for a big array declaration in RAM*/
#define LV_ATTRIBUTE_LARGE_RAM_ARRAY

/*Place performance critical functions into a faster memory (e.g RAM)*/
#define LV_ATTRIBUTE_FAST_MEM

/*Export integer constant to binding. This macro is used with constants in the form of LV_<CONST> that
 *should also appear on LVGL binding API such as Micropython.*/
#define LV_EXPORT_CONST_INT(int_value) struct _silence_gcc_warning /*The default value just prevents GCC warning*/

/*Prefix all global extern data with this*/
#define LV_ATTRIBUTE_EXTERN_DATA

/* Use `float` as `lv_value_precise_t` */
#define LV_USE_FLOAT            0

/*==================
 *   FONT USAGE
 *===================*/

/*Montserrat fonts with ASCII range and some symbols using bpp = 4
 *https://fonts.google.com/specimen/Montserrat*/
#define LV_FONT_MONTSERRAT_8  0
#define LV_FONT_MONTSERRAT_10 0
#define LV_FONT_MONTSERRAT_12 0
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_16 0
#define LV_FONT_MONTSERRAT_18 0
#define LV_FONT_MONTSERRAT_20 0
#define LV_FONT_MONTSERRAT_22 0
#define LV_FONT_MONTSERRAT_24 0
#define LV_FONT_MONTSERRAT_26 0
#define LV_FONT_MONTSERRAT_28 0
#define LV_FONT_MONTSERRAT_30 0
#define LV_FONT_MONTSERRAT_32 0
#define LV_FONT_MONTSERRAT_34 0
#define LV_FONT_MONTSERRAT_36 0
#define LV_FONT_MONTSERRAT_38 0
#define LV_FONT_MONTSERRAT_40 0
#define LV_FONT_MONTSERRAT_42 0
#define LV_FONT_MONTSERRAT_44 0
#define LV_FONT_MONTSERRAT_46 0
#define LV_FONT_MONTSERRAT_48 0

/*Demonstrate special features*/
#define LV_FONT_MONTSERRAT_28_COMPRESSED 0  /*bpp = 3*/
#define LV_FONT_DEJAVU_16_PERSIAN_HEBREW 0  /*Hebrew, Arabic, Persian letters and all their forms*/
#define LV_FONT_SIMSUN_16_CJK            0  /*1000 most common CJK radicals*/

/*Pixel perfect monospace fonts*/
#define LV_FONT_UNSCII_8  0
#define LV_FONT_UNSCII_16 0

/*Optionally declare custom fonts here.
 *You can use these fonts as default font too and they will be available globally.
 *E.g. #define LV_FONT_CUSTOM_DECLARE   LV_FONT_DECLARE(my_font_1) LV_FONT_DECLARE(my_font_2)*/
#define LV_FONT_CUSTOM_DECLARE

/*Always set a default font*/
#define LV_FONT_DEFAULT &lv_font_montserrat_14

/*Enable handling large font and/or fonts with a lot of characters.
 *The limit depends on the font size, font face and bpp.
 *Compiler error will be triggered if a font needs it.*/
#define LV_FONT_FMT_TXT_LARGE 0

/*Enables/disables support for compressed fonts.*/
#define LV_USE_FONT_COMPRESSED 0

/*Enable drawing placeholders when glyph dsc is not found*/
#define LV_USE_FONT_PLACEHOLDER 1

/*=================
 *  TEXT SETTINGS
 *=================*/

/**
 * Select a character encoding for strings.
 * Your IDE or editor should have the same character encoding
 * - LV_TXT_ENC_UTF8
 * - LV_TXT_ENC_ASCII
 */
#define LV_TXT_ENC LV_TXT_ENC_UTF8

/*Can break (wrap) texts on these chars*/
#define LV_TXT_BREAK_CHARS " ,.;:-_)]}"

/*If a word is at least this long, will break wherever "prettiest"
 *To disable, set to a value <= 0*/
#define LV_TXT_LINE_BREAK_LONG_LEN 0

/*Minimum number of characters in a long word to put on a line before a break.
 *Depends on LV_TXT_LINE_BREAK_LONG_LEN.*/
#define LV_TXT_LINE_BREAK_LONG_PRE_MIN_LEN 3

/*Minimum number of characters in a long word to put on a line after a break.
 *Depends on LV_TXT_LINE_BREAK_LONG_LEN.*/
#define LV_TXT_LINE_BREAK_LONG_POST_MIN_LEN 3

/*Support bidirectional texts. Allows mixing Left-to-Right and Right-to-Left texts.
 *The direction will be processed according to the Unicode Bidirectional Algorithm:
 *https://www.w3.org/International/articles/inline-bidi-markup/uba-basics*/
#define LV_USE_BIDI 0
#if LV_USE_BIDI
    /*Set the default direction. Supported values:
    *`LV_BASE_DIR_LTR` Left-to-Right
    *`LV_BASE_DIR_RTL` Right-to-Left
    *`LV_BASE_DIR_AUTO` detect texts base direction*/
    #define LV_BIDI_BASE_DIR_DEF LV_BASE_DIR_AUTO
#endif

/*Enable Arabic/Persian processing
 *In these languages characters should be replaced with an other form based on their position in the text*/
#define LV_USE_ARABIC_PERSIAN_CHARS 0

/*==================
 * WIDGETS
 *================*/

/*Documentation of the widgets: https://docs.lvgl.io/latest/en/html/widgets/index.html*/

#define LV_WIDGETS_HAS_DEFAULT_VALUE  1



#define LV_USE_ARC        1
#define LV_USE_BAR        1
#define LV_USE_BUTTON     1
#define LV_USE_CHART      1
#define LV_USE_CHECKBOX   1
#define LV_USE_IMAGE      1
#define LV_USE_LABEL      1
#define LV_USE_LED        1
#define LV_USE_LINE       1
#define LV_USE_SWITCH     1
#define LV_USE_TABLE      1
#define LV_USE_TILEVIEW   1



dependant on LV_USE_FLEX
LV_USE_BUTTONMATRIX
LV_USE_TABVIEW
LV_USE_MSGBOX
LV_USE_WIN
LV_USE_LIST
LV_USE_MENU


dependant on LV_USE_BUTTON
LV_USE_BUTTONMATRIX
LV_USE_IMAGEBUTTON
LV_USE_TABVIEW
LV_USE_WIN
LV_USE_LIST
LV_USE_MENU


dependant on LV_USE_IMAGE
LV_USE_IMAGEBUTTON
LV_USE_DROPDOWN
LV_USE_WIN
LV_USE_LIST
LV_USE_MENU
LV_USE_ANIMIMG
LV_USE_SCALE
LV_USE_MSGBOX


dependant on LV_USE_BUTTONMATRIX
LV_USE_CALENDAR
LV_USE_KEYBOARD


dependant on LV_USE_TEXTAREA
LV_USE_KEYBOARD
LV_USE_SPINBOX


dependant on LV_USE_LABEL
LV_USE_CANVAS
LV_USE_ROLLER
LV_USE_TEXTAREA
LV_USE_TABVIEW
LV_USE_DROPDOWN
LV_USE_MSGBOX
LV_USE_WIN
LV_USE_LIST
LV_USE_MENU



dependant on LV_USE_LINE
LV_USE_SCALE

dependant on LV_USE_BAR
LV_USE_SLIDER

dependant on LV_USE_SPANGROUP
LV_USE_SPAN


depenant on LV_USE_ARC
LV_USE_SPINNER




-------------------------

dependant on LV_USE_TEXTAREA
    LV_TEXTAREA_DEF_PWD_SHOW_TIME
        default: 1500

-------------------

dependant on LV_USE_SPAN
    LV_SPAN_SNIPPET_STACK_SIZE
        default: 64

----------------------

dependant on LV_USE_LABEL
    LV_LABEL_TEXT_SELECTION
        default: 1
        doc: Enable selecting text of the label

    LV_LABEL_LONG_TXT_HINT
        default: 1
        doc: Store some extra info in labels to speed up drawing of very long texts

    LV_LABEL_WAIT_CHAR_COUNT
        default: 3
        doc: The count of wait chart


-------------------------------

dependant on LV_USE_CALENDAR

    class LV_CALENDAR_WEEK_STARTS_MONDAY:
        """
        """

        __default__ = 0

        class LV_CALENDAR_DEFAULT_DAY_NAMES:
            """
            if LV_CALENDAR_WEEK_STARTS_MONDAY == 1:
                {"Mo", "Tu", "We", "Th", "Fr", "Sa", "Su"}
            else:
                {"Su", "Mo", "Tu", "We", "Th", "Fr", "Sa"}
            """

            __default__ = {"Mo", "Tu", "We", "Th", "Fr", "Sa", "Su"}


    class LV_CALENDAR_DEFAULT_MONTH_NAMES:
        """
        """

        __default__ = '{"January", "February", "March",  "April", "May",  "June", "July", "August", "September", "October", "November", "December"}'


    #if LV_USE_BUTTON == 1 && LV_USE_LABEL == 1

        class LV_USE_CALENDAR_HEADER_ARROW:
            """
            """

            __default__ = 1

    #if LV_USE_DROPDOWM == 1

        class LV_USE_CALENDAR_HEADER_DROPDOWN:
            """
            """

            __default__ = 1

--------------------------------------------


class LV_USE_THEME_DEFAULT:
    """
    A simple, impressive and very complete theme
    """

    __default__ = 1

    class LV_THEME_DEFAULT_DARK:
        """
        0: Light mode; 1: Dark mode
        """

        __default__ = 0

    class LV_THEME_DEFAULT_GROW:
        """
        Enable grow on press
        """

        __default__ = 1

    class LV_THEME_DEFAULT_TRANSITION_TIME:
        """
        Default transition time in [ms]
        """

        __default__ = 80


class LV_USE_THEME_SIMPLE:
    """
    A very simple theme that is a good starting point for a custom theme
    """

    __default__ = 1



class LV_USE_THEME_MONO:
    """
    A theme designed for monochrome displays
    """

    __default__ = 1


class LV_USE_FLEX:
    """
    A layout similar to Flexbox in CSS.
    """

    __default__ = 1


class LV_USE_GRID:
    """
    A layout similar to Grid in CSS.
    """

    __default__ = 1




/*====================
 * 3RD PARTS LIBRARIES
 *====================*/

/*File system interfaces for common APIs */


class LV_USE_FS_STDIO:
    """
    API for fopen, fread, etc
    """

    __default__ = 0

    class LV_FS_STDIO_LETTER:
        """
        Set an upper cased letter on which the drive will accessible (e.g. 'A')
        """
        __default__ = "'\0'"


    class LV_FS_STDIO_PATH:
        """
        Set the working directory. File/directory paths will be appended to it.
        """
        __default__ = '""'


    class LV_FS_STDIO_CACHE_SIZE:
        """
        0 to cache this number of bytes in lv_fs_read()
        """
        __default__ = 0





class LV_USE_FS_POSIX:
    """
    API for open, read, etc
    """

    __default__ = 0

    class LV_FS_POSIX_LETTER:
        """
        Set an upper cased letter on which the drive will accessible (e.g. 'A')
        """
        __default__ = "'\0'"


    class LV_FS_POSIX_PATH:
        """
        Set the working directory. File/directory paths will be appended to it.
        """
        __default__ = '""'


    class LV_FS_POSIX_CACHE_SIZE:
        """
        0 to cache this number of bytes in lv_fs_read()
        """
        __default__ = 0




class LV_USE_FS_WIN32:
    """
    API for CreateFile, ReadFile, etc
    """

    __default__ = 0

    class LV_FS_WIN32_LETTER:
        """
        Set an upper cased letter on which the drive will accessible (e.g. 'A')
        """
        __default__ = "'\0'"


    class LV_FS_WIN32_PATH:
        """
        Set the working directory. File/directory paths will be appended to it.
        """
        __default__ = '""'


    class LV_FS_WIN32_CACHE_SIZE:
        """
        0 to cache this number of bytes in lv_fs_read()
        """
        __default__ = 0




class LV_USE_FS_WIN32:
    """
    API for FATFS (needs to be added separately). Uses f_open, f_read, etc
    """

    __default__ = 0

    class LV_FS_FATFS_LETTER:
        """
        Set an upper cased letter on which the drive will accessible (e.g. 'A')
        """
        __default__ = "'\0'"


    class LV_FS_FATFS_CACHE_SIZE:
        """
        0 to cache this number of bytes in lv_fs_read()
        """
        __default__ = 0





class LV_USE_FS_MEMFS:
    """
    API for memory-mapped file access.
    """

    __default__ = 0

    class LV_FS_MEMFS_LETTER:
        """
        Set an upper cased letter on which the drive will accessible (e.g. 'A')
        """
        __default__ = "'\0'"




class LV_USE_LODEPNG:
    """
    LODEPNG decoder library
    """
    __default__ = 0


class LV_USE_LIBPNG:
    """
    PNG decoder(libpng) library
    """
    __default__ = 0




class LV_USE_LIBPNG:
    """
    PNG decoder(libpng) library
    """
    __default__ = 0

/*BMP decoder library*/
#define LV_USE_BMP 0


class LV_USE_TJPGD:
    """
    JPG + split JPG decoder library.

    Split JPG is a custom format optimized for embedded systems.
    """
    __default__ = 0


class LV_USE_LIBJPEG_TURBO:
    """
    libjpeg-turbo decoder library.

    Supports complete JPEG specifications and high-performance JPEG decoding.
    """
    __default__ = 0



class LV_USE_GIF:
    """
    GIF decoder library
    """
    __default__ = 0

    class LV_GIF_CACHE_DECODE_DATA:
        """
        GIF decoder accelerate
        """
        __default__ = 0


class LV_BIN_DECODER_RAM_LOAD:
    """
    Decode bin images to RAM
    """
    __default__ = 0



class LV_USE_RLE:
    """
    RLE decompress library
    """
    __default__ = 0


class LV_USE_QRCODE:
    """
    QR code library
    """
    __default__ = 0


class LV_USE_BARCODE:
    """
    Barcode library
    """
    __default__ = 0


class LV_USE_BARCODE:
    """
    FreeType library
    """
    __default__ = 0

    class LV_FREETYPE_CACHE_SIZE:
        """
        Memory used by FreeType to cache characters in kilobytes
        """
        __default__ = 768

    class LV_FREETYPE_USE_LVGL_PORT:
        """
        Let FreeType to use LVGL memory and file porting
        """
        __default__ = 0

    class LV_FREETYPE_CACHE_FT_FACES:
        """
        Maximum number of opened FT_Face

        0:use system defaults
        """
        __default__ = 8

    class LV_FREETYPE_CACHE_FT_SIZES:
        """
        Maximum number of opened FT_Size

        0:use system defaults
        """
        __default__ = 8

    class LV_FREETYPE_CACHE_FT_GLYPH_CNT:
        """
        Maximum number of objects managed by this cache instance

        0:use system defaults
        """
        __default__ = 256



class LV_USE_TINY_TTF:
    """
    Built-in TTF decoder
    """
    __default__ = 0

    class LV_TINY_TTF_FILE_SUPPORT:
        """
        Enable loading TTF data from files
        """
        __default__ = 0



class LV_USE_RLOTTIE:
    """
    Rlottie library
    """
    __default__ = 0



class LV_USE_VECTOR_GRAPHIC:
    """
    Enable Vector Graphic APIs
    """
    __default__ = 0




class LV_USE_THORVG_INTERNAL:
    """
    Enable ThorVG (vector graphics library) from the src/libs folder
    """
    __default__ = 0



class LV_USE_THORVG_EXTERNAL:
    """
    Enable ThorVG by assuming that its installed and linked to the project
    """
    __default__ = 0


class LV_USE_LZ4:
    """
    Enable LZ4 compress/decompress lib
    """
    __default__ = 0



class LV_USE_LZ4_INTERNAL:
    """
    Use lvgl built-in LZ4 lib
    """
    __default__ = 0



class LV_USE_LZ4_EXTERNAL:
    """
    Use external LZ4 library
    """
    __default__ = 0




class LV_USE_FFMPEG:
    """
    FFmpeg library for image decoding and playing videos

    Supports all major image formats so do not enable other image decoder with it
    """
    __default__ = 0

    class LV_FFMPEG_DUMP_FORMAT:
        """
        Dump input information to stderr
        """
        __default__ = 0




/*==================
 * OTHERS
 *==================*/


class LV_USE_SNAPSHOT:
    """
    Enable API to take snapshot for object
    """
    __default__ = 0


class LV_USE_SYSMON:
    """
    Enable system monitor component
    """
    __default__ = '(LV_USE_MEM_MONITOR | LV_USE_PERF_MONITOR)'



class LV_USE_PROFILER:
    """
    Enable the runtime performance profiler
    """
    __default__ = 0

    class LV_USE_PROFILER_BUILTIN:
        """
        Enable the built-in profiler
        """
        __default__ = 1

        class LV_PROFILER_BUILTIN_BUF_SIZE:
            """
            Default profiler trace buffer size [bytes]
            """
            __default__ = 16384

    class LV_PROFILER_INCLUDE:
        """
        Header to include for the profiler
        """
        __default__ = '"lvgl/src/misc/lv_profiler_builtin.h"'

    class LV_PROFILER_BEGIN:
        """
        Profiler start point function
        """
        __default__ = 'LV_PROFILER_BUILTIN_BEGIN'

    class LV_PROFILER_END:
        """
        Profiler end point function
        """
        __default__ = 'LV_PROFILER_BUILTIN_END'


    class LV_PROFILER_BEGIN_TAG:
        """
        Profiler start point function with custom tag
        """
        __default__ = 'LV_PROFILER_BUILTIN_BEGIN_TAG'

    class LV_PROFILER_END_TAG:
        """
        Profiler end point function with custom tag
        """
        __default__ = 'LV_PROFILER_BUILTIN_END_TAG'



class LV_USE_MONKEY:
    """
    Enable Monkey test
    """
    __default__ = 0


class LV_USE_GRIDNAV:
    """
    Enable grid navigation
    """
    __default__ = 0


class LV_USE_FRAGMENT:
    """
    Enable lv_obj fragment
    """
    __default__ = 0


class LV_USE_IMGFONT:
    """
    Support using images as font in label or span widgets
    """
    __default__ = 0

    class LV_IMGFONT_PATH_MAX_LEN:
        """
        Imgfont image file path maximum length
        """
        __default__ = 64

    class LV_IMGFONT_USE_IMAGE_CACHE_HEADER:
        """
        Use img cache to buffer header information
        """
        __default__ = 0


class LV_USE_OBSERVER:
    """
    Enable an observer pattern implementation
    """
    __default__ = 1



/*Requires: lv_keyboard*/

class LV_USE_IME_PINYIN:
    """
    Enable Pinyin input method
    """
    __default__ = 0

    class LV_IME_PINYIN_USE_DEFAULT_DICT:
        """
        Use default thesaurus

        If you do not use the default thesaurus, be sure to use `lv_ime_pinyin` after setting the thesauruss
        """
        __default__ = 1

    class LV_IME_PINYIN_CAND_TEXT_NUM:
        """
        Set the maximum number of candidate panels that can be displayed

        This needs to be adjusted according to the size of the screen
        """
        __default__ = 6

    class LV_IME_PINYIN_USE_K9_MODE:
        """
        Use 9 key input(k9)
        """
        __default__ = 1

        class LV_IME_PINYIN_K9_CAND_TEXT_NUM:
            """
            Use 9 key input(k9)
            """
            __default__ = 3




/*Requires: lv_table*/
class LV_USE_FILE_EXPLORER:
    """
    Enable file explorer
    """
    __default__ = 0

    class LV_FILE_EXPLORER_PATH_MAX_LEN:
        """
        Maximum length of path
        """
        __default__ = (128)

    /*Requires: lv_list*/
    class LV_FILE_EXPLORER_QUICK_ACCESS:
        """
        Quick access bar
        """
        __default__ = 1


/*==================
 * DEVICES
 *==================*/



class LV_USE_SDL:
    """
    Use SDL to open window on PC and handle mouse and keyboard
    """
    __default__ = 0

    class LV_SDL_INCLUDE_PATH:
        """
        """
        __default__ = '<SDL2/SDL.h>

    class LV_SDL_RENDER_MODE:
        """
        LV_DISPLAY_RENDER_MODE_DIRECT is recommended for best performance
        """
        __default__ = 'LV_DISPLAY_RENDER_MODE_DIRECT'

    class LV_SDL_BUF_COUNT:
        """
        1 or 2
        """
        __default__ = 1

    class LV_SDL_FULLSCREEN:
        """
        Make the window full screen by default
        """
        __default__ = 0

    class LV_SDL_DIRECT_EXIT:
        """
        Exit the application when all SDL windows are closed
        """
        __default__ = 1




class LV_USE_X11:
    """
    Use X11 to open window on Linux desktop and handle mouse and keyboard
    """
    __default__ = 0

    class LV_X11_DIRECT_EXIT:
        """
        Exit the application when all X11 windows have been closed
        """
        __default__ = 1

    class LV_X11_DOUBLE_BUFFER:
        """
        Use double buffers for endering
        """
        __default__ = 1

    class RenderMode(RadioBox):
        """
        Render Modes
        """

        __default__ = 'LV_X11_RENDER_MODE_PARTIAL'

        class LV_X11_RENDER_MODE_PARTIAL:
            """
            Partial Mode
            """
            __default__ = 0

        class LV_X11_RENDER_MODE_DIRECT:
            """
            Direct Mode
            """
            __default__ = 0

        class LV_X11_RENDER_MODE_FULL:
            """
            Full Mode
            """
            __default__ = 0



class LV_USE_LINUX_FBDEV:
    """
    Driver for /dev/fb
    """
    __default__ = 0

    class LV_LINUX_FBDEV_BSD:
        """
        Exit the application when all X11 windows have been closed
        """
        __default__ = 0

    class LV_LINUX_FBDEV_RENDER_MODE:
        """
        """
        __default__ = 'LV_DISPLAY_RENDER_MODE_PARTIAL'

    class LV_LINUX_FBDEV_BUFFER_COUNT:
        """
        """
        __default__ = 0

    class LV_LINUX_FBDEV_BUFFER_SIZE:
        """
        """
        __default__ = 60



class LV_USE_NUTTX:
    """
    Use Nuttx to open window and handle touchscreen
    """
    __default__ = 0

    class LV_USE_NUTTX_LIBUV:
        """
        """
        __default__ = 0

    class LV_USE_NUTTX_CUSTOM_INIT:
        """
        Use Nuttx custom init API to open window and handle touchscreen
        """
        __default__ = 0

    class LV_USE_NUTTX_LCD:
        """
        Driver for /dev/lcd
        """
        __default__ = 0

        class LV_NUTTX_LCD_BUFFER_COUNT:
            """
            """
            __default__ = 0

        class LV_NUTTX_LCD_BUFFER_SIZE:
            """
            """
            __default__ = 60


    class LV_USE_NUTTX_TOUCHSCREEN:
        """
        Driver for /dev/input
        """
        __default__ = 0



class LV_USE_LINUX_DRM:
    """
    Driver for /dev/dri/card
    """
    __default__ = 0


class LV_USE_TFT_ESPI:
    """
    Interface for TFT_eSPI
    """
    __default__ = 0


class LV_USE_EVDEV:
    """
    Driver for evdev input devices
    """
    __default__ = 0


/*==================
* EXAMPLES
*==================*/

class LV_BUILD_EXAMPLES:
    """
    Enable the examples to be built with the library
    """
    __default__ = 1


/*===================
 * DEMO USAGE
 ====================*/


class LV_USE_DEMO_WIDGETS:
    """
    Show some widget.

    It might be required to increase `LV_MEM_SIZE`
    """
    __default__ = 0

    class LV_DEMO_WIDGETS_SLIDESHOW:
        """
        """
        __default__ = 0


class LV_USE_DEMO_KEYPAD_AND_ENCODER:
    """
    Demonstrate the usage of encoder and keyboard
    """
    __default__ = 0


class LV_USE_DEMO_BENCHMARK:
    """
    Benchmark your system
    """
    __default__ = 0


class LV_USE_DEMO_RENDER:
    """
    Render test for each primitives. Requires at least 480x272 display
    """
    __default__ = 0


class LV_USE_DEMO_STRESS:
    """
    Stress test for LVGL
    """
    __default__ = 0


class LV_USE_DEMO_MUSIC:
    """
    Music player demo
    """
    __default__ = 0

    class LV_DEMO_MUSIC_SQUARE:
        """
        """
        __default__ = 0

    class LV_DEMO_MUSIC_LANDSCAPE:
        """
        """
        __default__ = 0

    class LV_DEMO_MUSIC_ROUND:
        """
        """
        __default__ = 0

    class LV_DEMO_MUSIC_LARGE:
        """
        """
        __default__ = 0

    class LV_DEMO_MUSIC_AUTO_PLAY:
        """
        """
        __default__ = 0


class LV_USE_DEMO_FLEX_LAYOUT:
    """
    Flex layout demo
    """
    __default__ = 0


class LV_USE_DEMO_MULTILANG:
    """
    Smart-phone like multi-language demo
    """
    __default__ = 0


class LV_USE_DEMO_TRANSFORM:
    """
    Widget transformation demo
    """
    __default__ = 0


class LV_USE_DEMO_SCROLL:
    """
    Demonstrate scroll settings
    """
    __default__ = 0


class LV_USE_DEMO_VECTOR_GRAPHIC:
    """
    Vector graphic demo
    """
    __default__ = 0

'''