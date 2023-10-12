// SbWindow.cc for Shynebox Window Manager

#include "SbWindow.hh"
#include "SbPixmap.hh"
#include "SbString.hh"

#include "EventManager.hh"
#include "Color.hh"
#include "App.hh"

#include "Debug.hh"

#include <X11/Xutil.h>
#include <X11/Xatom.h>

#include <cassert>
#include <iostream>
#include <limits>
#include <string.h>

namespace tk {

Window SbWindow::rootWindow(Display* dpy, Drawable win) {
  union { int i; unsigned int ui; } ignore;
  Window root = None;
  XGetGeometry(dpy, win, &root, &ignore.i, &ignore.i, &ignore.ui, &ignore.ui, &ignore.ui, &ignore.ui);
  return root;
}

// SbWindow class inits
SbWindow::SbWindow():
  SbDrawable(),
  m_parent(0), m_screen_num(0), m_window(0),
  m_x(0), m_y(0), m_width(0), m_height(0),
  m_border_width(0), m_border_color(0),
  m_depth(0), m_destroy(true),
  m_lastbg_color_set(false), m_lastbg_color(0), m_lastbg_pm(0),
  m_renderer(0) { }

SbWindow::SbWindow(const SbWindow& the_copy):
  SbDrawable(),
  m_parent(the_copy.parent() ),
  m_screen_num(the_copy.screenNumber() ), m_window(the_copy.window() ),
  m_x(the_copy.x() ), m_y(the_copy.y() ),
  m_width(the_copy.width() ), m_height(the_copy.height() ),
  m_border_width(the_copy.borderWidth() ),
  m_border_color(the_copy.borderColor() ),
  m_depth(the_copy.depth() ), m_destroy(true),
  m_lastbg_color_set(false), m_lastbg_color(0), m_lastbg_pm(0),
  m_renderer(the_copy.m_renderer) {
  the_copy.m_window = 0;
}

SbWindow::SbWindow(int screen_num,
                   int x, int y,
                   unsigned int width, unsigned int height,
                   long eventmask,
                   bool override_redirect,
                   bool save_unders,
                   unsigned int depth,
                   int class_type,
                   Visual *visual,
                   Colormap cmap):
      SbDrawable(),
      m_parent(0),
      m_screen_num(screen_num),
      m_window(0),
      m_x(0), m_y(0), m_width(1), m_height(1),
      m_border_width(0),
      m_border_color(0),
      m_depth(0),
      m_destroy(true),
      m_lastbg_color_set(false),
      m_lastbg_color(0),
      m_lastbg_pm(0), m_renderer(0) {
  create(RootWindow(display(), screen_num),
         x, y, width, height, eventmask,
         override_redirect, save_unders, depth, class_type, visual, cmap);
}

SbWindow::SbWindow(const SbWindow &parent,
                   int x, int y, unsigned int width, unsigned int height,
                   long eventmask,
                   bool override_redirect,
                   bool save_unders,
                   unsigned int depth,
                   int class_type,
                   Visual *visual,
                   Colormap cmap):
      SbDrawable(),
      m_parent(&parent),
      m_screen_num(parent.screenNumber() ),
      m_window(0),
      m_x(0), m_y(0),
      m_width(1), m_height(1),
      m_destroy(true),
      m_lastbg_color_set(false), m_lastbg_color(0),
      m_lastbg_pm(0), m_renderer(0) {
  create(parent.window(), x, y, width, height, eventmask,
         override_redirect, save_unders, depth, class_type, visual, cmap);
}

SbWindow::SbWindow(Window client):
      SbDrawable(),
      m_parent(0), m_screen_num(0), m_window(0),
      m_x(0), m_y(0), m_width(1), m_height(1),
      m_border_width(0), m_border_color(0),
      m_depth(0), m_destroy(false), // don't destroy this window
      m_lastbg_color_set(false), m_lastbg_color(0), m_lastbg_pm(0),
      m_renderer(0) {
  setNew(client);
}
// SbWindow class inits

SbWindow::~SbWindow() {
  if (m_window != 0) {
    // so we don't get any dangling eventhandler for this window
    tk::EventManager::instance()->remove(m_window);
    if (m_destroy)
      XDestroyWindow(display(), m_window);
  }
} // SbWindow class destroy

void SbWindow::setBackgroundColor(const tk::Color &bg_color) {
  if (bg_color.isAllocated() ) {
    m_lastbg_color = bg_color.pixel();
    m_lastbg_color_set = true;
    m_lastbg_pm = None;
  } else
    m_lastbg_color_set = false;

  updateBackground();
}

void SbWindow::setBackgroundPixmap(Pixmap bg_pixmap) {
  m_lastbg_pm = bg_pixmap;
  if (bg_pixmap != None)
    m_lastbg_color_set = false;

  updateBackground();
}

void SbWindow::invalidateBackground() {
  m_lastbg_pm = None;
  m_lastbg_color_set = false;
}

void SbWindow::updateBackground() {
  Pixmap newbg = m_lastbg_pm;
  bool free_newbg = false;

  if (m_lastbg_pm == None && !m_lastbg_color_set)
    return;

  // cause it does nice caching things, assuming we have a renderer
  if (m_lastbg_pm != ParentRelative && m_renderer) {
    SbPixmap newpm = SbPixmap(*this, width(), height(), depth() );
    free_newbg = true; // newpm gets released to newbg at end of block
    GC gc = XCreateGC(display(), window(), 0, 0);

    if (m_lastbg_pm == None && m_lastbg_color_set) {
      XSetForeground(display(), gc, m_lastbg_color);
      newpm.fillRectangle(gc, 0, 0, width(), height() );
    } else // copy from window if no color and no bg...
      newpm.copyArea((m_lastbg_pm == None)?drawable():m_lastbg_pm, gc, 0, 0, 0, 0, width(), height() );
    XFreeGC(display(), gc);

    // render any foreground items
    if (m_renderer)
      m_renderer->renderForeground(*this, newpm);

    newbg = newpm.release();
  } // if lastbg != Parent and m_renderer

  if (newbg != None)
    XSetWindowBackgroundPixmap(display(), m_window, newbg);
  else if (m_lastbg_color_set)
    XSetWindowBackground(display(), m_window, m_lastbg_color);

  if (free_newbg)
    XFreePixmap(display(), newbg);
} // updatebackground

void SbWindow::setBorderColor(const tk::Color &border_color) {
  XSetWindowBorder(display(), m_window, border_color.pixel() );
  m_border_color = border_color.pixel();
}

void SbWindow::setBorderWidth(unsigned int size) {
  XSetWindowBorderWidth(display(), m_window, size);
  m_border_width = size;
}

void SbWindow::setName(const char *name) {
  XStoreName(display(), m_window, name);
  Atom net_wm_name = XInternAtom(display(), "_NET_WM_NAME", False);
  Atom utf8_string = XInternAtom(display(), "UTF8_STRING", False);
  XChangeProperty(display(), m_window,
                  net_wm_name, utf8_string, 8,
                  PropModeReplace,
                  (unsigned char*)name, strlen(name) );
}

void SbWindow::setWindowRole(const char *windowRole) {
  XTextProperty tp;
  XStringListToTextProperty(const_cast<char **>(&windowRole), 1, &tp);
  XSetTextProperty(display(), m_window, &tp, XInternAtom(display(), "WM_WINDOW_ROLE", False) );
  XFree(tp.value);
}

void SbWindow::setEventMask(long mask) {
  XSelectInput(display(), m_window, mask);
}

void SbWindow::clear() {
  XClearWindow(display(), m_window);
  if (m_lastbg_pm == ParentRelative && m_renderer)
    m_renderer->renderForeground(*this, *this);
}

void SbWindow::clearArea(int x, int y,
                         unsigned int width, unsigned int height,
                         bool exposures) {
  if (m_lastbg_pm == ParentRelative && m_renderer)
    SbWindow::clear();
  else
    XClearArea(display(), window(), x, y, width, height, exposures);
}

SbWindow &SbWindow::operator = (const SbWindow &win) {
  m_parent = win.parent();
  m_screen_num = win.screenNumber();
  m_window = win.window();
  m_x = win.x();
  m_y = win.y();
  m_width = win.width();
  m_height = win.height();
  m_border_width = win.borderWidth();
  m_border_color = win.borderColor();
  m_depth = win.depth();
  // take over this window
  win.m_window = 0;
  return *this;
}

SbWindow &SbWindow::operator = (Window win) {
  setNew(win);
  return *this;
}

void SbWindow::setNew(Window win) {
  if (m_window != 0 && m_destroy)
    XDestroyWindow(display(), m_window);

  m_window = win;

  if (m_window != 0) {
    updateGeometry();
    XWindowAttributes attr;
    attr.screen = 0;
    //get screen number
    if (XGetWindowAttributes(display(), m_window,
                &attr) != 0 && attr.screen != 0) {
      m_screen_num = XScreenNumberOfScreen(attr.screen);
      if (attr.width <= 0)
        m_width = 1;
      else
        m_width = attr.width;

      if (attr.height <= 0)
        m_height = 1;
      else
        m_height = attr.height;

      m_x = attr.x;
      m_y = attr.y;
      m_depth = attr.depth;
      m_border_width = attr.border_width;
    }
  }
} // setNew

void SbWindow::show() {
  XMapWindow(display(), m_window);
}

void SbWindow::showSubwindows() {
  XMapSubwindows(display(), m_window);
}

void SbWindow::hide() {
  XUnmapWindow(display(), m_window);
}

void SbWindow::lower() {
  XLowerWindow(display(), window() );
}

void SbWindow::raise() {
  XRaiseWindow(display(), window() );
}

void SbWindow::setInputFocus(int revert_to, int time) {
  XSetInputFocus(display(), window(), revert_to, time);
}

void SbWindow::setCursor(Cursor cur) {
  XDefineCursor(display(), window(), cur);
}

void SbWindow::reparent(const SbWindow &parent, int x, int y, bool continuing) {
  XReparentWindow(display(), window(), parent.window(), x, y);
  m_parent = &parent;
  if (continuing) // we will continue managing this window after reparent
    updateGeometry();
}

namespace {
struct TextPropPtr {
  TextPropPtr(XTextProperty& prop):m_prop(prop) {}
  ~TextPropPtr() {
    if (m_prop.value != 0) {
      XFree(m_prop.value);
      m_prop.value = 0;
    }
  }
  XTextProperty& m_prop;
};
}

long SbWindow::cardinalProperty(Atom prop, bool* exists) const {
  Atom type;
  int format;
  unsigned long nitems, bytes_after;
  long* num;
  long ret=0;
  if (exists) *exists=false;
  if (property(prop, 0, 1, False, XA_CARDINAL, &type, &format,
      &nitems, &bytes_after, reinterpret_cast<unsigned char**>(&num) ) ) {
    if (type == XA_CARDINAL && nitems) {
      ret = *num;
      if (exists) *exists=true;
    }
    XFree(num);
  }
  return ret;
}

tk::SbString SbWindow::textProperty(Atom prop,bool*exists) const {
  XTextProperty text_prop;
  TextPropPtr ensure_value_freed(text_prop);
  char ** stringlist = 0;
  int count = 0;
  bool found = false;
  tk::SbString ret = "";

  static const Atom utf8string = XInternAtom(display(), "UTF8_STRING", False);

  if (exists) *exists=false;
  Status stat = XGetTextProperty(display(), window(), &text_prop, prop);
  if ( stat == 0 || text_prop.value == 0 || text_prop.nitems == 0)
    return ret;

  if (text_prop.encoding == XA_STRING) {
    if (XTextPropertyToStringList(&text_prop, &stringlist, &count)
        && count) {
      found = true;
      ret = SbStringUtil::XStrToSb(stringlist[0]);
    }
  } else if (text_prop.encoding == utf8string && text_prop.format == 8) {
      int retcode;
#ifdef X_HAVE_UTF8_STRING
      retcode = Xutf8TextPropertyToTextList(display(), &text_prop, &stringlist, &count);
#else
      retcode = XTextPropertyToStringList(&text_prop, &stringlist, &count);
#endif
      if (retcode == XLocaleNotSupported) {
        std::cerr << "Warning: current locale not supported, using "
                  << "raw _NET_WM_NAME text value.\n";
        found = true;
        ret = reinterpret_cast<char*>(text_prop.value);
      } else if (count && stringlist) {
        found = true;
        ret = stringlist[0];
      } // XLocaleNotSupported
  } else {
      // still returns a "StringList" despite the different name
      XmbTextPropertyToTextList(display(), &text_prop, &stringlist, &count);
      if (count && stringlist) {
        found = true;
        ret = SbStringUtil::LocaleStrToSb(stringlist[0]);
      }
  } // if XA_STRING

  if (stringlist) XFreeStringList(stringlist);
  if (exists)     *exists = found;
  return ret;
} // textProperty

bool SbWindow::property(Atom prop,
                        long long_offset, long long_length,
                        bool do_delete,
                        Atom req_type,
                        Atom *actual_type_return,
                        int *actual_format_return,
                        unsigned long *nitems_return,
                        unsigned long *bytes_after_return,
                        unsigned char **prop_return) const {
  if (XGetWindowProperty(display(), window(),
                         prop, long_offset, long_length, do_delete,
                         req_type, actual_type_return,
                         actual_format_return, nitems_return,
                         bytes_after_return, prop_return) == Success)
    return true;
  return false;
}

void SbWindow::changeProperty(Atom prop, Atom type,
                              int format,
                              int mode,
                              unsigned char *data,
                              int nelements) {
  XChangeProperty(display(), m_window, prop, type,
                  format, mode,
                  data, nelements);
}

void SbWindow::deleteProperty(Atom prop) {
  XDeleteProperty(display(), m_window, prop);
}

void SbWindow::addToSaveSet() {
  XAddToSaveSet(display(), m_window);
}

void SbWindow::removeFromSaveSet() {
  XRemoveFromSaveSet(display(), m_window);
}

int SbWindow::screenNumber() const {
  return m_screen_num;
}

long SbWindow::eventMask() const {
  XWindowAttributes attrib;
  XGetWindowAttributes(display(), window(), &attrib);
  return attrib.your_event_mask;
}

bool SbWindow::updateGeometry() {
  if (m_window == 0)
    return false;

  int old_x = m_x, old_y = m_y;
  unsigned int old_width = m_width, old_height = m_height;

  Window root;
  unsigned int border_width, depth;
  if (XGetGeometry(display(), m_window, &root, &m_x, &m_y,
                   &m_width, &m_height, &border_width, &depth) )
    m_depth = depth;

  return (old_x != m_x || old_width != m_width
          || old_y != m_y || old_height != m_height);
}

void SbWindow::create(Window parent, int x, int y,
                      unsigned int width, unsigned int height,
                      long eventmask, bool override_redirect,
                      bool save_unders, unsigned int depth, int class_type,
                      Visual *visual, Colormap cmap) {
  m_border_width = 0;
  m_border_color = 0;

  long valmask = CWEventMask;
  XSetWindowAttributes values;
  values.event_mask = eventmask;

  if (override_redirect) {
    valmask |= CWOverrideRedirect;
    values.override_redirect = True;
  }

  if (save_unders) {
    valmask |= CWSaveUnder;
    values.save_under = True;
  }

  if (cmap != CopyFromParent) {
    valmask |= CWColormap | CWBackPixel | CWBorderPixel;
    values.colormap = cmap;
    values.background_pixel = XWhitePixel(display(), 0);
    values.border_pixel = XBlackPixel(display(), 0);
  }

  m_window = XCreateWindow(display(), parent, x, y, width, height,
                           0, // border width
                           depth, // depth
                           class_type, // class
                           visual, // visual
                           valmask, // create mask
                           &values); // create atrribs

  assert(m_window);
  updateGeometry();
} // create

void SbWindow::sendConfigureNotify(int x, int y,
                                   unsigned int width, unsigned int height,
                                   unsigned int bw) {
  Display *disp = tk::App::instance()->display();
  XEvent event;
  event.type = ConfigureNotify;

  event.xconfigure.display = disp;
  event.xconfigure.event = window();
  event.xconfigure.window = window();
  event.xconfigure.x = x;
  event.xconfigure.y = y;
  event.xconfigure.width = width;
  event.xconfigure.height = height;
  event.xconfigure.border_width = bw;
  event.xconfigure.above = None;
  event.xconfigure.override_redirect = false;

  XSendEvent(disp, window(), False, StructureNotifyMask, &event);
}

bool operator == (Window win, const SbWindow &sbwin) {
  return win == sbwin.window();
}

} // tk namespace

// Copyright (c) 2023 Shynebox - zlice
//
// Copyright (c) 2002 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
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
