// ToolTheme.hh for Shynebox Window Manager

/*
  Theme intermediate for all Toolbar related items.
*/

#ifndef TOOLTHEME_HH
#define TOOLTHEME_HH

#include "tk/TextTheme.hh"
#include "tk/BorderTheme.hh"
#include "tk/Texture.hh"

/// Handles toolbar item theme for text and texture
class ToolTheme: public tk::Theme, public tk::TextTheme,
                 public tk::ThemeProxy<ToolTheme> {
public:
  ToolTheme(int screen_num, const std::string &name);
  virtual ~ToolTheme();

  bool fallback(tk::ThemeItem_base &item);

  void reconfigTheme();

  const tk::Texture &texture() const { return *m_texture; }
  const tk::BorderTheme &border() const { return m_border; }

  virtual ToolTheme &operator *() { return *this; }
  virtual const ToolTheme &operator *() const { return *this; }

protected:
  tk::ThemeItem<tk::Texture> &textureTheme() { return m_texture; }

private:
  tk::ThemeItem<tk::Texture> m_texture;
  tk::BorderTheme m_border;
};

#endif // TOOLTHEME_HH

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
