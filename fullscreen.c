#include "fullscreen.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/Xft/Xft.h>  // Add Xft headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

const char* message = "BATTERY CRITICALLY LOW";

int fullscreen() {
    Display *display;
    Window window;
    XEvent event;
    int screen;
    GC gc;

    /* Open display */
    display = XOpenDisplay(NULL);
    if (display == NULL) {
        fprintf(stderr, "ERROR: fullscreen: Cannot open display\n");
        return 1;
    }

    screen = DefaultScreen(display);

    /* Get screen dimensions */
    int screen_width = DisplayWidth(display, screen);
    int screen_height = DisplayHeight(display, screen);

    /* Create window at full screen size */
    window = XCreateSimpleWindow(display, RootWindow(display, screen),
                                0, 0, screen_width, screen_height, 0,
                                BlackPixel(display, screen), 
                                0xFF0000); /* Red background */

    /* Set window name and class */
    XStoreName(display, window, "Critical Warning");

    XClassHint class_hint;
    class_hint.res_name = "critical_warning";
    class_hint.res_class = "CriticalWarning";
    XSetClassHint(display, window, &class_hint);

    /* Make the window fullscreen */
    Atom wm_state = XInternAtom(display, "_NET_WM_STATE", False);
    Atom fullscreen = XInternAtom(display, "_NET_WM_STATE_FULLSCREEN", False);

    XEvent fullscreen_event;
    memset(&fullscreen_event, 0, sizeof(fullscreen_event));

    fullscreen_event.type = ClientMessage;
    fullscreen_event.xclient.window = window;
    fullscreen_event.xclient.message_type = wm_state;
    fullscreen_event.xclient.format = 32;
    fullscreen_event.xclient.data.l[0] = 1;  // _NET_WM_STATE_ADD
    fullscreen_event.xclient.data.l[1] = fullscreen;
    fullscreen_event.xclient.data.l[2] = 0;

    /* Select inputs */
    XSelectInput(display, window, ExposureMask | KeyPressMask);

    /* Create GC for basic X11 operations */
    gc = XCreateGC(display, window, 0, 0);

    /* Initialize Xft drawing */
    Visual *visual = DefaultVisual(display, screen);
    Colormap colormap = DefaultColormap(display, screen);

    /* Create Xft font */
    XftFont *xft_font = XftFontOpen(display, screen,
                                   XFT_FAMILY, XftTypeString, "Liberation Mono",
                                   XFT_WEIGHT, XftTypeInteger, XFT_WEIGHT_BOLD,
                                   XFT_SIZE, XftTypeDouble, 64.0,
                                   NULL);

    if (!xft_font) {
        fprintf(stderr, "ERROR: fullscreen: Cannot load Liberation Mono font, trying fallback\n");
        xft_font = XftFontOpen(display, screen,
                              XFT_FAMILY, XftTypeString, "monospace",
                              XFT_WEIGHT, XftTypeInteger, XFT_WEIGHT_BOLD,
                              XFT_SIZE, XftTypeDouble, 64.0,
                              NULL);
    }

    /* Create Xft colors */
    XftColor xft_color;
    XRenderColor render_color;
    render_color.red = 0xFFFF;    /* White text (full red component) */
    render_color.green = 0xFFFF;  /* White text (full green component) */
    render_color.blue = 0xFFFF;   /* White text (full blue component) */
    render_color.alpha = 0xFFFF;  /* Fully opaque */

    XftColorAllocValue(display, visual, colormap, &render_color, &xft_color);

    /* Create Xft draw context */
    XftDraw *xft_draw = XftDrawCreate(display, window, visual, colormap);

    /* Map window */
    XMapRaised(display, window);

    /* Send fullscreen event */
    XSendEvent(display, RootWindow(display, screen), False,
               SubstructureNotifyMask | SubstructureRedirectMask,
               &fullscreen_event);

    XFlush(display);

    /* Event loop */
    while (1) {
        XNextEvent(display, &event);

        if (event.type == Expose) {
            if (xft_font) {
                /* Calculate text size for centering */
                XGlyphInfo extents;
                XftTextExtents8(display, xft_font, (XftChar8 *)message, strlen(message), &extents);

                int x = (screen_width - extents.width) / 2;
                int y = (screen_height + extents.height) / 2;

                /* Draw text with Xft for high-quality rendering */
                XftDrawString8(xft_draw, &xft_color, xft_font, x, y,
                               (XftChar8 *)message, strlen(message));
            }
        } else if (event.type == KeyPress) {
            break; /* Exit on any keypress */
        }
    }

    /* Clean up */
    if (xft_font) XftFontClose(display, xft_font);
    XftDrawDestroy(xft_draw);
    XftColorFree(display, visual, colormap, &xft_color);

    XFreeGC(display, gc);
    XDestroyWindow(display, window);
    XCloseDisplay(display);

    return 0;
}
