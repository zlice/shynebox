// ClockTool.hh for Shynebox Window Manager

/*
  Toolbar item for the clock.
  Also holds a menu and command to edit the time format.
*/

#ifndef CLOCKTOOL_HH
#define CLOCKTOOL_HH

#include "ToolbarItem.hh"
#include "tk/TextButton.hh"
#include "tk/Config.hh"
#include "tk/Timer.hh"

class ToolTheme;
class BScreen;

namespace tk {
class ImageControl;
class Menu;
template <class T> class ThemeProxy;
class StringConvertor;
}

class ClockTool:public ToolbarItem {
public:
  ClockTool(const tk::SbWindow &parent, tk::ThemeProxy<ToolTheme> &theme,
            BScreen &screen, tk::Menu &menu);
  virtual ~ClockTool();

  void move(int x, int y);
  void resize(unsigned int width, unsigned int height);
  void moveResize(int x, int y,
                  unsigned int width, unsigned int height);

  void show();
  void hide();
  void setTimeFormat(const std::string &format);
  // accessors
  unsigned int width() const;
  unsigned int height() const;
  unsigned int borderWidth() const;
  const std::string &timeFormat() const { return m_timeformat; }

  void setOrientation(tk::Orientation orient);

  void parentMoved() { m_button.parentMoved(); }

private:
  void updateTime();
  void themeReconfigured();
  void renderTheme();
  void reRender();
  void updateSizing();

  tk::TextButton                    m_button;

  const tk::ThemeProxy<ToolTheme>&  m_theme;
  BScreen&                          m_screen;
  Pixmap                            m_pixmap;
  tk::Timer                         m_timer;

  std::string                       &m_timeformat; // config item
  tk::StringConvertor               *m_stringconvertor = 0;
};

#endif // CLOCKTOOL_HH

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
