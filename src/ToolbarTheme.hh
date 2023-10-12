// ToolbarTheme.hh for Shynebox Window Manager

/*
  Theme for the Toolbar.
  Will get passed to toolbar items and used for theme-item fallbacks.
*/

#ifndef TOOLBARTHEME_HH
#define TOOLBARTHEME_HH

#include "tk/Theme.hh"
#include "tk/Texture.hh"
#include "tk/BorderTheme.hh"

class ToolbarTheme: public tk::Theme, public tk::ThemeProxy<ToolbarTheme> {
public:
  explicit ToolbarTheme(int screen_num);
  virtual ~ToolbarTheme();

  void reconfigTheme();

  const tk::BorderTheme &border() const { return m_border; }
  const tk::Texture &toolbar() const { return *m_toolbar; }

  bool fallback(tk::ThemeItem_base &item);

  int bevelWidth() const { return *m_bevel_width; }
  bool shape() const { return *m_shape; }
  int height() const { return *m_height; }
  int buttonSize() const { return *m_button_size; }

  virtual ToolbarTheme &operator *() { return *this; }
  virtual const ToolbarTheme &operator *() const { return *this; }

private:
  tk::ThemeItem<tk::Texture> m_toolbar;
  tk::BorderTheme m_border;

  tk::ThemeItem<int> m_bevel_width;
  tk::ThemeItem<bool> m_shape;
  tk::ThemeItem<int> m_height, m_button_size;
};

#endif // TOOLBARTHEME_HH

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
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
