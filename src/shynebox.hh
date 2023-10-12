// shynebox.hh for Shynebox Window Manager

/*
  Start a singleton instance of the window manager.
  Initiates config and theme loads, EWMH and Remember instances,
  and creates Screen(s).
  Handles self-restarts and reconfigures.
  Handle XEvents and passes them to the appropriate places.
  Host notifier functions for updates like workspace name changes,
  EWMH hints and other state changes.
*/

#ifndef SHYNEBOX_HH
#define SHYNEBOX_HH

#include "tk/App.hh"
#include "tk/Config.hh" // map
#include "tk/MacroCommand.hh"
#include "tk/MenuSearch.hh"
#include "tk/Timer.hh"

#include "Ewmh.hh"
#include "Focusable.hh"
#include "Remember.hh"

#ifdef TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#else // !TIME_WITH_SYS_TIME
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#else // !HAVE_SYS_TIME_H
#include <time.h>
#endif // HAVE_SYS_TIME_H
#endif // TIME_WITH_SYS_TIME

#include <list>
#include <set>
#include <vector>
#include <cstdio>

class ShyneboxWindow;
class WinClient;
class Keys;
class BScreen;
class SbAtoms;

class Shynebox : public tk::App {
public:
  typedef std::list<BScreen *> ScreenList;

  enum {
    OPT_TOOLBAR = 1 << 0,
  };

  static Shynebox *instance();

  Shynebox(int argc, char **argv,
          const std::string& dpy_name,
          const std::string& rc_path, const std::string& rc_filename,
          bool xsync = false);
  virtual ~Shynebox();

  void eventLoop();
  void flag_button_replay();

  void grab();
  void ungrab();
  Keys *keys() { return m_key; }
  Atom getShyneboxPidAtom() const { return m_shynebox_pid; }

  WinClient *searchWindow(Window);
  BScreen *searchScreen(Window w);
  bool validateWindow(Window win) const;
  bool validateClient(const WinClient *client) const;

  // Not currently implemented until we decide how it'll be used
  //WinClient *searchGroup(Window);

  Time getLastTime() const { return m_last_time; }

  std::string getDefaultDataFilename(const char *name) const;

  const std::string &getAppsFilename() const         { return m_config.apps_file; }
  const std::string &getKeysFilename() const         { return m_config.key_file; }
  const std::string &getMenuFilename() const         { return m_config.menu_file; }
  const std::string &getStyleOverlayFilename() const { return m_config.overlay_file; }
  const std::string &getStyleFilename() const        { return m_config.style_file; }
  const std::string &getWinMenuFilename() const      { return m_config.winmenu_file; }
  int colorsPerChannel() const                       { return m_config.colors_per_channel; }
  int getTabsPadding() const                         { return m_config.tabs_padding; }
  unsigned int getDoubleClickInterval() const        { return m_config.double_click_interval; }
  time_t getAutoRaiseDelay() const                   { return m_config.auto_raise_delay; }
  unsigned int getCacheLife() const                  { return m_config.cache_life * tk::SbTime::IN_MINUTES; }
  unsigned int getCacheMax() const                   { return m_config.cache_max; }

  void maskWindowEvents(Window w, ShyneboxWindow *bw)
      { m_masked = w; m_masked_window = bw; }

  void shutdown(int x_wants_down = 0);
  void saveStyleFilename(const char *val) { m_config.style_file = (val == 0 ? "" : val); }
  void saveWindowSearch(Window win, WinClient *winclient);
  // some windows relate to the group, not the client, so we record separately
  // searchWindow on these windows will give the active client in the group
  void saveWindowSearchGroup(Window win, ShyneboxWindow *sbwin);
  void saveGroupSearch(Window win, WinClient *winclient);
  void save_rc();
  void removeWindowSearch(Window win);
  void removeWindowSearchGroup(Window win);
  void removeGroupSearch(Window win);
  void restart(const char *command = 0);
  void reconfigure();
  void reconfigThemes();

  void updateFrameExtents(ShyneboxWindow &win);

  void setupFrame(ShyneboxWindow &win);
  void setupClient(WinClient &winclient);

  void timed_reconfigure();
  void revertFocus();
  void setShowingDialog(bool value) {
    m_showing_dialog = value; if (!value) revertFocus();
  }

  bool isStartup() const       { return m_state.starting; }
  bool isRestarting() const    { return m_state.restarting; }
  bool isShuttingDown() const  { return m_state.shutdown; }

  const std::string &getRestartArgument() const { return m_restart_argument; }

  // get screen from number
  BScreen *findScreen(int num);

  const ScreenList screenList() const { return m_screens; }

  bool haveShape() const;
  int shapeEventbase() const;

  BScreen *mouseScreen() { return m_active_screen.mouse; }
  BScreen *keyScreen() { return m_active_screen.key; }
  const XEvent &lastEvent() const { return m_last_event; }
  // most of these are self explanatory
  // usually call Ewmh, maybe Remember and toolbar updates
  void windowLayerChanged(ShyneboxWindow &win);
  void clientListChanged(BScreen &screen);
  void workspaceAreaChanged(BScreen &screen);
  void workspaceChanged(BScreen& screen);

  void workspaceNamesChanged(BScreen &screen);
  void workspaceCountChanged( BScreen& screen );

  // only caller is FocusController
  void focusedWindowChanged(BScreen &screen,
                            WinClient* client);
  // if window isMoving() then raise and focus
  void windowWorkspaceChanged(ShyneboxWindow &win);
  // some extra workspace/screen checks
  void windowStateChanged(ShyneboxWindow &win);

  // remove from its screen and reset focus
  void windowDied(Focusable &focusable);
  void clientDied(Focusable &focusable);

  tk::Command<void> *getSharedSaveRC() { return m_shared_saverc; }

  tk::Command<void> *getSharedSaveRcfgMacro() {
    return (tk::Command<void>*)m_shared_sv_rcfg_macro;
  }

private:
  std::string getRcFilename();
  void load_rc();
  void real_reconfigure();
  void handleEvent(XEvent *xe);
  void handleUnmapNotify(XUnmapEvent &ue);
  void handleClientMessage(XClientMessageEvent &ce);

  typedef std::map<Window, WinClient *> WinClientMap;
  typedef std::map<Window, ShyneboxWindow *> WindowMap;

  tk::Command<void> *m_shared_saverc = 0;
  tk::MacroCommand *m_shared_sv_rcfg_macro = 0;

  SbAtoms *m_sbatoms = 0;
  tk::ConfigManager m_configmanager;

  // ConfigManager and config values
  struct Config {
      Config(tk::ConfigManager& cm);

      std::string path;
      std::string file;

      int &colors_per_channel, &double_click_interval, &tabs_padding;

      std::string &apps_file, &key_file, &menu_file,
          &overlay_file, &style_file, &winmenu_file;

      tk::MenuMode_e &menusearch;

      unsigned int &cache_life, &cache_max, &auto_raise_delay;
  } m_config;

  Keys *m_key = 0;
  Ewmh                   *m_ewmh;
  Remember               *m_remember;

  ScreenList             m_screens;
  WinClientMap           m_window_search;
  WindowMap              m_window_search_group;

  // A window is the group leader, which can map to several
  // WinClients in the group, it is *not* shynebox's concept of groups
  // See ICCCM section 4.1.11
  // The group leader (which may not be mapped, so may not have a WinClient)
  // will have it's window being the group index
  std::multimap<Window, WinClient *> m_group_search;

  Time    m_last_time;
  XEvent  m_last_event;

  Window  m_masked;
  ShyneboxWindow *m_masked_window;

  struct {
      BScreen* mouse;
      BScreen* key;
  } m_active_screen;

  Atom m_shynebox_pid;

  bool m_reconfigure_wait;
  char **m_argv;
  int m_argc;
  std::string m_restart_argument; // what to restart

  // when we execute reconfig command we must wait until next event round
  tk::Timer m_reconfig_timer;
  tk::Timer m_key_reload_timer;
  bool m_showing_dialog;

  struct {
      bool starting;
      bool restarting;
      bool shutdown;
  } m_state;

  int m_server_grabs;
};
#endif // SHYNEBOX_HH

// Copyright (c) 2023 Shynebox - zlice
//
// Copyright (c) 2001 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
//
// blackbox.hh for Blackbox - an X11 Window manager
// Copyright (c) 1997 - 2000 Brad Hughes (bhughes at tcac.net)
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
