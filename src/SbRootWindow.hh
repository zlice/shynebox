// SbRootWindow.hh for Shynebox Window Manager

/*
  Sets up root window for Screen
  Disables functions so the window is always where it should be.
*/

#ifndef SBROOTWINDOW_HH
#define SBROOTWINDOW_HH

#include "tk/SbWindow.hh"

class SbRootWindow: public tk::SbWindow {
public:
    explicit SbRootWindow(int screen_num);
    void move(int x, int y) {
      (void) x; (void) y;
    }
    void resize(unsigned int width, unsigned int height) {
      (void) width; (void) height;
    }
    void moveResize(int x, int y, unsigned int width, unsigned int height) {
      (void) x; (void) y; (void) width; (void) height;
    }
    void show() { }
    void hide() { }
    // we should not assign a new window to this
    tk::SbWindow &operator = (Window win) { (void) win; return *this; }
    Visual *visual() const { return m_visual; }
    Colormap colormap() const { return m_colormap; }

    int decorationDepth() const { return m_decorationDepth; }
    Visual *decorationVisual() const { return m_decorationVisual; }
    Colormap decorationColormap() const { return m_decorationColormap; }
    int maxDepth() const { return m_maxDepth; }

private:
    Visual *m_visual;
    Colormap m_colormap;

    int m_decorationDepth;
    Visual *m_decorationVisual;
    Colormap m_decorationColormap;
    int m_maxDepth;
};

#endif // SBROOTWINDOW_HH

// Copyright (c) 2023 Shynebox - zlice
//
// Copyright (c) 2003 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
