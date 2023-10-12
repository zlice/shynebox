// SbDrawable.hh for Shynebox Window Manager

/*
  Basic drawing functions for X drawables.
*/

#ifndef TK_SBDRAWABLE_HH
#define TK_SBDRAWABLE_HH

#include <X11/Xlib.h>

namespace tk {

class SbDrawable {
public:
  SbDrawable();
  virtual ~SbDrawable() { }
  virtual void copyArea(Drawable src, GC gc,
                        int src_x, int src_y,
                        int dest_x, int dest_y,
                        unsigned int width, unsigned int height);

  virtual void fillRectangle(GC gc, int x, int y,
                             unsigned int width, unsigned int height);

  virtual void drawRectangle(GC gc, int x, int y,
                             unsigned int width, unsigned int height);

  virtual void drawLine(GC gc, int start_x, int start_y,
                        int end_x, int end_y);
  virtual void fillPolygon(GC gc, XPoint *points, int npoints,
                           int shape, int mode);

  // type of arrow that should be drawn
  enum TriangleType {
      LEFT,
      RIGHT,
      UP,
      DOWN
  };

  // x, y, width and height define a space to draw a triangle
  // scale defines number of triangles that'd fit in a space of 100 width x 100 height
  // (i.e. 200 = half size, 300 = a third).
  // this is used for Shade caption button and menu bullets
  virtual void drawTriangle(GC gc, TriangleType type, int x, int y,
               unsigned int width, unsigned int height, int scale);

  virtual XImage *image(int x, int y, unsigned int width, unsigned int height) const;

  // X drawable
  virtual Drawable drawable() const = 0;
  virtual unsigned int width() const = 0;
  virtual unsigned int height() const = 0;
  virtual unsigned int depth() const = 0;
  static Display *display() { return s_display; }
protected:
  static Display *s_display; // display connection
};

} // end namespace tk

#endif // TK_SBDRAWABLE_HH

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
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
