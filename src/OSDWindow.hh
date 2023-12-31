// OSDWindow.hh for Shynebox Window Manager

/*
  'On Screen Display Window'
  Shows information on top of other windows.
  Main use is for TooltipWindow and
  Screen 'geometry/position' of current windows.
*/

#ifndef OSDWINDOW_HH
#define OSDWINDOW_HH

#include "tk/SbWindow.hh"

class BScreen;
class SbWinFrameTheme;

namespace tk {
template <class T> class ThemeProxy;
class BiDiString;
}

class OSDWindow: public tk::SbWindow {
public:
  OSDWindow(const tk::SbWindow &parent, BScreen &screen,
            tk::ThemeProxy<SbWinFrameTheme> &theme):
      tk::SbWindow(parent, 0, 0, 10, 10, 0, false, true),
      m_screen(screen), m_theme(theme),
      m_pixmap(None), m_visible(false) { }

  void reconfigTheme();
  void resizeForText(const tk::BiDiString &text);
  void showText(const tk::BiDiString &text);
  void hide();

  bool isVisible() const { return m_visible; }
  BScreen &screen() const { return m_screen; }
  tk::ThemeProxy<SbWinFrameTheme> &theme() { return m_theme; }
protected:
  // Force visible status, use with care.
  void setVisible(bool visible) {
    m_visible = visible;
  }

private:
  void show();

  BScreen &m_screen;
  tk::ThemeProxy<SbWinFrameTheme> &m_theme;
  Pixmap m_pixmap;
  bool m_visible;
};

#endif // OSDWINDOW_HH

// Copyright (c) 2023 Shynebox - zlice
//
// Copyright (c) 2008 Fluxbox Team (fluxgen at fluxbox dot org)
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
