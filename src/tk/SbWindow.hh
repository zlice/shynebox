// SbWindow.hh for Shynebox Window Manager

/*
  Wrapper for actual X window.
  It will skip doing extra work for moves/resizes if there is no change.

  X window's are the 'real' windows > SbWindows(this) are the base WM
  windows > ShyneboxWindow/Menu/Toolbar/Frames/etc are the end use windows

  Usage Example:
  SbWindow window(0, 10, 10, 100, 100, ExposeMask | ButtonPressMask);
  this will create a window on screen 0, position 10 10, size 100 100
  and with eventmask Expose and ButtonPress.
  You need to register it to some eventhandler so you can catch events:
  EventManager::instance()->add(your_eventhandler, window);

  see EventHandler and EventManager
*/

#ifndef TK_SBWINDOW_HH
#define TK_SBWINDOW_HH

#include "SbDrawable.hh"

#include <string>

namespace tk {
typedef std::string SbString;
class BiDiString;
class Color;
class SbWindowRenderer;

class SbWindow: public SbDrawable {
public:
  static Window rootWindow(Display* dpy, Drawable win);

  SbWindow();
  SbWindow(const SbWindow &win_copy);

  SbWindow(int screen_num,
           int x, int y, unsigned int width, unsigned int height, long eventmask,
           bool overrride_redirect = false,
           bool save_unders = false,
           unsigned int depth = CopyFromParent,
           int class_type = InputOutput,
           Visual *visual = CopyFromParent,
           Colormap cmap = CopyFromParent);

  SbWindow(const SbWindow &parent,
           int x, int y,
           unsigned int width, unsigned int height,
           long eventmask,
           bool overrride_redirect = false,
           bool save_unders = false,
           unsigned int depth = CopyFromParent,
           int class_type = InputOutput,
           Visual *visual = CopyFromParent,
           Colormap cmap = CopyFromParent);

  virtual ~SbWindow();
  virtual void setBackgroundColor(const tk::Color &bg_color);
  virtual void setBackgroundPixmap(Pixmap bg_pixmap);
  // call when background is freed, and new one not ready yet
  virtual void invalidateBackground();
  virtual void setBorderColor(const tk::Color &border_color);
  virtual void setBorderWidth(unsigned int size);
  // set window name ("title")
  void setName(const char *name);
  // set window role
  void setWindowRole(const char *windowRole);
  void setEventMask(long mask);
  // clear window with background pixmap or color
  virtual void clear();
  // exposures wheter Expose event should be generated
  virtual void clearArea(int x, int y,
                         unsigned int width, unsigned int height,
                         bool exposures = false);

  virtual SbWindow &operator = (const SbWindow &win);
  // assign a new X window to this
  virtual SbWindow &operator = (Window win);
  virtual void hide();
  virtual void show();
  virtual void showSubwindows();

  // Notify that the parent window was moved,
  // thus the absolute position of this one moved
  virtual void parentMoved() {
    updateBackground();
  }

  virtual void move(int x, int y) {
    if (x == m_x && y == m_y)
      return;
    XMoveWindow(display(), m_window, x, y);
    m_x = x;
    m_y = y;
    updateBackground();
  }

  virtual void resize(unsigned int width, unsigned int height) {
    if (width == m_width && height == m_height)
      return;
    XResizeWindow(display(), m_window, width, height);
    m_width = width;
    m_height = height;
    updateBackground();
  }

  virtual void moveResize(int x, int y, unsigned int width, unsigned int height) {
    if (x == m_x && y == m_y && width == m_width && height == m_height)
      return;
    XMoveResizeWindow(display(), m_window, x, y, width, height);
    m_x = x;
    m_y = y;
    m_width = width;
    m_height = height;
    updateBackground();
  }
  virtual void lower();
  virtual void raise();
  void setInputFocus(int revert_to, int time);
  // defines a cursor for this window
  void setCursor(Cursor cur);

  void reparent(const SbWindow &parent, int x, int y, bool continuing = true);

  bool property(Atom property,
                long long_offset, long long_length,
                bool do_delete,
                Atom req_type,
                Atom *actual_type_return,
                int *actual_format_return,
                unsigned long *nitems_return,
                unsigned long *bytes_after_return,
                unsigned char **prop_return) const;

  void changeProperty(Atom property, Atom type,
                      int format,
                      int mode,
                      unsigned char *data,
                      int nelements);

  void deleteProperty(Atom property);

  long cardinalProperty(Atom property, bool*exists=NULL) const;
  tk::SbString textProperty(Atom property,bool*exists=NULL) const;

  void addToSaveSet();
  void removeFromSaveSet();

  // parent SbWindow
  const SbWindow *parent() const { return m_parent; }
  // real X window
  Window window() const { return m_window; }
  // drawable (the X window)
  Drawable drawable() const { return window(); }
  int x() const { return m_x; }
  int y() const { return m_y; }
  unsigned int width() const { return m_width; }
  unsigned int height() const { return m_height; }
  unsigned int borderWidth() const { return m_border_width; }
  unsigned long borderColor() const { return m_border_color; }
  unsigned int depth() const { return m_depth; }
  int screenNumber() const;
  long eventMask() const;

  // compare X window
  bool operator == (Window win) const { return m_window == win; }
  bool operator != (Window win) const { return m_window != win; }
  // compare two windows
  bool operator == (const SbWindow &win) const { return m_window == win.m_window; }
  bool operator != (const SbWindow &win) const { return m_window != win.m_window; }

  void setRenderer(SbWindowRenderer &renderer) { m_renderer = &renderer; }
  void sendConfigureNotify(int x, int y, unsigned int width,
                           unsigned int height, unsigned int bw = 0);
  void updateBackground();

  // updates x,y, width, height and screen num from X window
  bool updateGeometry();

protected:
  // creates a window with x window client (m_window = client)
  explicit SbWindow(Window client);
  void setDepth(unsigned int depth) { m_depth = depth; }

private:
  // sets new X window and destroys old
  void setNew(Window win);
  // creates a new X window
  void create(Window parent, int x, int y, unsigned int width, unsigned int height,
              long eventmask,
              bool override_redirect,
              bool save_unders,
              unsigned int depth,
              int class_type,
              Visual *visual,
              Colormap cmap);

  const SbWindow *m_parent;       // parent SbWindow
  int m_screen_num;               // screen num on which this window exist
  mutable Window m_window;        // the X window
  int m_x, m_y;                   // position of window
  unsigned int m_width, m_height; // size of window
  unsigned int m_border_width;    // border size
  unsigned long m_border_color;   // border color
  unsigned int m_depth;           // bit depth
  bool m_destroy;                 // wheter the x window was created before
  bool m_lastbg_color_set;
  unsigned long m_lastbg_color;
  Pixmap m_lastbg_pm;

  SbWindowRenderer *m_renderer;
}; // SbWindow

bool operator == (Window win, const SbWindow &sbwin);

// Interface class to render SbWindow foregrounds.
class SbWindowRenderer {
public:
  virtual void renderForeground(SbWindow &win, SbDrawable &drawable) = 0;
  virtual ~SbWindowRenderer() { }
};

} // end namespace tk

#endif // TK_SBWINDOW_HH

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
