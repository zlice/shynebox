// BorderTheme.hh for Shynebox Window Manager

/*
  Theme for borders around windows.
*/

#ifndef TK_BORDERTHEME_HH
#define TK_BORDERTHEME_HH

#include "Theme.hh"
#include "Color.hh"

namespace tk {

// helper class for border theme items
class BorderTheme {
public:
  BorderTheme(Theme &theme, const std::string &name);
  virtual ~BorderTheme() { }

  int width() const { return *m_width; }
  const Color &color() const { return *m_color; }

private:
  ThemeItem<int> m_width;
  ThemeItem<Color> m_color;
};

} // end namespace tk

#endif // TK_BORDERTHEME_HH

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
