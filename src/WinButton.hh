// WinButton.hh for Shynebox Window Manager

/*
  Caption buttons for title of window frames.
  (close, maximize, minimize, sticky, shade, menu)
*/

#ifndef WINBUTTON_HH
#define WINBUTTON_HH

#include "tk/Button.hh"
#include "tk/SbPixmap.hh"

class ShyneboxWindow;
class WinButtonTheme;

namespace tk{
class Color;
template <class T> class ThemeProxy;
}

class WinButton:public tk::Button {
public:
  // draw type for the button
  enum Type {
      MAXIMIZE,
      MINIMIZE,
      SHADE,
      STICK,
      CLOSE,
      MENUICON,
  };

  WinButton(ShyneboxWindow &listen_to,
            tk::ThemeProxy<WinButtonTheme> &theme,
            tk::ThemeProxy<WinButtonTheme> &pressed,
            Type buttontype, const tk::SbWindow &parent, int x, int y,
            unsigned int width, unsigned int height);

  // override for drawing
  void exposeEvent(XExposeEvent &event);
  void buttonReleaseEvent(XButtonEvent &event);
  void setBackgroundPixmap(Pixmap pm);
  void setPressedPixmap(Pixmap pm);
  void setBackgroundColor(const tk::Color &color);
  void setPressedColor(const tk::Color &color);
  // override for redrawing
  void clear();
  void updateAll();

  Pixmap getBackgroundPixmap() const { return getPixmap(m_theme); }
  Pixmap getPressedPixmap() const { return getPixmap(m_pressed_theme); }

private:
  void drawType();
  Pixmap getPixmap(const tk::ThemeProxy<WinButtonTheme> &) const;
  Type m_type;
  ShyneboxWindow &m_listen_to;
  tk::ThemeProxy<WinButtonTheme> &m_theme, &m_pressed_theme;

  tk::SbPixmap m_icon_pixmap;
  tk::SbPixmap m_icon_mask;

  bool overrode_bg, overrode_pressed;
};

#endif // WINBUTTON_HH

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
