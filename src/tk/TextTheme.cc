// TextTheme.cc for Shynebox Window Manager

#include "TextTheme.hh"
#include "App.hh"

#include <X11/Xlib.h>

namespace tk {

TextTheme::TextTheme(Theme &theme, const std::string &name):
      m_font(theme, name + ".font"),
      m_text_color(theme, name + ".textColor"),
      m_justify(theme, name + ".justify"),
      m_text_gc(RootWindow(App::instance()->display(), theme.screenNum() ) ) {
  *m_justify = LEFT;
  m_text_color->setFromString("white", theme.screenNum() );
  updateTextColor();
} // TextTheme class init

void TextTheme::updateTextColor() {
  m_text_gc.setForeground(*m_text_color);
}

} // end namespace tk

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
