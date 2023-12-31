// SystemTray.hh for Shynebox Window Manager

/*
  Toolbar item for docked notification windows.
  Think mail clients, discord, zoom, etc
*/

#ifndef SYSTEMTRAY_HH
#define SYSTEMTRAY_HH

#include "tk/SbWindow.hh"
#include "tk/EventHandler.hh"
#include "tk/Config.hh" // string

#include "ToolTheme.hh"
#include "ToolbarItem.hh"
#include "WinClient.hh"

#include <list>

class BScreen;

namespace tk {
template <class T> class ThemeProxy;
}

class SystemTray: public ToolbarItem, public tk::EventHandler {
public:
  explicit SystemTray(const tk::SbWindow &parent,
                      tk::ThemeProxy<ToolTheme> &theme, BScreen& screen);
  virtual ~SystemTray();

  void move(int x, int y);
  void resize(unsigned int width, unsigned int height);
  void moveResize(int x, int y,
                  unsigned int width, unsigned int height);
  void show();
  void hide();

  bool active() { return !m_clients.empty(); }

  bool clientMessage(const XClientMessageEvent &event);
  void exposeEvent(XExposeEvent &event);
  void handleEvent(XEvent &event);

  void addClient(Window win, bool using_xembed);
  void removeClient(Window win, bool destroyed);

  unsigned int width() const;
  unsigned int height() const;
  unsigned int borderWidth() const;

  int numClients() const { return m_clients.size(); }
  const tk::SbWindow &window() const { return m_window; }

  void renderTheme() {
    m_window.setBorderWidth(m_theme->border().width() );
    m_window.setBorderColor(m_theme->border().color() );
    update();
  }

  // HACK: this does not work properly on its own when switching themes.
  //       height changes can warp width incorrectly
  //       Toolbar fixes it by checking size changes and re-calling relayout()
  void updateSizing() { m_window.setBorderWidth(m_theme->border().width() ); }

  void parentMoved() { m_window.parentMoved(); }

  bool checkClientMessage(const XClientMessageEvent &ce,
                          BScreen * screen, WinClient * const winclient);
  void setupClient(WinClient &winclient);

  static std::string getNetSystemTrayAtom(int screen_nr);
  static Atom getXEmbedInfoAtom();

private:
  void update();

  class TrayWindow;
  typedef std::list<TrayWindow*> ClientList;
  ClientList::iterator findClient(Window win);

  void rearrangeClients();
  void removeAllClients();
  void hideClient(TrayWindow *traywin, bool destroyed = false);
  void showClient(TrayWindow *traywin);

  tk::SbWindow m_window;
  tk::ThemeProxy<ToolTheme> &m_theme;
  BScreen& m_screen;
  Pixmap m_pixmap;

  ClientList m_clients;
  size_t m_num_visible_clients;

  // gaim/pidgin seems to barf if the selection is not an independent window.
  // I suspect it's an interacton with parent relationship and gdk window caching.
  // 2023 verified still true
  tk::SbWindow m_selection_owner;

  // config items
  std::string &m_rc_systray_pinleft;
  std::string &m_rc_systray_pinright;

  std::vector<std::string> m_pinleft, m_pinright;
};

#endif // SYSTEMTRAY_HH

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
