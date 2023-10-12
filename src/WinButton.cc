// WinButton.cc for Shynebox Window Manager

#include "WinButton.hh"
#include "shynebox.hh"
#include "Keys.hh"
#include "Window.hh"
#include "WindowCmd.hh"
#include "Screen.hh"
#include "WinClient.hh"
#include "WinButtonTheme.hh"
#include "tk/Color.hh"
#include "tk/Command.hh"

#ifdef SHAPE
#include <X11/extensions/shape.h>
#endif

WinButton::WinButton(ShyneboxWindow &listen_to,
                     tk::ThemeProxy<WinButtonTheme> &theme,
                     tk::ThemeProxy<WinButtonTheme> &pressed,
                     Type buttontype, const tk::SbWindow &parent,
                     int x, int y,
                     unsigned int width, unsigned int height):
      tk::Button(parent, x, y, width, height),
      m_type(buttontype), m_listen_to(listen_to),
      m_theme(theme), m_pressed_theme(pressed),
      m_icon_pixmap(0), m_icon_mask(0),
      overrode_bg(false), overrode_pressed(false) {
  if (buttontype == MENUICON)
    updateAll();
}

void WinButton::exposeEvent(XExposeEvent &event) {
  tk::Button::exposeEvent(event);
  drawType();
}

void WinButton::buttonReleaseEvent(XButtonEvent &be) {
  bool didCustomAction = false;
  if (isPressed() && be.button > 0 && be.button <= 5
      && be.x >= -static_cast<signed>(borderWidth() )
      && be.x <= static_cast<signed>(width()+borderWidth() )
      && be.y >= -static_cast<signed>(borderWidth() )
      && be.y <= static_cast<signed>(height()+borderWidth() ) ) {
    int context = Keys::ON_WINBUTTON;
    if (m_type == MINIMIZE)
      context = Keys::ON_MINBUTTON;
    else if (m_type == MAXIMIZE)
      context = Keys::ON_MAXBUTTON;
    Keys *k = Shynebox::instance()->keys();
    didCustomAction = k->doAction(be.type, be.state, be.button, context, &m_listen_to.winClient(), be.time);
  } // if release makes sense
  if (!didCustomAction) { // do default action if no custom one
    WinClient *oldClient = WindowCmd<void>::client();
    WindowCmd<void>::setWindow(&m_listen_to);
    tk::Button::buttonReleaseEvent(be);
    WindowCmd<void>::setClient(oldClient);
  }
} // buttonReleaseEvent

// when someone else tries to set the background, we may override it
void WinButton::setBackgroundPixmap(Pixmap pm) {
  Pixmap my_pm = getBackgroundPixmap();

  if (my_pm != 0) {
    overrode_bg = true;
    pm = my_pm;
  } else
    overrode_bg = false;

  tk::Button::setBackgroundPixmap(pm);
}

void WinButton::setBackgroundColor(const tk::Color &color) {
  Pixmap my_pm = getBackgroundPixmap();

  if (my_pm != 0) {
    overrode_bg = true;
    tk::Button::setBackgroundPixmap(my_pm);
  } else {
    overrode_bg = false;
    tk::Button::setBackgroundColor(color);
  }
}

void WinButton::setPressedPixmap(Pixmap pm) {
  Pixmap my_pm = getPressedPixmap();

  if (my_pm != 0) {
    overrode_pressed = true;
    pm = my_pm;
  } else
    overrode_pressed = false;

  tk::Button::setPressedPixmap(pm);
}

void WinButton::setPressedColor(const tk::Color &color) {
  Pixmap my_pm = getPressedPixmap();

  if (my_pm != 0) {
    overrode_pressed = true;
    tk::Button::setPressedPixmap(my_pm);
  } else {
    overrode_pressed = false;
    tk::Button::setPressedColor(color);
  }
}

Pixmap WinButton::getPixmap(const tk::ThemeProxy<WinButtonTheme> &theme) const {
  switch(m_type) {
  case MAXIMIZE:
    return theme->maximizePixmap().pixmap().drawable();
  case MINIMIZE:
    return theme->iconifyPixmap().pixmap().drawable();
  case STICK:
    if (m_listen_to.isStuck() )
      return theme->stuckPixmap().pixmap().drawable();
    else
      return theme->stickPixmap().pixmap().drawable();
  case CLOSE:
    return theme->closePixmap().pixmap().drawable();
  case SHADE:
    if (m_listen_to.isShaded() )
      return theme->unshadePixmap().pixmap().drawable();
    else
      return theme->shadePixmap().pixmap().drawable();
  case MENUICON:
    if (m_icon_pixmap.drawable() )
      return theme->titlePixmap().pixmap().drawable();
    else
      return theme->menuiconPixmap().pixmap().drawable();
  default:
    return None;
  }
}

// clear is used to force this to clear the window (e.g. called from clear() )
void WinButton::drawType() {
  int w = width();
  int h = height();
  int oddW = w % 2; // if it's odd and we're centring, we need to add one
  int oddH = h % 2;
  bool is_pressed = pressed();

  if ((!m_icon_pixmap.drawable()
        && ((is_pressed && overrode_pressed)
          || (!is_pressed && overrode_bg) ) )
      || gc() == 0)
    return;

  // otherwise draw old style imagery
  switch (m_type) {
  case MAXIMIZE:
    if ((w < 6) || (h < 6) )
      return;

    drawRectangle(gc(), 2, 2, w - 5, h - 5);
    drawLine(gc(), 2, 3, w - 3, 3);
    break;
  case MINIMIZE:
    if ((w < 6) || (h < 6) )
      return;

    drawRectangle(gc(), 2, w - 5, h - 5, 2);
    break;
  case STICK:
    {
    int s = 4;
    if (!m_listen_to.isStuck() )
      s = 8;

    fillRectangle(gc(), (w / 2) - (w / s), (h / 2) - (h / s),
                        2*(w / s) + oddW, 2*(h / s) + oddH);
    }
    break;
  case CLOSE:
    if ((w < 4) || (h < 4) )
      return;

    drawLine(gc(), 2, 2, w - 3, h - 3);
    drawLine(gc(), 2, w - 3, h - 3, 2);
    break;
  case SHADE: {
    int size = w - 5 - oddW;
    if (size < 4)
      return;

    tk::SbDrawable::TriangleType dir = (m_listen_to.isShaded() ?
                     tk::SbDrawable::DOWN: tk::SbDrawable::UP);

    drawRectangle(gc(), 2, 2, size, 2);
    // draw a one-quarter triangle below the rectangle
    drawTriangle(gc(), dir, 4, 6, size-2, size/2 - 1, 100);
    break;
  }
  case MENUICON:
    if (m_icon_pixmap.drawable() ) {
      Display* disp = m_listen_to.sbWindow().display();

      if (m_icon_mask.drawable() ) {
        XSetClipMask(disp, gc(), m_icon_mask.drawable() );
        XSetClipOrigin(disp, gc(), 2, 2);
      }

      copyArea(m_icon_pixmap.drawable(), gc(),
               0, 0, 2, 2,
               m_icon_pixmap.width(), m_icon_pixmap.height() );

      if (m_icon_mask.drawable() )
        XSetClipMask(disp, gc(), None);
    } else {
      if ((w < 6) || (h < 6) )
        return;

      int y = h / 3;
      for ( ; y <= h; y += 3)
        drawLine(gc(), w / 4, y, w - (w / 4) - 1, y);
      drawRectangle(gc(), 2, 2, w - 5, h - 5);
    } // if drawable
    break;
  } // switch (m_type)
} // drawType

void WinButton::clear() {
  tk::Button::clear();
  drawType();
}
void WinButton::updateAll() {
  int w = static_cast<int>(width() ) - 4;
  int h = static_cast<int>(height() ) - 4;

  // update the menu icon
  if ((w > 0 && h > 0) && m_type == MENUICON && !m_listen_to.empty() ) {
    Display* display = m_listen_to.sbWindow().display();
    int screen = m_listen_to.screen().screenNumber();

    Drawable d = m_listen_to.icon().pixmap().drawable();
    if (d != None) {
       m_icon_pixmap.copy(d, DefaultDepth(display, screen), screen);
       m_icon_pixmap.scale(w, h);
    } else
      m_icon_pixmap.release();

    d = m_listen_to.icon().mask().drawable();
    if (d != None) {
      m_icon_mask.copy(d, 0, 0);
      m_icon_mask.scale(w, h);
    } else
      m_icon_mask.release();
  } // if w & h & MENUICON & !empty

  // pressed_pixmap isn't stateful in any current buttons, so no need
  // to potentially override that. Just make sure background pm is ok
  Pixmap my_pm = getBackgroundPixmap();
  if (my_pm != None)
    setBackgroundPixmap(my_pm);

  // incorrect, pressed_pixmap is stateful in shade, so we'll do oneoff for now
  if (m_type == SHADE || m_type == STICK) {
    Pixmap p_pm = getPressedPixmap();
    if (p_pm != None)
      setPressedPixmap(p_pm);
  }
  clear();
} // updateAll

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
