// Screen.cc for Shynebox Window Manager

#include "Screen.hh"

#include "shynebox.hh"
#include "Keys.hh"
#include "Window.hh"
#include "Workspace.hh"
#include "FocusControl.hh"
#include "ScreenPlacement.hh"

// menus
#include "ConfigMenu.hh" // SbMenu.hh
#include "WorkspaceMenu.hh"
#include "SendToMenu.hh"
#include "MenuCreator.hh"

#include "WinClient.hh"
#include "SbWinFrame.hh"
#include "Strut.hh"
#include "HeadArea.hh"
#include "SbCommands.hh"
#include "RectangleUtil.hh"
#include "TooltipWindow.hh"

#include "Debug.hh"

#include "tk/EventManager.hh"
#include "tk/I18n.hh"
#include "tk/ImageControl.hh"
#include "tk/KeyUtil.hh"
#include "tk/LayerItem.hh"
#include "tk/LayerManager.hh"
#include "tk/MacroCommand.hh"
#include "tk/RelCalcHelper.hh"
#include "tk/SbWindow.hh"
#include "tk/SimpleCommand.hh"
#include "tk/StringUtil.hh"

#if USE_TOOLBAR
#include "SystemTray.hh"
#include "Toolbar.hh"
#else
class Toolbar {};
#endif

#ifdef STDC_HEADERS
#include <sys/types.h>
#endif

#ifdef HAVE_UNISTD_H
#include <sys/types.h>
#include <unistd.h>
#endif

#ifdef TIME_WITH_SYS_TIME
  #include <sys/time.h>
  #include <time.h>
#else
#ifdef HAVE_SYS_TIME_H
  #include <sys/time.h>
#else
  #include <time.h>
#endif // HAVE_SYS_TIME_H
#endif // TIME_WITH_SYS_TIME

#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <X11/cursorfont.h>

#include <X11/extensions/Xrandr.h>

#include <iostream>
#include <stack>
#include <cstdarg>

using std::cerr;
using std::string;
using std::make_pair;
using std::pair;
using std::list;
using std::vector;

using std::hex;
using std::dec;

static bool running = true;
namespace {

int anotherWMRunning(Display *display, XErrorEvent *) {
  _SB_USES_NLS;
  cerr<<_SB_CONSOLETEXT(Screen, AnotherWMRunning,
                "BScreen::BScreen: an error occured while querying the X server.\n"
                "	another window manager already running on display ",
                "Message when another WM is found already active on all screens")
      <<DisplayString(display)<<"\n";

  running = false;

  return -1;
}

void clampMenuDelay(int& delay) {
  delay = std::clamp(delay, 0, 5000);
}

Atom atom_wm_check = 0;
Atom atom_net_desktop = 0;
Atom atom_utf8_string = 0;
Atom atom_kde_systray = 0;
Atom atom_kwm1 = 0;

void initAtoms(Display* dpy) {
    atom_wm_check = XInternAtom(dpy, "_NET_SUPPORTING_WM_CHECK", False);
    atom_net_desktop = XInternAtom(dpy, "_NET_CURRENT_DESKTOP", False);
    atom_utf8_string = XInternAtom(dpy, "UTF8_STRING", False);
    atom_kde_systray = XInternAtom(dpy, "_KDE_NET_WM_SYSTEM_TRAY_WINDOW_FOR", False);
    atom_kwm1 = XInternAtom(dpy, "KWM_DOCKWINDOW", False);
}

} // end anonymous namespace



BScreen::BScreen(tk::ConfigManager &cm,
                 int scrn, int num_layers,
                 unsigned int opts) :
      m_layermanager(new tk::LayerManager(num_layers) ),
      root_colormap_installed(false),
      m_current_workspace(0),
      m_former_workspace(0),
      m_focused_windowtheme(new SbWinFrameTheme(scrn, ".focus") ),
      m_unfocused_windowtheme(new SbWinFrameTheme(scrn, ".unfocus") ),
      // the order of windowtheme and winbutton theme is important
      // because winbutton need to rescale the pixmaps in winbutton theme
      // after sbwinframe have resized them
      m_focused_winbutton_theme(new WinButtonTheme(scrn, "", *m_focused_windowtheme) ),
      m_unfocused_winbutton_theme(new WinButtonTheme(scrn, ".unfocus", *m_unfocused_windowtheme) ),
      m_pressed_winbutton_theme(new WinButtonTheme(scrn, ".pressed", *m_focused_windowtheme) ),
      m_menutheme(new tk::MenuTheme(scrn) ),
      m_root_window(scrn),
      m_geom_window(new OSDWindow(m_root_window, *this, *m_focused_windowtheme) ),
      m_pos_window(new OSDWindow(m_root_window, *this, *m_focused_windowtheme) ),
      m_tooltip_window(new TooltipWindow(m_root_window, *this, *m_focused_windowtheme) ),
      m_cfgmap(cm.get_cfgmap() ),
      m_workspace_cnt(*m_cfgmap["workspaces"]),
      m_menu_delay(*m_cfgmap["menuDelay"]),
      m_tooltip_delay(*m_cfgmap["tooltipDelay"]),
      m_titlebar_left_str(*m_cfgmap["titlebar.left"]),
      m_titlebar_right_str(*m_cfgmap["titlebar.right"]),
      m_focus_control(new FocusControl(*this) ),
      m_placement_strategy(new ScreenPlacement(*this) ),
      m_opts(opts) {
  m_cycle_timer.setTimeout(10 * tk::SbTime::IN_MILLISECONDS); // 10 millisec
  m_cycle_timer.fireOnce(true);
  tk::SimpleCommand<BScreen> *cycle_time_cmd(new tk::SimpleCommand<BScreen>(*this, &BScreen::unlockCycleTimer) );
  m_cycle_timer.setCommand(*cycle_time_cmd);

  m_bg_timer.setTimeout(10 * tk::SbTime::IN_MILLISECONDS); // 10 millisec
  m_bg_timer.fireOnce(true);
  tk::SimpleCommand<BScreen> *bg_time_cmd(new tk::SimpleCommand<BScreen>(*this, &BScreen::reconfigBGTimer) );
  m_bg_timer.setCommand(*bg_time_cmd);

  m_state.cycling = false;
  m_state.restart = false;
  m_state.shutdown = false;
  m_state.managed = false;

  Shynebox *shynebox = Shynebox::instance();
  Display *disp = shynebox->display();

  initAtoms(disp);

  // create default 'all heads' area
  m_head_areas.resize(1);
  m_head_areas[0] = new HeadArea();

  configRandr(); // setup monitor areas

  // setup error handler to catch "screen already managed by other wm"
  XErrorHandler old = XSetErrorHandler((XErrorHandler) anotherWMRunning);

  rootWindow().setEventMask(ColormapChangeMask | EnterWindowMask | PropertyChangeMask |
                            SubstructureRedirectMask | KeyPressMask | KeyReleaseMask |
                            ButtonPressMask | ButtonReleaseMask| SubstructureNotifyMask);
  shynebox->sync(false);

  XSetErrorHandler((XErrorHandler) old);

  m_state.managed = running;
  if (!m_state.managed) {
    delete m_placement_strategy; m_placement_strategy = 0;
    delete m_focus_control; m_focus_control = 0;
    return;
  }

  // we're going to manage the screen, so now add our pid
#ifdef HAVE_GETPID
  unsigned long bpid = static_cast<unsigned long>(getpid() );

  rootWindow().changeProperty(shynebox->getShyneboxPidAtom(), XA_CARDINAL,
                              sizeof(pid_t) * 8, PropModeReplace,
                              (unsigned char *) &bpid, 1);
#endif

  // check if we're the first EWMH compliant window manager on this screen
  union { Atom atom; unsigned long ul; int i; } ignore;
  unsigned char *ret_prop;
  if (rootWindow().property(atom_wm_check, 0l, 1l,
          False, XA_WINDOW, &ignore.atom, &ignore.i, &ignore.ul,
          &ignore.ul, &ret_prop) ) {
    m_state.restart = (ret_prop != NULL);
    XFree(ret_prop);
  }

// setup RANDR for this screens root window
  int randr_mask = RRScreenChangeNotifyMask;
# ifdef RRCrtcChangeNotifyMask
  randr_mask |= RRCrtcChangeNotifyMask;
# endif
# ifdef RROutputChangeNotifyMask
  randr_mask |= RROutputChangeNotifyMask;
# endif
# ifdef RROutputPropertyNotifyMask
  randr_mask |= RROutputPropertyNotifyMask;
# endif
  XRRSelectInput(disp, rootWindow().window(), randr_mask);

  _SB_USES_NLS;

#ifdef DEBUG
  cerr << "BScreen::BScreen: managing screen " << screenNumber()
       << "using visual 0x" << std::hex << std::uppercase
       << XVisualIDFromVisual(rootWindow().visual() ) << dec
       << std::nouppercase << " depth " << rootWindow().maxDepth() << "\n";
  // NLS
  //fprintf(stderr, _SB_CONSOLETEXT(Screen, ManagingScreen,
  //                        "BScreen::BScreen: managing screen %d "
  //                        "using visual 0x%lx, depth %d\n",
  //                        "informational message saying screen number (%d), visual (%lx), and colour depth (%d)").c_str(),
  //        screenNumber(), XVisualIDFromVisual(rootWindow().visual() ),
  //        rootWindow().maxDepth() );
#endif

  tk::EventManager *evm = tk::EventManager::instance();
  evm->add(*this, rootWindow() );
  Keys *keys = shynebox->keys();
  if (keys)
    keys->registerWindow(rootWindow().window(), *this,
                         Keys::GLOBAL|Keys::ON_DESKTOP);
  rootWindow().setCursor(XCreateFontCursor(disp, XC_left_ptr) );

  // initialize workspace names from config
  string cfg_names = *m_cfgmap["workspaceNames"];
  BScreen::WorkspaceNames names;
  tk::StringUtil::stringtok<BScreen::WorkspaceNames>(names, cfg_names, ",");

  size_t n = names.size();

  for (auto &it : names) {
    string tmp_name = it;
    if (it.empty() || it == "")
      string tmp_name = tk::StringUtil::number2String(n+1);
    addWorkspaceName(tmp_name.c_str() );
  }

  // setup image cache engine
  m_image_control = new tk::ImageControl(scrn, shynebox->colorsPerChannel(),
                                         shynebox->getCacheLife(), shynebox->getCacheMax() );
  imageControl().installRootColormap();
  root_colormap_installed = true;

  m_root_theme = new RootTheme(imageControl() );
  m_root_theme->reconfigTheme();

  clampMenuDelay(m_menu_delay);

  m_menutheme->setDelay(m_menu_delay);

  // this was in Window
  // converted to string to move here to do once on startup
  for (auto &tbut : {m_titlebar_left_str, m_titlebar_right_str} ) {
    vector<string> val;
    vector<WinButton::Type> *buts;
    if (tbut == m_titlebar_left_str)
      buts = &m_titlebar_left;
    else
      buts = &m_titlebar_right;

    tk::StringUtil::stringtok(val, tbut);
    buts->clear();

    std::string v;
    for (size_t i = 0; i < val.size(); i++) {
      v = tk::StringUtil::toLower(val[i]);
      if (v == "maximize")
        buts->push_back(WinButton::MAXIMIZE);
      else if (v == "minimize")
        buts->push_back(WinButton::MINIMIZE);
      else if (v == "shade")
        buts->push_back(WinButton::SHADE);
      else if (v == "stick")
        buts->push_back(WinButton::STICK);
      else if (v == "menuicon")
        buts->push_back(WinButton::MENUICON);
      else if (v == "close")
        buts->push_back(WinButton::CLOSE);
    }
  } // for titlebar left/right (buttons)

  renderGeomWindow();
  renderPosWindow();
  m_tooltip_window->setDelay(m_tooltip_delay);

  // setup workspaces and workspace menu
  int nr_ws = m_workspace_cnt;
  addWorkspace(); // at least one
  for (int i = 1; i < nr_ws; ++i)
    addWorkspace();

  m_current_workspace = m_workspaces_list.front();

  m_windowmenu = MenuCreator::createMenu("", *this);
  m_windowmenu->setInternalMenu();
  m_windowmenu->setReloadHelper(new tk::AutoReloadHelper() );
  tk::Command<void> *windowmenu_rh(new tk::SimpleCommand<BScreen>(*this, &BScreen::rereadWindowMenu) );
  m_windowmenu->reloadHelper()->setReloadCmd(*windowmenu_rh);

  m_rootmenu = MenuCreator::createMenu("", *this);
  m_rootmenu->setReloadHelper(new tk::AutoReloadHelper() );
  tk::Command<void> *rootmenu_rh(new tk::SimpleCommand<BScreen>(*this, &BScreen::rereadMenu) );
  m_rootmenu->reloadHelper()->setReloadCmd(*rootmenu_rh);

  m_configmenu = MenuCreator::createMenu(_SB_XTEXT(Menu, Configuration,
                                "Configuration", "Title of configuration menu"), *this);
  m_configmenu->setInternalMenu();
  setupConfigmenu(*m_configmenu);

  // check which desktop we should start on
  int first_desktop = 0;
  if (m_state.restart) {
    bool exists = false;
    int ret = (rootWindow().cardinalProperty(atom_net_desktop, &exists) );
    if (exists) {
      first_desktop = std::clamp<int>(ret, 0, nr_ws - 1);
      m_current_workspace = getWorkspace(first_desktop);
    }
  }

  changeWorkspaceID(first_desktop);

  XFlush(disp);
} // Screen class init

BScreen::~BScreen() {
  if (!m_state.managed)
    return;

  if (m_focused_windowtheme)
    delete m_focused_windowtheme;
  if (m_unfocused_windowtheme)
    delete m_unfocused_windowtheme;
  if (m_focused_winbutton_theme)
    delete m_focused_winbutton_theme;
  if (m_unfocused_winbutton_theme)
    delete m_unfocused_winbutton_theme;
  if (m_pressed_winbutton_theme)
    delete m_pressed_winbutton_theme;

#if USE_TOOLBAR
  if (m_toolbar)
    delete m_toolbar;
  if (m_toolButtonMap)
    delete m_toolButtonMap;
#endif
  tk::EventManager *evm = tk::EventManager::instance();
  evm->remove(rootWindow() );

  Keys *keys = Shynebox::instance()->keys();
  if (keys)
    keys->unregisterWindow(rootWindow().window() );

// do root menu in init, not here, lest le crash
  if (m_rootmenu) {
    delete m_rootmenu;
    m_rootmenu = 0;
  }

  // rootmenu can hold configmenu
  // but 'setInternalMenu' should prevent double delete
  if (m_configmenu)
    delete m_configmenu;

  if (m_windowmenu)
    delete m_windowmenu;

  if (m_root_theme)
    delete m_root_theme;

  if (m_menutheme)
    delete m_menutheme;

  // Since workspacemenu holds client list menus (from workspace)
  // we need to destroy it before we destroy workspaces
  if (m_workspacemenu)
    delete m_workspacemenu;

  m_workspace_names.clear();

  for (auto it : m_workspaces_list)
    delete it;
  m_workspaces_list.clear();

  while (!m_icon_list.empty() ) {
    removeWindow(m_icon_list.back() );
    m_icon_list.back()->restore(true);
    delete (m_icon_list.back() );
    m_icon_list.pop_back();
  }

  for (auto it : m_custom_struts)
    clearStrut(it);

  for (size_t i = 0; i < m_head_areas.size(); i++)
    delete m_head_areas[i];

  if (m_geom_window)
    delete m_geom_window;
  if (m_pos_window)
    delete m_pos_window;
  if (m_tooltip_window)
    delete m_tooltip_window;

  if (m_image_control)
    delete m_image_control;

  delete m_focus_control;
  delete m_placement_strategy;
  delete m_layermanager;
} // Screen class destroy

bool BScreen::isRestart() {
  return Shynebox::instance()->isStartup() && m_state.restart;
}

void BScreen::shutdown() {
  rootWindow().setEventMask(NoEventMask);
  tk::App::instance()->sync(false);
  m_state.shutdown = true;
  m_focus_control->shutdown();
  for (auto it : m_workspaces_list)
    it->shutdown();
}

void BScreen::initWindows() {
#if USE_TOOLBAR
  // TOOLBAR CREATION
  if (m_opts & Shynebox::OPT_TOOLBAR) {
    m_toolButtonMap = new ToolButtonMap();
    Toolbar* tb = new Toolbar(*this, *layerManager().getLayer((int)tk::ResLayers_e::NORMAL) );
    m_toolbar = tb;
  }
#endif

  unsigned int nchild;
  Window r, p, *children;
  Shynebox* shynebox = Shynebox::instance();
  Display* disp = shynebox->display();

  XQueryTree(disp, rootWindow().window(), &r, &p, &children, &nchild);

  // preen the window list of all icon windows... for better dockapp support
  for (unsigned int i = 0; i < nchild; i++) {
    if (children[i] == None)
      continue;

    XWMHints *wmhints = XGetWMHints(disp, children[i]);

    if (wmhints
        && (wmhints->flags & IconWindowHint)
        && wmhints->icon_window != children[i] ) {
      for (unsigned int j = 0; j < nchild; j++) {
        if (children[j] == wmhints->icon_window) {
          sbdbg<<"BScreen::initWindows(): children[j] = 0x"<<hex<<children[j]<<dec<<"\n";
          sbdbg<<"BScreen::initWindows(): = icon_window\n";

          children[j] = None;
          break;
        } // if icon_window
      } // for nchild
      XFree(wmhints);
    } // if wmhints && flags & IconWindowHint && icon_window != child[i]
  } // for nchild

  // manage shown windows
  Window transient_for = 0;
  bool safety_flag = false;
  unsigned int num_transients = 0;

  for (unsigned int i = 0; i <= nchild; ++i) {
    if (i == nchild) {
      if (num_transients) {
        if (num_transients == nchild)
         safety_flag = true;
        nchild = num_transients;
        i = num_transients = 0;
      } else
        break; // if no transients
    } // if loop is nchild

    if (children[i] == None)
      continue;
    else if (!shynebox->validateWindow(children[i]) ) {
      sbdbg<<"BScreen::initWindows(): not valid window = "<<hex<<children[i]<<dec<<"\n";
      children[i] = None;
      continue;
    }

    // if we have a transient_for window and it isn't created yet...
    // postpone creation of this window until after all others
    if (XGetTransientForHint(disp, children[i], &transient_for)
        && shynebox->searchWindow(transient_for) == 0 && !safety_flag) {
      // add this window back to the beginning of the list of children
      children[num_transients] = children[i];
      num_transients++;

      sbdbg<<"BScreen::initWindows(): postpone creation of 0x"<<hex<<children[i]<<dec<<"\n";
      sbdbg<<"BScreen::initWindows(): transient_for = 0x"<<hex<<transient_for<<dec<<"\n";

      continue;
    } // if GetTransientForHint and not transient postpone

    XWindowAttributes attrib;
    if (XGetWindowAttributes(disp, children[i], &attrib) ) {
      if (attrib.override_redirect) {
        children[i] = None; // we dont need this anymore, since we already created a window for it
        continue;
      }

      if (attrib.map_state != IsUnmapped)
        createWindow(children[i]);
    }
    children[i] = None; // we dont need this anymore, since we already created a window for it
  } // for nchild

  XFree(children);
#if USE_TOOLBAR
  resetToolbar();      // update after window creation for restarts
  updateToolbar(true); // keeps existing windows in iconbar in order
#endif
} // initWindows

void BScreen::addTemporalMenu(SbMenu *menu) {
  m_temporalmenu = menu; // current active command created menu
}

void BScreen::updateClientMenus() {
  if (m_temporalmenu) {
    if (m_temporalmenu->isVisible() )
      m_temporalmenu->updateMenu();
    else
      m_temporalmenu = 0; // will reset in a SbCommand
  }
  for (auto ws : m_workspaces_list)
    ws->updateClientmenu();
  m_workspacemenu->updateIconMenu();
}

// start workspace commands
Workspace *BScreen::getWorkspace(unsigned int w) {
  return ( w < m_workspaces_list.size() ? m_workspaces_list[w] : 0);
}

const Workspace *BScreen::getWorkspace(unsigned int w) const {
  return (w < m_workspaces_list.size() ? m_workspaces_list[w] : 0);
}

unsigned int BScreen::getWorkspaceID(Workspace &w) {
  unsigned int i = m_workspaces_list.size() - 1;
  for (; i > 0 ; i--)
    if (m_workspaces_list[i] == &w)
      break;
  return i;
}

const unsigned int BScreen::getWorkspaceID(Workspace &w) const {
  unsigned int i = m_workspaces_list.size() - 1;
  for (; i > 0 ; i--)
    if (m_workspaces_list[i] == &w)
      break;
  return i;
}

void BScreen::updateWorkspaceName(unsigned int w) {
  Workspace *space = getWorkspace(w);
  if (space) {
    bool name_changed = m_workspace_names[w] != space->name();
    m_workspace_names[w] = space->name();

#if USE_TOOLBAR
    updateWSItems();
#endif

    if (!Shynebox::instance()->isStartup() && name_changed) {
      // update EWMH
      Shynebox::instance()->workspaceNamesChanged(*this);

      string new_names = "";
      for (auto n : m_workspace_names)
        new_names += n + ",";
      *m_cfgmap["workspaceNames"] = new_names;
      Shynebox::instance()->save_rc();
    }

  } // if workspace exist
}

void BScreen::addWorkspaceName(const char *name) {
  m_workspace_names.push_back(tk::SbStringUtil::LocaleStrToSb(name) );
  Workspace *wkspc = getWorkspace(m_workspace_names.size()-1);
  if (wkspc)
    wkspc->setName(m_workspace_names.back() );
  if (!Shynebox::instance()->isStartup() )
    Shynebox::instance()->workspaceNamesChanged(*this);
}

void BScreen::addWorkspace() {
  std::string name = getNameOfWorkspace(m_workspaces_list.size() );
  bool unnamed_ws = name == "";
  Workspace *ws = new Workspace(*this, name, m_workspaces_list.size() );
  m_workspaces_list.push_back(ws);

  if (unnamed_ws)
    addWorkspaceName(ws->name().c_str() );

  updateActiveWorkspaceCount(m_workspaces_list.size() );
  // calls save_rc (cfg) on name *changes*
  updateWorkspaceName(m_workspaces_list.size() - 1);

  // rebuilds menuitems from workspace's clientmenu
  if (m_workspacemenu)
    m_workspacemenu->workspaceInfoChanged(*this);

#if USE_TOOLBAR
  updateWSItems();
  updateToolbar(true);
#endif
  // already called if name change happened
  if (!unnamed_ws && !Shynebox::instance()->isStartup() )
    Shynebox::instance()->workspaceNamesChanged(*this);
} // addWorkspace

void BScreen::removeLastWorkspace() {
  if (m_workspaces_list.size() <= 1)
    return;

  Workspace *wkspc = m_workspaces_list.back();

  unsigned int ws_id = getWorkspaceID(*wkspc);

  if (m_current_wsid == ws_id)
    changeWorkspaceID(m_current_wsid - 1);

  if (m_former_workspace && getWorkspaceID(*m_former_workspace) == ws_id)
    m_former_workspace = 0;

  wkspc->removeAll(ws_id - 1);

  for (auto it : iconList() )
    if (it->workspaceNumber() == ws_id)
      it->setWorkspace(ws_id - 1);

  Shynebox::instance()->clientListChanged(*this);

  // remove last workspace
  m_workspaces_list.pop_back();

  updateActiveWorkspaceCount(m_workspaces_list.size() );

  // rebuilds menuitems from workspace's clientmenu
  // deletes non-owned menus
  // needs to happen before delete wkspc ahead
  if (m_workspacemenu)
    m_workspacemenu->workspaceInfoChanged(*this);

  Shynebox::instance()->workspaceCountChanged(*this);
#if USE_TOOLBAR
  updateToolbar(true);
#endif

  delete wkspc;

  if (!Shynebox::instance()->isStartup() )
    Shynebox::instance()->workspaceNamesChanged(*this);
} // removeLastWorkspace

// Goes to the workspace "right" of the current
void BScreen::nextWorkspace(int delta) {
  focusControl().stopCyclingFocus();
  if (delta)
    changeWorkspaceID( (m_current_wsid + delta) % numberOfWorkspaces() );
  else if (m_former_workspace)
    changeWorkspaceID(getWorkspaceID(*m_former_workspace) );
}

// Goes to the workspace "left" of the current
void BScreen::prevWorkspace(int delta) {
  focusControl().stopCyclingFocus();
  if (delta)
    changeWorkspaceID( (static_cast<signed>(numberOfWorkspaces() )
    + m_current_wsid - (delta % numberOfWorkspaces() ) ) % numberOfWorkspaces() );
  else if (m_former_workspace)
    changeWorkspaceID(getWorkspaceID(*m_former_workspace) );
}

// ^-these wrap
// v-these dont

// Goes to the workspace "right" of the current
void BScreen::rightWorkspace(int delta) {
  focusControl().stopCyclingFocus();
  if (m_current_wsid + delta < numberOfWorkspaces() )
    changeWorkspaceID(m_current_wsid + delta);
}

// Goes to the workspace "left" of the current
void BScreen::leftWorkspace(int delta) {
  focusControl().stopCyclingFocus();
  if (m_current_wsid >= static_cast<unsigned int>(delta) )
    changeWorkspaceID(m_current_wsid - delta);
}

void BScreen::changeWorkspaceID(unsigned int id, bool revert) {
  if (!m_current_workspace || id >= m_workspaces_list.size()
      || id == m_current_wsid)
    return;

  m_former_workspace = m_current_workspace;

  // Ignore all EnterNotify events until the pointer actually moves
  this->focusControl().ignoreAtPointer();

  tk::App::instance()->sync(false);
  Shynebox::instance()->grab();

  ShyneboxWindow *focused = FocusControl::focusedSbWindow();

  // don't reassociate if not opaque moving
  if (focused && focused->isMoving() && doOpaqueMove() )
    reassociateWindow(focused, id, true);

  // set new workspace
  Workspace *old = m_current_workspace;
  m_current_workspace = getWorkspace(id);
  m_current_wsid = id;

  // we show new workspace first in order to appear faster
  currentWorkspace()->showAll();

  // reassociate all windows that are stuck to the new workspace
  Workspace::Windows wins = old->windowList(); // specifies type for 'auto'
  for (auto it : wins)
    if (it->isStuck() )
      reassociateWindow(it, id, true);

  // change workspace ID of stuck iconified windows, too
  for (auto it : iconList() )
    if (it->isStuck() ) {
      getWorkspace(id)->addWindow(*it);
      it->setWorkspace(id);
    }

  if (focused && focused->isMoving() && doOpaqueMove() )
    focused->focus();
  else if (revert)
    FocusControl::revertFocus(*this);

  old->hideAll(false);

  Shynebox::instance()->ungrab();
  tk::App::instance()->sync(false);

#if USE_TOOLBAR
  resetToolbar();
#endif
  Shynebox::instance()->workspaceChanged(*this);

  Shynebox::instance()->keys()->doAction(FocusIn, 0, 0, Keys::ON_DESKTOP);
} // changeWorkspaceID

void BScreen::sendToWorkspace(unsigned int id, ShyneboxWindow *win, bool changeWS) {
  if (! m_current_workspace || id >= m_workspaces_list.size() )
    return;

  if (!win)
    win = FocusControl::focusedSbWindow();

  // this seems like it's intended
  // however 1.3.7 was warping workspace anyway before rewrites?
  //if (!win || &win->screen() != this || win->isStuck() )
  if (!win || &win->screen() != this)
    return;

  tk::App::instance()->sync(false);

  windowMenu().hide();
  reassociateWindow(win, id, true);

  if (changeWS)
    changeWorkspaceID(id, false);
#if USE_TOOLBAR
  else
    resetToolbar();
#endif

  // if the window is on current workspace, show it; else hide it.
  if (id == m_current_wsid && !win->isIconic() )
    win->show();
  else {
    win->hide(true);
    FocusControl::revertFocus(*this);
  }

  // send all the transients too
  for (auto client : win->clientList() )
    for (auto transient : client->transientList() )
      if (transient->sbwindow() )
        sendToWorkspace(id, transient->sbwindow(), false);
} // sendToWorkspace

string BScreen::getNameOfWorkspace(unsigned int workspace) const {
  if (workspace < m_workspace_names.size() )
    return m_workspace_names[workspace];
  else
    return "";
}
// end workspace commands

void BScreen::keyPressEvent(XKeyEvent &ke) {
  if (Shynebox::instance()->keys()->doAction(ke.type, ke.state, ke.keycode,
              Keys::GLOBAL|(ke.subwindow ? 0 : Keys::ON_DESKTOP), 0, ke.time) ) {
    // re-grab keyboard, so we don't pass KeyRelease to clients
    // also for catching invalid keys in the middle of keychains
    tk::EventManager::instance()->grabKeyboard(rootWindow().window() );
    XAllowEvents(Shynebox::instance()->display(), SyncKeyboard, CurrentTime);
  } else
    XAllowEvents(Shynebox::instance()->display(), ReplayKeyboard, CurrentTime);
}

void BScreen::keyReleaseEvent(XKeyEvent &ke) {
  if (m_state.cycling) {
    unsigned int state = tk::KeyUtil::instance().cleanMods(ke.state);
    state &= ~tk::KeyUtil::instance().keycodeToModmask(ke.keycode);

    if (state) // still cycling
      return;

    m_state.cycling = false;
    focusControl().stopCyclingFocus();
  }
  if (!Shynebox::instance()->keys()->inKeychain() )
    tk::EventManager::instance()->ungrabKeyboard();
}

void BScreen::buttonPressEvent(XButtonEvent &be) {
  if (be.button == 1 && !isRootColormapInstalled() )
    imageControl().installRootColormap();

  Keys *keys = Shynebox::instance()->keys();
  if (keys->doAction(be.type, be.state, be.button,
      Keys::GLOBAL|Keys::ON_DESKTOP, 0, be.time) )
    XAllowEvents(Shynebox::instance()->display(), SyncPointer, CurrentTime);
  else
    XAllowEvents(Shynebox::instance()->display(), ReplayPointer, CurrentTime);
}

void BScreen::cycleFocus(int options, const ClientPattern *pat, bool reverse) {
  // if the keyboard repeat rate is too fast, hold+cycling can cause double
  // focus and essentially lock out all minimized/iconified windows.
  // alternatively it can chug with many windows open.
  // no human is going to mean to cycle at the speed of light anyway
  // so just put a lock on this, ignore and wait for the next attempt
  if (m_cycle_lock)
    return;
  m_cycle_lock = true;
  m_cycle_timer.start();

  // get modifiers from event that causes this for focus order cycling
  XEvent ev = Shynebox::instance()->lastEvent();
  unsigned int mods = 0;
  if (ev.type == KeyPress)
    mods = tk::KeyUtil::instance().cleanMods(ev.xkey.state);
  else if (ev.type == ButtonPress)
    mods = tk::KeyUtil::instance().cleanMods(ev.xbutton.state);

  if (!m_state.cycling && mods) {
    m_state.cycling = true;
    tk::EventManager::instance()->grabKeyboard(rootWindow().window() );
  }

  if (mods == 0) // can't stacked cycle unless there is a mod to grab
    options |= FocusableList::STATIC_ORDER;

  const FocusableList *win_list =
      FocusableList::getListFromOptions(*this, options);
  focusControl().cycleFocus(*win_list, pat, reverse);
#if USE_TOOLBAR
  updateToolbar(false);
#endif
} // cycleFocus

void BScreen::reconfigure() {
  Shynebox *shynebox = Shynebox::instance();

  clampMenuDelay(m_menu_delay);
  m_menutheme->setDelay(m_menu_delay);

  // provide the number of workspaces from the init-file
  const unsigned int nr_ws = m_workspace_cnt;
  if (nr_ws > m_workspaces_list.size() )
    while (nr_ws != m_workspaces_list.size() )
      addWorkspace();
  else if (nr_ws < m_workspaces_list.size() )
    while (nr_ws != m_workspaces_list.size() )
      removeLastWorkspace();

  // update menu filenames
  m_rootmenu->reloadHelper()->setMainFile(shynebox->getMenuFilename() );
  m_windowmenu->reloadHelper()->setMainFile(windowMenuFilename() );

  // reconfigure workspaces
  for (auto it : m_workspaces_list)
    it->reconfigure(); // updates client menus

  // update all windows
  focusedWinFrameThemeReconfigured();

  imageControl().cleanCache();

  reconfigureStruts();
#if USE_TOOLBAR
  updateWSItems(true);
  recfgToolbar();
  resetToolbar();
  updateToolbar(true);
#endif
} // reconfigure

void BScreen::reconfigThemes() {
  focusedWinFrameTheme()->reconfigTheme();
  unfocusedWinFrameTheme()->reconfigTheme();
  menuTheme()->reconfigTheme();
  rootTheme()->reconfigTheme();
  focusedWinButtonTheme()->reconfigTheme();
  unfocusedWinButtonTheme()->reconfigTheme();
  pressedWinButtonTheme()->reconfigTheme();
  focusedWinFrameThemeReconfigured();
  if (m_rootmenu)      m_rootmenu->reconfigure();
  if (m_configmenu)    m_configmenu->reconfigure();
  if (m_windowmenu)    m_windowmenu->reconfigure();
  if (m_workspacemenu) m_workspacemenu->reconfigure();
#if USE_TOOLBAR
  recfgToolbar();
  updateToolbar(true);
#endif

} // reconfigThemes

// reserve edge space on the screen to not max or create windows over
void BScreen::reconfigureStruts() {
  // only saved and used here
  for (auto it : m_custom_struts)
    clearStrut(it);

  m_custom_struts.clear();

  using namespace tk::StringUtil;

  // assume no one is using 100 monitors
  const int nh = std::min(std::max(0, numHeads() ), 100);
  for (int h = 0; h <= nh; ++h) {
    const string strut_cfg = "struts." + number2String(h);
    if (m_cfgmap.count(strut_cfg) == 0)
      continue; // not in config

    vector<string> v;
    stringtok(v, *m_cfgmap[strut_cfg], " ,");
    if (v.size() != 4)
      continue; // bad line

    // TODO: size checks, has to be positive
    //       has to be smaller than...? half screen ? less?
    int pos = 0, l, r, t, b;
    extractNumber(v[pos++], l);
    extractNumber(v[pos++], r);
    extractNumber(v[pos++], t);
    extractNumber(v[pos],   b);
    m_custom_struts.push_back(requestStrut(h, l, r, t, b) );
  }

  updateAvailableWorkspaceArea();
}

void BScreen::addIcon(ShyneboxWindow *w) {
  if (w == 0)
    return;

  if (find(iconList().begin(), iconList().end(), w) == iconList().end() )
    iconList().push_back(w);
}

void BScreen::removeIcon(ShyneboxWindow *w) {
  if (w == 0)
    return;

  auto eraseit = std::find(iconList().begin(), iconList().end(), w);
  if (eraseit != iconList().end() )
    iconList().erase(eraseit);
}

void BScreen::removeWindow(ShyneboxWindow *win) {
  sbdbg<<"BScreen::removeWindow("<<win<<")\n";

  // extra precaution, if for some reason, the
  // icon list should be out of sync
  removeIcon(win);
  // remove from workspace
  // 'Window' class removes window from focuscontrol on destroy
  Workspace *space = getWorkspace(win->workspaceNumber() );
  if (space != 0)
    space->removeWindow(win, false);

#if USE_TOOLBAR
  if (m_toolbar)
    m_toolbar->m_tool_factory.resetIconbar(win);
#endif
}

void BScreen::removeClient(WinClient &client) {
  focusControl().removeClient(client);

  for (auto [w, wc] : m_expecting_groups) {
    if (wc == &client) {
      m_expecting_groups.erase(w);
      break;
    }
  }

#if USE_TOOLBAR
  updateToolbar(false);
#endif
} // removeClient

bool BScreen::isKdeDockapp(Window client) const {
  //Check and see if client is KDE dock applet.
  bool iskdedockapp = false;
  Atom ajunk;
  int ijunk;
  unsigned long *data = 0, uljunk;
  Display *disp = tk::App::instance()->display();
  // Check if KDE v2.x dock applet
  if (XGetWindowProperty(disp, client, atom_kde_systray,
                         0l, 1l, False,
                         XA_WINDOW, &ajunk, &ijunk, &uljunk,
                         &uljunk, (unsigned char **) &data) == Success) {

    if (data)
      iskdedockapp = true;
    XFree((void *) data);
    data = 0;
  } // if XA_WINDOW

  // Check if KDE v1.x dock applet
  if (!iskdedockapp) {
    if (XGetWindowProperty(disp, client,
                           atom_kwm1, 0l, 1l, False,
                           atom_kwm1, &ajunk, &ijunk, &uljunk,
                           &uljunk, (unsigned char **) &data) == Success && data) {
      iskdedockapp = (data && data[0] != 0);
      XFree((void *) data);
      data = 0;
    } // if kwm1 aka KWM_DOCKWINDOW
  }
  return iskdedockapp;
} // isKdeDockapp

bool BScreen::addKdeDockapp(Window client) {
#if USE_TOOLBAR
  if (m_toolbar == 0 || !m_toolbar->m_tool_factory.hasSystray() )
    return false;

  XSelectInput(tk::App::instance()->display(), client, StructureNotifyMask);
  tk::EventHandler *evh  = 0;
  tk::EventManager *evm = tk::EventManager::instance();

  // this ~~handler~~ systray is a special case
  // so we call setupClient in it
  WinClient winclient(client, *this);
  m_toolbar->m_tool_factory.m_sys_tray->setupClient(winclient);
  // we need to save old event handler and re-add it later
  evh = evm->find(client);

  if (evh != 0) // re-add event handler
    evm->add(*evh, client);

  return true;
#else
  return false;
#endif
} // addKdeDockapp

void BScreen::createWindow(Window client) {
  Shynebox* shynebox = Shynebox::instance();
  shynebox->sync(false);

  if (isKdeDockapp(client) && addKdeDockapp(client) )
    return;

  WinClient *winclient = new WinClient(client, *this);

  if (winclient->initial_state == WithdrawnState
      || winclient->getWMClassClass() == "DockApp") {
    delete winclient;
    return;
  }

  // check if it should be grouped with something else
  WinClient* other = findGroupLeft(*winclient);
  if (!other && m_placement_strategy->placementPolicy() == tk::ScreenPlacementPolicy_e::AUTOTABPLACEMENT)
    other = FocusControl::focusedWindow();
  ShyneboxWindow* win = other ? other->sbwindow() : 0;

  if (other && win) {
    win->attachClient(*winclient);
    shynebox->setupClient(*winclient);
  } else {
    shynebox->setupClient(*winclient);
    if (winclient->sbwindow() ) { // may have been set elsewhere
      win = winclient->sbwindow();
      Workspace *workspace = getWorkspace(win->workspaceNumber() );
      if (workspace)
        workspace->updateClientmenu();
    } else { // attempt to create a window for it
      win = new ShyneboxWindow(*winclient);

      if (!win->isManaged() ) {
        delete win;
        return;
      }
    }
  } // if other && win

  // add the window to the focus list
  // always add to front on startup to keep the focus order the same
  if (win->isFocused() || shynebox->isStartup() )
    focusControl().addFocusFront(*winclient);
  else
    focusControl().addFocusBack(*winclient);

  // we also need to check if another window expects this window to the left
  // and if so, then join it.
  if ((other = findGroupRight(*winclient) ) && other->sbwindow() != win)
    win->attachClient(*other);
  else if (other) // should never happen
    win->moveClientRightOf(*other, *winclient);

  Shynebox::instance()->clientListChanged(*this);
#if USE_TOOLBAR
  if (m_toolbar)
    m_toolbar->m_tool_factory.updateIconbar(win);
#endif

  shynebox->sync(false);
} // createWindow(Window) - aka X11 'Window'

void BScreen::createWindow(WinClient &client) {
  if (isKdeDockapp(client.window() ) && addKdeDockapp(client.window() ) )
    return;

  ShyneboxWindow *win = new ShyneboxWindow(client);

  if (!win->isManaged() ) {
    delete win;
    return;
  }

  win->show();
  // don't ask me why, but client doesn't seem to keep focus in new window
  // and we don't seem to get a FocusIn event from setInputFocus
  if (focusControl().focusNew() || FocusControl::focusedWindow() == &client)
    FocusControl::setFocusedWindow(&client); // xgrabs for focus

  Shynebox::instance()->setupClient(client); // ewmh and Remember

  Shynebox::instance()->clientListChanged(*this);
#if USE_TOOLBAR
  if (m_toolbar)
    m_toolbar->m_tool_factory.updateIconbar(win);
#endif
} // createWindow(client)

void BScreen::reassociateWindow(ShyneboxWindow *w, unsigned int wkspc_id,
                                bool ignore_sticky) {
  if (w == 0)
    return;

  if (wkspc_id >= numberOfWorkspaces() )
    wkspc_id = m_current_wsid;

  if (!w->isIconic() && w->workspaceNumber() == wkspc_id)
    return;

  if (w->isIconic() ) {
    removeIcon(w);
    getWorkspace(wkspc_id)->addWindow(*w);
    w->setWorkspace(wkspc_id);
  } else if (ignore_sticky || ! w->isStuck() ) {
    // fresh windows have workspaceNumber == -1, which leads to
    // an invalid workspace (unsigned int)
    Workspace* ws = getWorkspace(w->workspaceNumber() );
    if (ws)
      ws->removeWindow(w, true);
    getWorkspace(wkspc_id)->addWindow(*w);
    w->setWorkspace(wkspc_id);
  }
} // reassociateWindow (to workspace)

void BScreen::focusedWinFrameThemeReconfigured() {
  renderGeomWindow();
  renderPosWindow();

  for (auto &f : focusControl().focusedOrderWinList().clientList() )
    f->sbwindow()->themeReconfigured(); // just applyDecorations()
}

#if USE_TOOLBAR
void BScreen::clearToolButtonMap() {
  m_toolButtonMap->clear();
}

void BScreen::mapToolButton(std::string name, tk::TextButton *button) {
  m_toolButtonMap->insert(std::pair<std::string, tk::TextButton*>(name, button) );
}

void BScreen::systrayCheckClientMsg(const XClientMessageEvent &ce,
                                  BScreen *screen, WinClient *const winclient) {
  if (m_toolbar && m_toolbar->m_tool_factory.hasSystray() )
    m_toolbar->m_tool_factory.m_sys_tray->checkClientMessage(ce, screen, winclient);
}

// only used in setupClient() but outside of screen class and called multiple times
void BScreen::setupSystrayClient(WinClient &winclient) {
  if (m_toolbar && m_toolbar->m_tool_factory.hasSystray() )
    m_toolbar->m_tool_factory.m_sys_tray->setupClient(winclient);
}

void BScreen::recfgToolbar() {
  if (m_toolbar)
    m_toolbar->reconfigure();
}

void BScreen::resetToolbar() { // TODO: bad name
  if (!m_toolbar)
    return;
  m_toolbar->m_tool_factory.updateWSNameTag();
  m_toolbar->m_tool_factory.resetIconbar(0);
}

// for single title updates
// ewmh > winclient > shyneboxwindow > here > iconbar
void BScreen::updateIconbar() {
  if (m_toolbar)
    m_toolbar->m_tool_factory.updateIconbar(0); // 'ALIGN'
}

void BScreen::updateToolbar(bool everything) { // true
  if (!m_toolbar)
    return;
  if (everything) // aka not just iconbar
    m_toolbar->relayout();
  else if (m_toolbar->m_tool_factory.hasSystray() )
    m_toolbar->m_tool_factory.m_sys_tray->updateSizing();
  m_toolbar->m_tool_factory.updateIconbar(0); // ALIGN
}
#endif // USE_TOOLBAR

  void BScreen::updateWSItems(bool everything) { // false
#if USE_TOOLBAR
    if (m_toolbar)
      m_toolbar->m_tool_factory.updateWSNameTag();
#endif
    if (!everything) // for SbCommands::ShowRootMenuCmd
      return;

    if (m_workspacemenu) { // restarts will crash
      m_workspacemenu->workspaceInfoChanged(*this); // deletes menus it doesnt own
      m_workspacemenu->workspaceChanged(*this);
    }
    updateClientMenus();
  } // updateWSItems()

void BScreen::initMenus() {
  m_workspacemenu = new WorkspaceMenu(*this);
  m_rootmenu->reloadHelper()->setMainFile(Shynebox::instance()->getMenuFilename() );
  m_windowmenu->reloadHelper()->setMainFile(windowMenuFilename() );
}

void BScreen::rereadMenu() {
  m_rootmenu->removeAll();
  m_rootmenu->setLabel(tk::BiDiString("") );

  Shynebox * const sb = Shynebox::instance();
  if (!sb->getMenuFilename().empty() )
    MenuCreator::createFromFile(sb->getMenuFilename(), *m_rootmenu,
                                m_rootmenu->reloadHelper() );
}

const std::string BScreen::windowMenuFilename() const {
  return Shynebox::instance()->getWinMenuFilename();
}

void BScreen::rereadWindowMenu() {
  m_windowmenu->removeAll();
  if (!windowMenuFilename().empty() )
    MenuCreator::createFromFile(windowMenuFilename(), *m_windowmenu,
                                m_windowmenu->reloadHelper() );
}

void BScreen::addConfigMenu(const tk::SbString &label, tk::Menu &menu) {
  tk::Menu *cm = m_configmenu;
  if (cm && cm->findSubmenuIndex(&menu) == -1)
    cm->insertSubmenu(label, &menu, -1);
    // add if not found (-1)
}

void BScreen::removeConfigMenu(tk::Menu &menu) {
  tk::Menu *cm = m_configmenu;
  if (cm) {
    int pos = cm->findSubmenuIndex(&menu);
    if (pos > -1)
      cm->remove(pos);
  }
}

int BScreen::getGap(int head, const char type) {
  return type == 'w' ? getXGap(head) : getYGap(head);
}

int BScreen::calRelativeSize(int head, int i, char type) {
  return tk::RelCalcHelper::calPercentageValueOf(i, getGap(head, type) );
}

int BScreen::calRelativeWidth(int head, int i) {
  return calRelativeSize(head, i, 'w');
}

int BScreen::calRelativeHeight(int head, int i) {
  return calRelativeSize(head, i, 'h');
}

int BScreen::calRelativePosition(int head, int i, char type) {
  int max = type == 'w' ? maxLeft(head) : maxTop(head);
  return tk::RelCalcHelper::calPercentageOf((i - max), getGap(head, type) );
}

// returns a pixel, which is relative to the width of the screen
// screen starts from 0, 1000 px width, if i is 10 then it should return 100
int BScreen::calRelativePositionWidth(int head, int i) {
  return calRelativePosition(head, i, 'w');
}

// returns a pixel, which is relative to the height of th escreen
// screen starts from 0, 1000 px height, if i is 10 then it should return 100
int BScreen::calRelativePositionHeight(int head, int i) {
  return calRelativePosition(head, i, 'h');
}

int BScreen::calRelativeDimension(int head, int i, char type) {
  return tk::RelCalcHelper::calPercentageOf(i, getGap(head, type) );
}

int BScreen::calRelativeDimensionWidth(int head, int i) {
  return calRelativeDimension(head, i, 'w');
}

int BScreen::calRelativeDimensionHeight(int head, int i) {
  return calRelativeDimension(head, i, 'h');
}

float BScreen::getXGap(int head) {
  return maxRight(head) - maxLeft(head);
}

float BScreen::getYGap(int head) {
  return maxBottom(head) - maxTop(head);
}

void BScreen::showPosition(int x, int y) {
  if (!doShowWindowPos() )
    return;

  // have to have 14 8k monitors for 100k pixels
  char buf[30]; // too big? 17 max ???
  snprintf(buf, sizeof(buf), "X:%5d x Y:%5d", x, y);

  tk::BiDiString label(buf);
  m_pos_window->showText(label);
}

void BScreen::hidePosition() {
  m_pos_window->hide();
}

void BScreen::showGeometry(unsigned int gx, unsigned int gy) {
  if (!doShowWindowPos() )
    return;

  char buf[30]; // too big?
  _SB_USES_NLS;

  snprintf(buf, sizeof(buf), "W: %4d x H: %4d", gx, gy);

  tk::BiDiString label(buf);
  m_geom_window->showText(label);
}

void BScreen::hideGeometry() {
  m_geom_window->hide();
}

void BScreen::showTooltip(const tk::BiDiString &text) {
  if (m_tooltip_delay >= 0)
    m_tooltip_window->showText(text);
}

void BScreen::hideTooltip() {
  if (m_tooltip_delay >= 0)
    m_tooltip_window->hide();
}

// start private
void BScreen::setupConfigmenu(tk::Menu &menu) {
//  menu.removeAll();
  ConfigMenu::setup(menu, *this);
  menu.updateMenu();
}

void BScreen::renderGeomWindow() {
  char buf[30]; // too big? 17 max?
  _SB_USES_NLS;

  const int n = snprintf(buf, sizeof(buf), "W: %4d x H: %4d", 0, 0);

  tk::BiDiString label(std::string(buf, n) );
  m_geom_window->resizeForText(label);
  m_geom_window->reconfigTheme();
}

void BScreen::renderPosWindow() {
  m_pos_window->resizeForText(tk::BiDiString("0:00000 x 0:00000") );
  m_pos_window->reconfigTheme();
}

// end private

WinClient *BScreen::findGroupLeft(WinClient &winclient) {
  Window w = winclient.getGroupLeftWindow();
  if (w == None)
    return 0;

  WinClient *have_client = Shynebox::instance()->searchWindow(w);

  if (!have_client)
    m_expecting_groups[w] = &winclient; // not found, add to expecting
  else if (&have_client->screen() != &winclient.screen() )
    return 0; // something is not consistent

  return have_client;
}

WinClient *BScreen::findGroupRight(WinClient &winclient) {
  auto it = m_expecting_groups.find(winclient.window() );
  if (it == m_expecting_groups.end() )
    return 0;

  WinClient *other = it->second;
  m_expecting_groups.erase(it);

  // forget about it if it isn't the left-most client in the group
  Window leftwin = other->getGroupLeftWindow();
  if (leftwin != None && leftwin != winclient.window() )
    return 0;

  return other;
}

void BScreen::configRandr() {
  // NOTE: m_head_areas init on screen() init
  m_heads.resize(1);
  // head 0 is WHOLE, all combined viewable monitors
  // top left should always be 0,0 (right?)
  m_heads[0]._x = 0;
  m_heads[0]._y = 0;
  m_heads[0]._width = width(); // root window dimensions
  m_heads[0]._height = height();

  int mon_cnt = 0, old_sz = m_heads.size();
  Display *disp = Shynebox::instance()->display();

  XRRMonitorInfo *xr_inf = XRRGetMonitors(disp, m_root_window.window(),
                  /* 1 means 'active' */  1, &mon_cnt);

  if (mon_cnt > 1) {
    m_heads.resize(mon_cnt + 1);
    for (int i = 0 ; i < mon_cnt; i++) {
      m_heads[i+1]._x = xr_inf[i].x;
      m_heads[i+1]._y = xr_inf[i].y;
      m_heads[i+1]._width = xr_inf[i].width;
      m_heads[i+1]._height = xr_inf[i].height;
    }

    if (mon_cnt > old_sz) { // grow
      m_head_areas.resize(mon_cnt + 1);
      for ( ; old_sz <= mon_cnt ; old_sz++)
        m_head_areas[old_sz] = new HeadArea();
    } else { // shrink
      for ( ; old_sz > 1 ; old_sz--)
        delete m_head_areas[old_sz];
      m_head_areas.resize(mon_cnt + 1);
    }
  } // if mon_cnt > 1

// DEBUG:
//
//  sbdbg << "       INIT MONITORS\n";
//  for (int i = 0 ; i <= mon_cnt; i++) {
//  sbdbg << "       x " << m_heads[i]._x << " - "
//        << "       y " << m_heads[i]._y << " - "
//        << "       w " << m_heads[i]._width << " - "
//        << "       h " << m_heads[i]._height << "\n";
//  }
//  for (int i = 0 ; i <= mon_cnt; i++) {
//  sbdbg << "       l " << m_head_areas[i]->availableWorkspaceArea()->left() << " - "
//        << "       r " << m_head_areas[i]->availableWorkspaceArea()->right() << " - "
//        << "       t " << m_head_areas[i]->availableWorkspaceArea()->top() << " - "
//        << "       b " << m_head_areas[i]->availableWorkspaceArea()->bottom() << "\n";
//  }

  XRRFreeMonitors(xr_inf);
} // configRandr

void BScreen::updateSize() {
  const std::vector<randr_head_info> old_heads = m_heads;

  configRandr();

  // assume geometry has changed, this is only called for root window here
  // and this func only called from randr events in main event loop
  bool heads_changed = rootWindow().updateGeometry()
                     && old_heads.size() != m_heads.size();

  // if root window seems to be the same
  // maybe a monitor/head moved places (e.g. 'left of' to 'right of')
  if (!heads_changed) {
    for (size_t i = 0; i < m_heads.size() ; i++) {
      if (old_heads[i]._x != this->m_heads[i]._x
          || old_heads[i]._y != this->m_heads[i]._y
          || old_heads[i]._width != this->m_heads[i]._width
          || old_heads[i]._height != this->m_heads[i]._height) {
        heads_changed = true;
        break;
      }
    }
  } // if heads same

  if (heads_changed) {
    m_bg_timer.start(); // m_root_theme->reset(); // reset background
    // this is on a timer because each monitor may generate a xrandr
    // event. this calls sbsetbg too fast and messes up lastwallpaper
#if USE_TOOLBAR
    recfgToolbar();
    resetToolbar();
#endif
  }
#if USE_TOOLBAR
  updateToolbar(true);
#endif

  reconfigureStruts();

  Shynebox::instance()->workspaceAreaChanged(*this);
}

// clamp head where it's accessed

const Strut* BScreen::availableWorkspaceArea(int head) const {
  if (!doObeyHeads() || head >= numHeads() || head < 0)
    head = 0;

  return m_head_areas[head]->availableWorkspaceArea();
}

// all of these ignore_struts default to false
// SbWinFrame::applyState() sends doFullMax()
unsigned int BScreen::maxLeft(int head, bool ignore_struts) const {
  int left = getHeadX(head);

  if (!ignore_struts)
    left += availableWorkspaceArea(head)->left();

  return left;
}

unsigned int BScreen::maxRight(int head, bool ignore_struts) const {
  int right = getHeadX(head) + getHeadWidth(head);

  if (!ignore_struts)
    right -= availableWorkspaceArea(head)->right();

  return right;
}

unsigned int BScreen::maxTop(int head, bool ignore_struts) const {
  int top = getHeadY(head);

  if (!ignore_struts)
    top += availableWorkspaceArea(head)->top();

  return top;
}

unsigned int BScreen::maxBottom(int head, bool ignore_struts) const {
  int bot = getHeadY(head) + getHeadHeight(head);

  if (!ignore_struts)
    bot -= availableWorkspaceArea(head)->bottom();

  return bot;
}

Strut *BScreen::requestStrut(int head, int left, int right, int top, int bottom) {
  std::clamp(head, 0, numHeads() - 1);

  int begin = head;
  int end   = begin + 1;

  if (head == 0 && numHeads() > 1) // all heads
    end = numHeads();

  Strut* next = 0;
  for (int i = begin; i < end; i++)
    next = m_head_areas[i]->requestStrut(i, left, right, top, bottom, next);

  return next;
}

void BScreen::clearStrut(Strut *str) {
  if (str->next() ) // clears all after
    clearStrut(str->next() );

  int head = str->head() ? str->head() : 0;

  // The number of heads may have changed, be careful.
  if (head < numHeads() )
    m_head_areas[head]->clearStrut(str);
  // str is invalid now
}

void BScreen::updateAvailableWorkspaceArea() {
  bool updated = false;

  for (int i = numHeads() - 1 ; i >= 0 ; i--)
    updated = m_head_areas[i]->updateAvailableWorkspaceArea() || updated;

  if (updated)
    Shynebox::instance()->workspaceAreaChanged(*this);
}

int BScreen::getHead(int x, int y) const {
  if (doObeyHeads() && numHeads() > 1)
    for (int i = 1; i < numHeads(); i++)
      if (RectangleUtil::insideBorder(m_heads[i], x, y, 0) )
        return i;
  return 0;
}

int BScreen::getHead(const tk::SbWindow &win) const {
  int head = 0;
  if (doObeyHeads() ) {
    // center of win
    int cx = win.x() + static_cast<int>(win.width() / 2);
    int cy = win.y() + static_cast<int>(win.height() / 2);

    head = getHead(cx, cy);

    if (head == 0 && numHeads() > 1) { // center is off screen
    long closest = -1;
    #define calcSquareDistance(x1, y1, x2, y2) ((x2-x1)*(x2-x1) + (y2-y1)*(y2-y1) )
    for (int i = 1; i < numHeads(); i++) {
      const randr_head_info &ri = m_heads[i];
      long dist = calcSquareDistance(cx, cy, ri.x() + (ri.width() / 2),
                                     ri.y() + (ri.height() / 2) );
      if (closest == -1 || dist < closest) {
        head = i; // found a closer head
        closest = dist;
      }
    }
    #undef calcSquareDistance
    } // if off screen
  } // if head checks
  return head;
}

int BScreen::getCurHead() const {
  if (!doObeyHeads() )
    return 0;

  int root_x = 0, root_y = 0;

  tk::KeyUtil::get_pointer_coords(
      tk::App::instance()->display(),
      rootWindow().window(), root_x, root_y);

  return getHead(root_x, root_y);
}

#define HEAD_CHK (doObeyHeads() && head < numHeads() && head >= 0)

int BScreen::getHeadX(int head) const {
  if (!HEAD_CHK)
    head = 0;
  return m_heads[head].x();
}

int BScreen::getHeadY(int head) const {
  if (!HEAD_CHK)
    head = 0;
  return m_heads[head].y();
}

int BScreen::getHeadWidth(int head) const {
  if (!HEAD_CHK)
    head = 0;
  return m_heads[head].width();
}

int BScreen::getHeadHeight(int head) const {
  if (!HEAD_CHK)
    head = 0;
  return m_heads[head].height();
}

#undef HEAD_CHK

void BScreen::fitToHead(int head, int &x, int &y,
        int w, int h, int bw, bool respect_struts) const {
  // head check should be covered in all subsequent calls

  int l, r, t, b;

  if (respect_struts) {
    l = maxLeft(head);
    r = maxRight(head);
    t = maxTop(head);
    b = maxBottom(head);
  } else {
    l = getHeadX(head);
    r = getHeadWidth(head) + l;
    t = getHeadY(head);
    b = getHeadHeight(head) + t;
  }

  if (x < l)
    x = l;
  else if (x + w + bw > r)
    x = r - w - bw;

  if (y < t)
    y = t;
  else if (y + h + bw > b)
    y = b - h - bw;
}

// Copyright (c) 2023 Shynebox - zlice
//
// Copyright (c) 2001 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
//
// Screen.cc for Blackbox - an X11 Window manager
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
