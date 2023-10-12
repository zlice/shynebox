// Screen.hh for Shynebox Window Manager

/*
  Manages workspaces, heads (monitors), creates window, menus and
  callbacks for most config items.
  This is where most of the magic happens or can be traced back to.
*/

#ifndef SCREEN_HH
#define SCREEN_HH

#include "SbRootWindow.hh"
#include "RootTheme.hh"
#include "WinButtonTheme.hh"  // themeproxy cast
#include "SbWinFrameTheme.hh" // themeproxy cast

#include "tk/EventHandler.hh"
#include "tk/Config.hh"
#include "tk/Timer.hh"

#if USE_TOOLBAR
#include "Toolbar.hh"
#else
#include "tk/Menu.hh"
#include "FocusControl.hh"
#include "WinButton.hh"
#endif

#include <list>
#include <vector>
#include <fstream>
#include <map>

class ClientPattern;
class SbMenu;
class WorkspaceMenu;
class Focusable;
class ShyneboxWindow;
class WinClient;
class Workspace;
class Strut;
class HeadArea;
class ScreenPlacement;
class TooltipWindow;
class OSDWindow;

namespace tk {
class MenuTheme;
class LayerManager;
class ImageControl;
class LayerItem;
class TextButton;
}

using std::string;
using std::vector;

typedef std::map<string, tk::TextButton*> ToolButtonMap;

// Handles screen connection, screen clients and workspaces
// Create workspaces, handles switching between workspaces and windows
class BScreen: public tk::EventHandler {
public:
  typedef std::list<ShyneboxWindow *> Icons;
  typedef vector<Workspace *> Workspaces;
  typedef vector<string> WorkspaceNames;

  BScreen(tk::ConfigManager &cm,
          int scrn, int num_layers, unsigned int opts);
  ~BScreen();

  bool isShuttingdown() const { return m_state.shutdown; }
  bool isRestart();
  void shutdown();
  void initWindows();

  // config items
  bool isWorkspaceWarpingHorizontal() const { return *m_cfgmap["workspaceWarpingHorizontal"]; }
  bool isWorkspaceWarpingVertical() const { return *m_cfgmap["workspaceWarpingVertical"]; }
  bool doAutoRaise() const { return *m_cfgmap["autoRaiseWindows"]; }
  bool clickRaisesWindows() const { return *m_cfgmap["clickRaisesWindows"]; }
  bool doOpaqueMove() const { return *m_cfgmap["opaqueMove"]; }
  bool doOpaqueResize() const { return *m_cfgmap["opaqueResize"]; }
  bool doFullMax() const { return *m_cfgmap["fullMaximization"]; }
  bool getMaxIgnoreIncrement() const { return *m_cfgmap["maxIgnoreIncrement"]; }
  bool getMaxDisableMove() const { return *m_cfgmap["maxDisableMove"]; }
  bool getMaxDisableResize() const { return *m_cfgmap["maxDisableResize"]; }
  bool doShowWindowPos() const { return *m_cfgmap["showWindowPosition"]; }
  bool clientMenuUsePixmap() const { return *m_cfgmap["clientMenu.usePixmap"]; }
  bool getDefaultInternalTabs() const { return *m_cfgmap["tabs.inTitlebar"]; }
  bool getTabsUsePixmap() const { return *m_cfgmap["tabs.usePixmap"]; }
  bool getMaxOverTabs() const { return *m_cfgmap["tabs.maxOver"]; }
  bool doObeyHeads() const { return *m_cfgmap["obeyHeads"]; }
  const string &defaultDeco() const { return *m_cfgmap["defaultDeco"]; }
  unsigned int opaqueResizeDelay() const { return *m_cfgmap["opaqueResizeDelay"]; }
  unsigned int getTabWidth() const { return *m_cfgmap["tabs.width"]; }
  unsigned int noFocusWhileTypingDelay() const { return *m_cfgmap["noFocusWhileTypingDelay"]; }
  int getEdgeSnapThreshold() const { return *m_cfgmap["edgeSnapThreshold"]; }
  int getEdgeResizeSnapThreshold() const { return *m_cfgmap["edgeResizeSnapThreshold"]; }
  int getWorkspaceWarpingHorizontalOffset() const { return *m_cfgmap["workspaceWarpingHorizontalOffset"]; }
  int getWorkspaceWarpingVerticalOffset() const { return *m_cfgmap["workspaceWarpingVerticalOffset"]; }
  tk::TabPlacement_e getTabPlacement() const { return (tk::TabPlacement_e)(int)((tk::strnum)*m_cfgmap["tabs.placement"]); }
  void saveTabPlacement(tk::TabPlacement_e place) { (tk::strnum&)(*m_cfgmap["tabs.placement"]) = (int)place; }
  void updateActiveWorkspaceCount(int w) { *m_cfgmap["workspaces"] = w; }

  // related to cfg for titlebar, but getters for private vars
  vector<WinButton::Type> &titlebar_left() { return m_titlebar_left; }
  vector<WinButton::Type> &titlebar_right() { return m_titlebar_right; }

  unsigned int width() const { return rootWindow().width(); }
  unsigned int height() const { return rootWindow().height(); }
  int screenNumber() const { return rootWindow().screenNumber(); }
  void setRootColormapInstalled(bool r) { root_colormap_installed = r; }

  bool isRootColormapInstalled() const { return root_colormap_installed; }
  bool isScreenManaged() const { return m_state.managed; }
  tk::ImageControl &imageControl() { return *m_image_control; }

  // menus
  const SbMenu &rootMenu() const { return *m_rootmenu; }
  SbMenu &rootMenu() { return *m_rootmenu; }
  const SbMenu &configMenu() const { return *m_configmenu; }
  SbMenu &configMenu() { return *m_configmenu; }
  const SbMenu &windowMenu() const { return *m_windowmenu; }
  SbMenu &windowMenu() { return *m_windowmenu; }

  // themes
  tk::ThemeProxy<SbWinFrameTheme> &focusedWinFrameTheme() { return *m_focused_windowtheme; }
  const tk::ThemeProxy<SbWinFrameTheme> &focusedWinFrameTheme() const { return *m_focused_windowtheme; }
  tk::ThemeProxy<SbWinFrameTheme> &unfocusedWinFrameTheme() { return *m_unfocused_windowtheme; }
  const tk::ThemeProxy<SbWinFrameTheme> &unfocusedWinFrameTheme() const { return *m_unfocused_windowtheme; }

  tk::ThemeProxy<tk::MenuTheme> &menuTheme() { return *m_menutheme; }
  const tk::ThemeProxy<tk::MenuTheme> &menuTheme() const { return *m_menutheme; }
  const tk::ThemeProxy<RootTheme> &rootTheme() const { return *m_root_theme; }
  tk::ThemeProxy<RootTheme> &rootTheme() { return *m_root_theme; }

  tk::ThemeProxy<WinButtonTheme> &focusedWinButtonTheme() { return *m_focused_winbutton_theme; }
  const tk::ThemeProxy<WinButtonTheme> &focusedWinButtonTheme() const { return *m_focused_winbutton_theme; }
  tk::ThemeProxy<WinButtonTheme> &unfocusedWinButtonTheme() { return *m_unfocused_winbutton_theme; }
  const tk::ThemeProxy<WinButtonTheme> &unfocusedWinButtonTheme() const { return *m_unfocused_winbutton_theme; }
  tk::ThemeProxy<WinButtonTheme> &pressedWinButtonTheme() { return *m_pressed_winbutton_theme; }
  const tk::ThemeProxy<WinButtonTheme> &pressedWinButtonTheme() const { return *m_pressed_winbutton_theme; }

  SbRootWindow &rootWindow() { return m_root_window; }
  const SbRootWindow &rootWindow() const { return m_root_window; }

  tk::LayerManager &layerManager() { return *m_layermanager; }
  const tk::LayerManager &layerManager() const { return *m_layermanager; }

  ScreenPlacement &placementStrategy() { return *m_placement_strategy; }
  const ScreenPlacement &placementStrategy() const { return *m_placement_strategy; }

  // list of all icons/minimized windows across all workspaces
  // used for workspace commands and menu
  const Icons &iconList() const { return m_icon_list; }
  Icons &iconList() { return m_icon_list; }

  void addTemporalMenu(SbMenu *menu);
  void updateClientMenus(); // temporal and workspace menus

  // workspaces
  Workspace *getWorkspace(unsigned int w);
  const Workspace *getWorkspace(unsigned int w) const;
  unsigned int getWorkspaceID(Workspace &w);
  const unsigned int getWorkspaceID(Workspace &w) const;
  void updateWorkspaceName(unsigned int w); // may save config
  void addWorkspaceName(const char *name);
  void addWorkspace();
  void removeLastWorkspace();
  // move workspaces
  void nextWorkspace(int delta = 1);
  void prevWorkspace(int delta = 1);
  // ^-these wrap
  // v-these dont
  void rightWorkspace(int delta);
  void leftWorkspace(int delta);
  void changeWorkspaceID(unsigned int, bool revert = true);
  void sendToWorkspace(unsigned int workspace, ShyneboxWindow *win=0,
                       bool changeworkspace=true);
  string getNameOfWorkspace(unsigned int workspace) const;

  unsigned int currentWorkspaceID() { return m_current_wsid; }
  const unsigned int currentWorkspaceID() const { return m_current_wsid; }
  Workspace *currentWorkspace() { return m_current_workspace; }
  const Workspace *currentWorkspace() const { return m_current_workspace; }
  WorkspaceMenu &workspaceMenu() { return *m_workspacemenu; }
  const WorkspaceMenu &workspaceMenu() const { return *m_workspacemenu; }
  FocusControl &focusControl() { return *m_focus_control; }
  const FocusControl &focusControl() const { return *m_focus_control; }
  size_t numberOfWorkspaces() const { return m_workspaces_list.size(); }
  const Workspaces &getWorkspacesList() const { return m_workspaces_list; }
  Workspaces &getWorkspacesList() { return m_workspaces_list; }
  const WorkspaceNames &getWorkspaceNames() const { return m_workspace_names; }

  // event handlers
  void keyPressEvent(XKeyEvent &ke);
  void keyReleaseEvent(XKeyEvent &ke);
  void buttonPressEvent(XButtonEvent &be);

  // Cycles focus of windows
  void cycleFocus(int opts = 0, const ClientPattern *pat = 0, bool reverse = false);
  bool isCycling() const { return m_state.cycling; }

  void reconfigure();
  void reconfigThemes();
  void reconfigureStruts();

  void addIcon(ShyneboxWindow *win);
  void removeIcon(ShyneboxWindow *win);
  void removeWindow(ShyneboxWindow *win);
  void removeClient(WinClient &client);

  bool isKdeDockapp(Window win) const;
  bool addKdeDockapp(Window win); // true if added

  // create window frame for client window and attach it
  void createWindow(Window clientwin);
  // creates a window frame for a winclient. The client is attached to the window
  void createWindow(WinClient &client);
  void reassociateWindow(ShyneboxWindow *window, unsigned int workspace_id,
                         bool ignore_sticky);
  void focusedWinFrameThemeReconfigured();

#ifdef USE_TOOLBAR
  Toolbar *toolbar() { return m_toolbar; }
  const Toolbar *toolbar() const { return m_toolbar; }

  void clearToolButtonMap();
  void mapToolButton(string name, tk::TextButton *button);
  void systrayCheckClientMsg(const XClientMessageEvent &ce,
                                    BScreen *screen, WinClient *const winclient);
  void setupSystrayClient(WinClient &winclient);
  void recfgToolbar();
  void resetToolbar(); // bad name, updates wsname tag and resets iconbar
  void updateIconbar();
  void updateToolbar(bool everything=true);
#endif // USE_TOOLBAR
  void updateWSItems(bool everything=false);

  void initMenus();
  void rereadMenu();
  const string windowMenuFilename() const;
  void rereadWindowMenu();
  void addConfigMenu(const tk::SbString &label, tk::Menu &menu);
  void removeConfigMenu(tk::Menu &menu);

  int getGap(int head, const char type);
  int calRelativeSize(int head, int i, char type);
  int calRelativeWidth(int head, int i);
  int calRelativeHeight(int head, int i);
  int calRelativePosition(int head, int i, char type);
  int calRelativePositionWidth(int head, int i);
  int calRelativePositionHeight(int head, int i);
  int calRelativeDimension(int head, int i, char type);
  int calRelativeDimensionWidth(int head, int i);
  int calRelativeDimensionHeight(int head, int i);
  float getXGap(int head);
  float getYGap(int head);

  // show position window centered on the screen with "X x Y" text
  void showPosition(int x, int y);
  void hidePosition();
  // show geomentry with "width x height"-text, not size of window
  void showGeometry(unsigned int width, unsigned int height);
  void hideGeometry();
  void showTooltip(const tk::BiDiString &text);
  void hideTooltip();

  TooltipWindow& tooltipWindow() { return *m_tooltip_window; }

  // grouping - we want ordering, so we can either search for a
  // group to the left, or to the right (they'll be different if
  // they exist).
  WinClient *findGroupLeft(WinClient &winclient);
  WinClient *findGroupRight(WinClient &winclient);

  void configRandr(); // rebuild randr info on startup or events
  // updates root window size and resizes/reconfigures xrandr
  void updateSize();

  unsigned int maxLeft(int head, bool ignore_struts=false) const;
  unsigned int maxRight(int head, bool ignore_struts=false) const;
  unsigned int maxTop(int head, bool ignore_struts=false) const;
  unsigned int maxBottom(int head, bool ignore_struts=false) const;

  // request workspace space, i.e "don't maximize over this area"
  Strut *requestStrut(int head, int left, int right, int top, int bottom);
  // remove requested space and destroy strut
  void clearStrut(Strut *strut);
  // updates max avaible area for the workspace
  void updateAvailableWorkspaceArea();

  int numHeads() const { return m_heads.size(); }
  int getHead(int x, int y) const;
  int getHead(const tk::SbWindow &win) const;
  int getCurHead() const; // where mouse is

  // get head (read 'monitor') dimensions
  int getHeadX(int head) const; // start pos
  int getHeadY(int head) const;
  int getHeadWidth(int head) const;
  int getHeadHeight(int head) const;

  // fit point or box to head
  void fitToHead(int head, int &x, int &y,
                   int w = 0, int h = 0, int bw = 0,
                   bool respect_struts = false) const;

  tk::CFGMAP &m_cfgmap;

private:
  tk::Timer m_cycle_timer,
            m_bg_timer;
  bool m_cycle_lock = false;
  void reconfigBGTimer() { m_root_theme->reconfigTheme(); }
  void unlockCycleTimer() { m_cycle_lock = false; }

  void setupConfigmenu(tk::Menu &menu);
  void renderGeomWindow();
  void renderPosWindow();

  const Strut* availableWorkspaceArea(int head) const;

  tk::LayerManager *m_layermanager = 0;

  bool root_colormap_installed;

  tk::ImageControl *m_image_control = 0;
  // TODO: multi-screen config menus no longer exist
  //       should make this static just in case?
  SbMenu *m_configmenu = 0, *m_rootmenu = 0,
         *m_windowmenu = 0, *m_temporalmenu = 0;
  WorkspaceMenu *m_workspacemenu = 0;

  Icons m_icon_list;

#if USE_TOOLBAR
  Toolbar *m_toolbar = 0;
  ToolButtonMap *m_toolButtonMap = 0;
#endif

  unsigned int m_current_wsid = 0;
  Workspace *m_current_workspace,
            *m_former_workspace;

  WorkspaceNames m_workspace_names;
  Workspaces m_workspaces_list;

  SbWinFrameTheme *m_focused_windowtheme = 0,
                  *m_unfocused_windowtheme = 0;
  WinButtonTheme  *m_focused_winbutton_theme = 0,
                  *m_unfocused_winbutton_theme = 0,
                  *m_pressed_winbutton_theme = 0;
  tk::MenuTheme *m_menutheme = 0;
  RootTheme *m_root_theme = 0;

  SbRootWindow m_root_window;
  OSDWindow *m_geom_window = 0,
            *m_pos_window = 0;
  TooltipWindow *m_tooltip_window = 0;

  // config items - refs set on init
  int &m_workspace_cnt, // workspace logic goes below 0
      &m_menu_delay;
  unsigned int &m_tooltip_delay;
  string &m_titlebar_left_str, &m_titlebar_right_str;

  FocusControl *m_focus_control = 0;
  ScreenPlacement *m_placement_strategy = 0;

  // This is a map of windows to clients for clients that had a left
  // window set, but that window wasn't present at the time
  typedef std::map<Window, WinClient *> Groupables;
  Groupables m_expecting_groups;

  // requsted reserved areas
  vector<HeadArea*> m_head_areas;
  vector<Strut*> m_custom_struts;

  vector<WinButton::Type> m_titlebar_left;
  vector<WinButton::Type> m_titlebar_right;

  struct {
    bool cycling;
    bool restart;
    bool shutdown;
    bool managed;
  } m_state;

  // multi-monitor randr
  // stripped down XRRMonitorInfo
  struct randr_head_info {
    int _x, _y, _width, _height;
    int x() const { return _x; }
    int y() const { return _y; }
    int width() const { return _width; }
    int height() const { return _height; }
  };

  vector<randr_head_info> m_heads;

  unsigned int m_opts; // for command line disable - Shynebox::OPT_TOOLBAR
};

#endif // SCREEN_HH

// Copyright (c) 2023 Shynebox - zlice
//
// Copyright (c) 2001 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
//
// Screen.hh for Blackbox - an X11 Window manager
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
