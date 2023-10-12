// Font.hh for Shynebox Window Manager

/*
  Base font class for different font types (xmb, xft, xfont)
*/

#ifndef TK_FONT_HH
#define TK_FONT_HH

#include <X11/Xlib.h>

#include <string>
#include "Color.hh"
#include "Orientation.hh"

namespace tk {

class BiDiString;
class FontImp;
class SbDrawable;

class Font {
public:
  static const char DEFAULT_FONT[];
  // called at tk::App destruction time, cleans up cache
  static void shutdown();

  // true if multibyte is enabled, else false
  static bool multibyte();
  // true if utf-8 mode is enabled, else false
  static bool utf8();

  explicit Font(const char* name = DEFAULT_FONT);
  virtual ~Font();

  bool load(const std::string &name);

  void setHalo(bool flag)   { m_halo = flag; if (m_halo) setShadow(false); }
  void setHaloColor(const Color& color) { m_halo_color = color; }

  void setShadow(bool flag) { m_shadow = flag; if (m_shadow) setHalo(false); }
  void setShadowColor(const Color& color) { m_shadow_color = color; }
  void setShadowOffX(int offx) { m_shadow_offx = offx; }
  void setShadowOffY(int offy) { m_shadow_offy = offy; }

  unsigned int textWidth(const char* text, unsigned int size) const;
  unsigned int textWidth(const BiDiString &text) const;

  unsigned int height() const;
  int ascent() const;
  int descent() const;

  // if font handles orientation
  bool validOrientation(tk::Orientation orient);

  void drawText(const SbDrawable &w, int screen, GC gc,
                const char* text, size_t len,
                int x, int y, tk::Orientation orient = ROT0) const;
  void drawText(const SbDrawable &w, int screen, GC gc,
                const BiDiString &text,
                int x, int y, tk::Orientation orient = ROT0) const;

  bool hasShadow() const { return m_shadow; }
  bool hasHalo() const { return m_halo; }

private:
  tk::FontImp* m_fontimp; // font implementation
  std::string m_fontstr;  // font name

  bool m_shadow;          // shadow text
  Color m_shadow_color;   // shadow color
  int m_shadow_offx;      // offset y for shadow
  int m_shadow_offy;      // offset x for shadow
  bool m_halo;            // halo text
  Color m_halo_color;     // halo color
};

} //end namespace tk

#endif //TK_FONT_HH

// Copyright (c) 2023 Shynebox - zlice
//
// Copyright (c) 2002 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
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
