// TextTheme.hh for Shynebox Window Manager

/*
  Theme for text inside misc objects.
*/

#ifndef TK_TEXTTHEME_HH
#define TK_TEXTTHEME_HH

#include "Theme.hh"
#include "Font.hh"
#include "Color.hh"
#include "Orientation.hh"
#include "GContext.hh"

namespace tk {

class TextTheme {
public:
  TextTheme(Theme &theme, const std::string &name);
  virtual ~TextTheme() { }

  void reconfigTheme();
  void updateTextColor();

  Font &font() { return *m_font; }
  const Font &font() const { return *m_font; }
  const Color &textColor() const { return *m_text_color; }
  Justify justify() const { return *m_justify; }
  GC textGC() const { return m_text_gc.gc(); }
private:
  ThemeItem<Font> m_font;
  ThemeItem<Color> m_text_color;
  ThemeItem<Justify> m_justify;
  GContext m_text_gc;
};

} // end namespace tk

#endif // TK_TEXTTHEME_HH

// Copyright (c) 2023 Shynebox - zlice
//
// Copyright (c) 2003 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
//                and Simon Bowden    (rathnor at users.sourceforge.net)
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
