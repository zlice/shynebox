// IconButton.cc for Shynebox Window Manager

#include "IconButton.hh"
#include "IconbarTool.hh"
#include "IconbarTheme.hh"

#include "Screen.hh"
#include "Window.hh"

#include "tk/App.hh"
#include "tk/Command.hh"
#include "tk/EventManager.hh"
#include "tk/ImageControl.hh"
#include "tk/TextUtils.hh"

#include <X11/Xutil.h>
#ifdef SHAPE
#include <X11/extensions/shape.h>
#endif

IconButton::IconButton(const tk::SbWindow &parent,
        tk::ThemeProxy<IconbarTheme> &focused_theme,
        tk::ThemeProxy<IconbarTheme> &unfocused_theme, Focusable &win):
      tk::TextButton(parent, focused_theme->text().font(), win.title() ),
      m_win(win),
      m_icon_window(*this, 1, 1, 1, 1,
                    ExposureMask |EnterWindowMask | LeaveWindowMask |
                    ButtonPressMask | ButtonReleaseMask),
      m_use_pixmap(true),
      m_has_tooltip(false),
      m_theme(win, focused_theme, unfocused_theme),
      m_pm(win.screen().imageControl() ) {
  tk::EventManager::instance()->add(*this, m_icon_window);
} // IconButton class init

IconButton::~IconButton() {
    // ~SbWindow cleans event manager
  if (m_has_tooltip)
    m_win.screen().hideTooltip();
} // IconButton class destroy

void IconButton::exposeEvent(XExposeEvent &event) {
  if (m_icon_window == event.window)
    m_icon_window.clear();
  else
    tk::TextButton::exposeEvent(event);
}

void IconButton::enterNotifyEvent(XCrossingEvent &ev) {
  (void) ev;
  m_has_tooltip = true;
  showTooltip();
}

void IconButton::leaveNotifyEvent(XCrossingEvent &ev) {
  (void) ev;
  m_has_tooltip = false;
  m_win.screen().hideTooltip();
}

void IconButton::moveResize(int x, int y,
                            unsigned int width, unsigned int height) {
  tk::TextButton::moveResize(x, y, width, height);

  if (m_icon_window.width() != tk::Button::width()
      || m_icon_window.height() != tk::Button::height() ) {
    reconfigTheme();
    refreshEverything(false); // update icon window
  }
}

void IconButton::resize(unsigned int width, unsigned int height) {
  tk::TextButton::resize(width, height);
  if (m_icon_window.width() != tk::Button::width()
      || m_icon_window.height() != tk::Button::height() ) {
    reconfigTheme();
    refreshEverything(false); // update icon window
  }
}

void IconButton::showTooltip() {
 int xoffset = 1;
 if (m_icon_pixmap.drawable() != 0)
   xoffset = m_icon_window.x() + m_icon_window.width() + 1;

  if (tk::TextButton::textExceeds(xoffset) )
    m_win.screen().showTooltip(m_win.title() );
  else
    m_win.screen().hideTooltip();
}

void IconButton::clear() {
  setupWindow();
}

void IconButton::clearArea(int x, int y,
                           unsigned int width, unsigned int height,
                           bool exposure) {
  tk::TextButton::clearArea(x, y, width, height, exposure);
}

void IconButton::setPixmap(bool use) {
  if (m_use_pixmap != use) {
    m_use_pixmap = use;
    refreshEverything(false);
  }
}

void IconButton::reconfigTheme() {
  setFont(m_theme->text().font() );
  setGC(m_theme->text().textGC() );
  setBorderWidth(m_theme->border().width() );
  setBorderColor(m_theme->border().color() );
  setJustify(m_theme->text().justify() );

  if (m_theme->texture().usePixmap() ) {
    m_pm.reset(m_win.screen().imageControl().renderImage(
                       width(), height(), m_theme->texture(),
                       orientation() ) );
    setBackgroundPixmap(m_pm);
  } else {
    m_pm.reset(0);
    setBackgroundColor(m_theme->texture().color() );
  }
}

void IconButton::reconfigAndClear() {
  reconfigTheme();
  clear();
}

void IconButton::refreshEverything(bool setup) {
  Display *display = tk::App::instance()->display();
  int screen = m_win.screen().screenNumber();

  if (m_use_pixmap
       && m_win.icon().pixmap().drawable() != None
       && width() > m_icon_window.width() ) {
    // setup icon window
    m_icon_window.show();
    unsigned int w = width();
    unsigned int h = height();
    tk::translateSize(orientation(), w, h);
    int iconx = 1;
    int icony = 1;
    unsigned int neww;
    unsigned int newh = h;
    if (newh > 2*static_cast<unsigned>(icony) )
      newh -= 2*icony;
    else
      newh = 1;
    neww = newh;

    tk::translateCoords(orientation(), iconx, icony, w, h);
    tk::translatePosition(orientation(), iconx, icony, neww, newh, 0);

    m_icon_window.moveResize(iconx, icony, neww, newh);

    m_icon_pixmap.copy(m_win.icon().pixmap().drawable(),
                       DefaultDepth(display, screen), screen);
    m_icon_pixmap.scale(m_icon_window.width(), m_icon_window.height() );

    // rotate the icon or not?? lets go not for now, and see what they say...
    // need to rotate mask too if we do do this
    m_icon_pixmap.rotate(orientation() );

    m_icon_window.setBackgroundPixmap(m_icon_pixmap.drawable() );
  } else {
    // no icon pixmap
    m_icon_window.move(0, 0);
    m_icon_window.hide();
    m_icon_pixmap = 0;
  } // if pixmap drawable and width check

  if (m_icon_pixmap.drawable() && m_win.icon().mask().drawable() != None) {
    m_icon_mask.copy(m_win.icon().mask().drawable(), 0, 0);
    m_icon_mask.scale(m_icon_pixmap.width(), m_icon_pixmap.height() );
    m_icon_mask.rotate(orientation() );
  } else
    m_icon_mask = 0;

#ifdef SHAPE
  XShapeCombineMask(display,
                    m_icon_window.drawable(),
                    ShapeBounding,
                    0, 0,
                    m_icon_mask.drawable(),
                    ShapeSet);
#endif // SHAPE

  if (setup)
    setupWindow();
  else
    m_icon_window.clear();
} // refreshEverything

void IconButton::setupWindow() {
  m_icon_window.clear();
  tk::SbString title = m_win.title().logical();
  if (m_win.sbwindow() && m_win.sbwindow()->isIconic() )
    title = IconbarTool::iconifiedPrefix() + title + IconbarTool::iconifiedSuffix();
  setText(title);
  tk::TextButton::clear();
}

void IconButton::drawText(int x, int y, tk::SbDrawable *drawable) {
  if (width() > m_icon_window.width() ) {
    x = 1;
    if (m_icon_pixmap.drawable() != 0) // offset text
      x += m_icon_window.x() + m_icon_window.width();
    tk::TextButton::drawText(x, y, drawable);
  }
}

void IconButton::setOrientation(tk::Orientation orient) {
  if (orientation() == orient)
    return;

  tk::TextButton::setOrientation(orient);
  int iconx = 1, icony = 1;
  unsigned int tmpw = width(), tmph = height();
  tk::translateSize(orient, tmpw, tmph);
  tk::translateCoords(orient, iconx, icony, tmpw, tmph);
  tk::translatePosition(orient, iconx, icony, m_icon_window.width(), m_icon_window.height(), 0);
  m_icon_window.move(iconx, icony);
}

unsigned int IconButton::preferredWidth() const {
  IconButton *that = const_cast<IconButton*>(this);
  that->setFont(that->m_theme.focusedTheme()->text().font() );
  unsigned int r = TextButton::preferredWidth();
  that->setFont(that->m_theme.unfocusedTheme()->text().font() );
  unsigned int r2 = TextButton::preferredWidth();
  that->setFont(that->m_theme->text().font() );
  r = std::max(r, r2);
  if (m_icon_pixmap.drawable() )
    r += m_icon_window.width() + 1;
  return r;
}

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
