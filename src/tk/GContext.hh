// GContext for Shynebox Window Manager

/*
  Graphics Context wrapper for X
*/

#ifndef TK_GCONTEXT_HH
#define TK_GCONTEXT_HH

#include "Color.hh"
#include "SbPixmap.hh"

namespace tk {

class SbDrawable;
class Font;

class GContext {
public:
  typedef enum {
    JOINMITER= JoinMiter,
    JOINROUND= JoinRound,
    JOINBEVEL= JoinBevel
  } JoinStyle;

  typedef enum {
    LINESOLID= LineSolid,
    LINEONOFFDASH= LineOnOffDash,
    LINEDOUBLEDASH= LineDoubleDash
  } LineStyle;

  typedef enum {
    CAPNOTLAST= CapNotLast,
    CAPBUTT= CapButt,
    CAPROUND= CapRound,
    CAPPROJECTING= CapProjecting
  } CapStyle;

  // for tk drawable
  explicit GContext(const tk::SbDrawable &drawable);
  // for X drawable
  explicit GContext(Drawable drawable);
  GContext(Drawable d, const tk::GContext &gc);
  virtual ~GContext();

  void setForeground(const tk::Color &color) {
    setForeground(color.pixel() );
  }

  void setForeground(long pixel_value) {
    XSetForeground(m_display, m_gc, pixel_value);
  }

  void setBackground(const tk::Color &color) {
    setBackground(color.pixel() );
  }

  void setBackground(long pixel_value) {
    XSetBackground(m_display, m_gc, pixel_value);
  }

  void setTile(Drawable draw) {
    XSetTile(m_display, m_gc, draw);
  }

  void setTile(const tk::SbPixmap &draw) {
    setTile(draw.drawable() );
  }

  // not implemented
  void setFont(const tk::Font &) {}

  // set font id
  void setFont(int fid) {
    XSetFont(m_display, m_gc, fid);
  }

  void setGraphicsExposure(bool value) {
    XSetGraphicsExposures(m_display, m_gc, value);
  }

  void setFunction(int func) {
    XSetFunction(m_display, m_gc, func);
  }

  void setSubwindowMode(int mode) {
    XSetSubwindowMode(m_display, m_gc, mode);
  }
  void setFillStyle(int style) {
    XSetFillStyle(m_display, m_gc, style);
  }

  void setLineAttributes(unsigned int width,
                                int line_style,
                                int cap_style,
                                int join_style) {
    XSetLineAttributes(m_display, m_gc, width, line_style, cap_style, join_style);
  }


  void copy(GC gc);
  void copy(const GContext &gc);

  GContext &operator = (const GContext &copy_gc) { copy(copy_gc); return *this; }
  GContext &operator = (GC copy_gc) { copy(copy_gc); return *this; }
  GC gc() const { return m_gc; }

private:
  GContext(const GContext &cont);

  static Display *m_display; // worth caching
  GC m_gc;
};

} // end namespace tk

#endif // TK_GCONTEXT_HH

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
