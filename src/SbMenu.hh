// SbMenu.hh for Shynebox Window Manager

/*
  Implementation of tk/Menu.
  This is used as a type of 'global' to track the 'current active window'
  for the windowmenu to determine what it is acting on.
  See MenuCreator
*/

#ifndef SBMENU_HH
#define SBMENU_HH

#include "tk/Menu.hh"
#ifndef USE_TOOLBAR
#include "tk/MenuItem.hh"
#endif
#include "tk/LayerItem.hh"
#include "tk/AutoReloadHelper.hh"

class ShyneboxWindow;

namespace tk {
class MenuTheme;
}

class SbMenu: public tk::Menu {
public:
  static void setWindow(ShyneboxWindow *win);
  static ShyneboxWindow *window();

  SbMenu(tk::ThemeProxy<tk::MenuTheme> &tm,
         tk::ImageControl &imgctrl, tk::Layer &layer);
  virtual ~SbMenu() {
    if (m_reloader) {
      delete m_reloader;
      m_reloader = 0;
    }
  }
  void raise() { m_layeritem.raise(); }
  void lower() { m_layeritem.lower(); }
  void buttonPressEvent(XButtonEvent &be);
  void buttonReleaseEvent(XButtonEvent &be);
  void keyPressEvent(XKeyEvent &ke);

  void setReloadHelper(tk::AutoReloadHelper *helper) {
    if (m_reloader)
      delete m_reloader;
    m_reloader = helper;
  }
  tk::AutoReloadHelper *reloadHelper() { return m_reloader; }

private:
  tk::LayerItem m_layeritem;
  tk::AutoReloadHelper *m_reloader = 0;
};

#endif // SBMENU_HH

// Copyright (c) 2023 Shynebox - zlice
//
// Copyright (c) 2003 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
//                and Simon Bowden    (rathnor at users.sourceforge.net)
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
