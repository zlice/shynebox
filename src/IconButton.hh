// IconButton.hh for Shynebox Window Manager

/*
  Button for 'icons' (read: 'windows'. Icon means minimized in X).
  Holds a potential icon (image) and title (label).
  Also handles 'tooltip' for hover pop-up descriptions.
*/

#ifndef ICONBUTTON_HH
#define ICONBUTTON_HH

#include "FocusableTheme.hh"

#include "tk/CachedPixmap.hh"
#include "tk/SbPixmap.hh"
#include "tk/TextButton.hh"

class IconbarTheme;

namespace tk {
template <class T> class ThemeProxy;
}

class IconButton: public tk::TextButton {
public:
  IconButton(const tk::SbWindow &parent,
             tk::ThemeProxy<IconbarTheme> &focused_theme,
             tk::ThemeProxy<IconbarTheme> &unfocused_theme,
             Focusable &window);
  virtual ~IconButton();

  void exposeEvent(XExposeEvent &event);
  void enterNotifyEvent(XCrossingEvent &ce);
  void leaveNotifyEvent(XCrossingEvent &ce);
  void clear();
  void clearArea(int x, int y,
                 unsigned int width, unsigned int height,
                 bool exposure = false);
  void moveResize(int x, int y,
                  unsigned int width, unsigned int height);
  void resize(unsigned int width, unsigned int height);

  void reconfigTheme();

  void setPixmap(bool use);

  Focusable &win() { return m_win; }
  const Focusable &win() const { return m_win; }

  void setOrientation(tk::Orientation orient);

  virtual unsigned int preferredWidth() const;
  void showTooltip();

protected:
  void drawText(int x, int y, tk::SbDrawable *drawable_override);

private:
  void reconfigAndClear();
  void setupWindow();

  // Refresh all windows
  // setup will setup window again.
  void refreshEverything(bool setup);

  Focusable &m_win;
  tk::SbWindow m_icon_window;
  tk::SbPixmap m_icon_pixmap;
  tk::SbPixmap m_icon_mask;
  bool m_use_pixmap;
  // if enter notify hit ~this~ is showing its tooltip
  bool m_has_tooltip;
  FocusableTheme<IconbarTheme> m_theme;
  // cached pixmaps
  tk::CachedPixmap m_pm;
};

#endif // ICONBUTTON_HH

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
