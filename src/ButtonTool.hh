// ButtonTool.hh for Shynebox Window Manager

/*
  This is for customizable buttons to put in the toolbar.
  Used in ToolFactory
*/

#ifndef BUTTONTOOL_HH
#define BUTTONTOOL_HH

#include "tk/NotCopyable.hh"
#include "ToolbarItem.hh"

#include <X11/Xlib.h>

class ButtonTheme;
class ToolTheme;

namespace tk {
class Button;
class ImageControl;
class SbWindow;
template <class T> class ThemeProxy;
}

class ButtonTool: public ToolbarItem, private tk::NotCopyable {
public:
  ButtonTool(tk::Button *button, ToolbarItem::Type type,
             tk::ThemeProxy<ButtonTheme> &theme,
             tk::ImageControl &img_ctrl);
  virtual ~ButtonTool();
  void setOrientation(tk::Orientation orient);

  // old generictool
  void move(int x, int y);
  void resize(unsigned int x, unsigned int y);
  void moveResize(int x, int y,
                  unsigned int width, unsigned int height);
  void show();
  void hide();

  unsigned int width() const;
  unsigned int height() const;
  unsigned int borderWidth() const;

  void parentMoved();

  const tk::ThemeProxy<ToolTheme> &theme() const { return m_theme; }
  tk::SbWindow &window() { return *m_window; }
  const tk::SbWindow &window() const { return *m_window; }

protected:
  void renderTheme();
  void updateSizing();
  Pixmap m_cache_pm, m_cache_pressed_pm;
  tk::ImageControl &m_image_ctrl;

private:
  void themeReconfigured();

  tk::SbWindow *m_window = 0;
  tk::ThemeProxy<ToolTheme> &m_theme;
};

#endif // BUTTONTOOL_HH

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
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

