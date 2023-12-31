// SbMenu.cc for Shynebox Window Manager

#include "SbMenu.hh"

#include "shynebox.hh"
#include "Screen.hh"
#include "WindowCmd.hh"

namespace {
ShyneboxWindow* s_window = 0;
}

void SbMenu::setWindow(ShyneboxWindow* win) { s_window = win; }

ShyneboxWindow* SbMenu::window() { return s_window; }

SbMenu::SbMenu(tk::ThemeProxy<tk::MenuTheme> &tm,
               tk::ImageControl &imgctrl, tk::Layer &layer):
      tk::Menu(tm, imgctrl),
      m_layeritem(sbwindow(), layer) {
  sbwindow().setWindowRole("shynebox-menu");
} // SbMenu class init

void SbMenu::buttonPressEvent(XButtonEvent &be) {
  WinClient *old = WindowCmd<void>::client();
  WindowCmd<void>::setWindow(s_window);
  tk::Menu::buttonPressEvent(be);
  WindowCmd<void>::setClient(old);
}

void SbMenu::buttonReleaseEvent(XButtonEvent &be) {
  BScreen *screen = Shynebox::instance()->findScreen(screenNumber() );
  if (be.window == titleWindow() && isMoving() && screen) {
    // menu stopped moving, so update head
    int head = screen->getHead(be.x_root, be.y_root);
    setScreen(screen->getHeadX(head),
              screen->getHeadY(head),
              screen->getHeadWidth(head),
              screen->getHeadHeight(head) );
  }

  // now get on with the show
  WinClient *old = WindowCmd<void>::client();
  WindowCmd<void>::setWindow(s_window);
  tk::Menu::buttonReleaseEvent(be);
  WindowCmd<void>::setClient(old);
}

void SbMenu::keyPressEvent(XKeyEvent &ke) {
  WinClient *old = WindowCmd<void>::client();
  WindowCmd<void>::setWindow(s_window);
  tk::Menu::keyPressEvent(ke);
  WindowCmd<void>::setClient(old);
}

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
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
