// shynebox.cc for Shynebox Window Manager

#include "shynebox.hh"

#include "Screen.hh"
#include "Window.hh"
#include "Workspace.hh"
#include "SbCommands.hh"
#include "WinClient.hh"
#include "Keys.hh"
#include "SbAtoms.hh"
#include "FocusControl.hh"

#include "defaults.hh"
#include "Debug.hh"

#include "tk/I18n.hh"
#include "tk/Image.hh"
#include "tk/ImageControl.hh"
#include "tk/EventManager.hh"
#include "tk/StringUtil.hh"
#include "tk/SimpleCommand.hh"
#include "tk/Command.hh"
#include "tk/KeyUtil.hh"

// X headers
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>

// X extensions
#ifdef SHAPE
#include <X11/extensions/shape.h>
#endif

#include <X11/extensions/Xrandr.h>

// system headers

#ifdef HAVE_UNISTD_H
#include <sys/types.h>
#include <unistd.h>
#endif

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

#ifdef HAVE_SYS_STAT_H
#include <sys/types.h>
#include <sys/stat.h>
#endif

#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif

#include <iostream>
#include <typeinfo>
#include <cstdio>
#include <cstdlib>
#include <cstring>


using std::cerr;
using std::string;
using std::vector;
using std::list;
using std::pair;
using std::hex;
using std::dec;

using namespace tk;

namespace {

const char RC_INIT_FILE[] = "init";

Window last_bad_window = None;

// *** NOTE: if you want to debug here the X errors are
//     coming from, you should turn on the XSynchronise call below
int handleXErrors(Display *d, XErrorEvent *e) {
  if (e->error_code == BadWindow)
    last_bad_window = e->resourceid;
#ifdef DEBUG
  else {
    // ignore bad window ones, they happen a lot
    // when windows close themselves
    char errtxt[128];

    XGetErrorText(d, e->error_code, errtxt, 128);
    cerr << "Shynebox: X Error: "
         << errtxt
         << "(" <<(int)e->error_code << ") opcodes "
         << (int)e->request_code
         << "/"
         << (int)e->minor_code
         << " resource 0x" << hex <<(int)e->resourceid
         << dec << "\n";
//        if (e->error_code != 9 && e->error_code != 183)
//            kill(0, 2);
  }
#endif
  return False;
}

int handleXIOErrors(Display* d) {
  (void) d; // x lib wants arg i presume
  cerr << "Shynebox: XIOError: lost connection to display.\n";
  exit(1);
}

class KeyReloadHelper {
public:
  void reload() {
    Shynebox* f = Shynebox::instance();
    Keys* k = (f ? f->keys() : 0);
    if (k) {
      XRefreshKeyboardMapping(&(this->xmapping) );
      tk::KeyUtil::instance().loadModmap();
      k->regrab();
    }
  }
  XMappingEvent xmapping;
};

KeyReloadHelper s_key_reloader;

int s_randr_event_type = 0; // the type number of randr event
int s_shape_eventbase = 0;  // event base for shape events
bool s_have_shape = false ; // if shape is supported by server

Shynebox* s_singleton = 0;

} // end anonymous


bool Shynebox::haveShape() const { return s_have_shape; }
int Shynebox::shapeEventbase() const { return s_shape_eventbase; }
Shynebox* Shynebox::instance() { return s_singleton; }

Shynebox::Config::Config(tk::ConfigManager& cm) :
  colors_per_channel(*cm.get_cfgmap()["colorsPerChannel"]),
  double_click_interval(*cm.get_cfgmap()["doubleClickInterval"]),
  tabs_padding(*cm.get_cfgmap()["tabs.textPadding"]),
  apps_file(*cm.get_cfgmap()["appsFile"]),
  key_file(*cm.get_cfgmap()["keyFile"]),
  menu_file(*cm.get_cfgmap()["menuFile"]),
  overlay_file(*cm.get_cfgmap()["styleOverlay"]),
  style_file(*cm.get_cfgmap()["styleFile"]),
  winmenu_file(*cm.get_cfgmap()["windowMenuFile"]),
  menusearch((tk::MenuMode_e&)(int&)(*cm.get_cfgmap()["menuSearch"]) ),
  cache_life(*cm.get_cfgmap()["cacheLife"]),
  cache_max(*cm.get_cfgmap()["cacheMax"]),
  auto_raise_delay(*cm.get_cfgmap()["autoRaiseDelay"])
{ }

Shynebox::Shynebox(int argc, char **argv,
                 const std::string& dpy_name,
                 const std::string& rc_path,
                 const std::string& rc_filename, bool xsync)
      : tk::App(dpy_name.c_str() ),
        m_sbatoms(SbAtoms::instance() ),
        m_configmanager(rc_filename.c_str() ),
        m_config(m_configmanager),
        m_last_time(0),
        m_masked(0),
        m_masked_window(0),
        m_argv(argv), m_argc(argc),
        m_showing_dialog(false),
        m_server_grabs(0) {
  _SB_USES_NLS;

  m_state.restarting = false;
  m_state.shutdown = false;
  m_state.starting = true;

  if (s_singleton != 0)
    throw _SB_CONSOLETEXT(Shynebox, FatalSingleton,
      "Fatal! There can only one instance of shynebox class.",
      "Error displayed on weird error where an instance of the Shynebox class already exists!");

  if (display() == 0)
    throw _SB_CONSOLETEXT(Shynebox, NoDisplay,
      "Can not connect to X server.\nMake sure you started X before you start Shynebox.",
      "Error message when no X display appears to exist");

  m_config.path = rc_path;
  m_config.file = rc_filename;
  // check config-item file-paths
  // they start blank but should be next to config unless set by user
  if (m_config.apps_file == "")    m_config.apps_file = m_config.path + "/apps";
  if (m_config.style_file == "")   m_config.style_file = DEFAULTSTYLE;
  if (m_config.overlay_file == "") m_config.overlay_file = m_config.path + "/overlay";
  if (m_config.menu_file == "")    m_config.menu_file = m_config.path + "/menu";
  if (m_config.key_file == "")     m_config.key_file = m_config.path + "/keys";
  if (m_config.winmenu_file == "") m_config.winmenu_file = m_config.path + "/windowmenu";

  m_active_screen.mouse = 0;
  m_active_screen.key = 0;

  Display *disp = tk::App::instance()->display();

  // setup X error handler
  XSetErrorHandler(handleXErrors);
  XSetIOErrorHandler(handleXIOErrors);

  // shared commands so ConfigMenu doesn't
  // have to create 50 of the same thing
  if (!m_shared_saverc) {
    m_shared_saverc = (Command<void> *) new SbCommands::SaveResources();
    m_shared_saverc->set_is_shared();
  }

  if (!m_shared_sv_rcfg_macro) {
    m_shared_sv_rcfg_macro = new MacroCommand();
    m_shared_sv_rcfg_macro->add(*m_shared_saverc);
    m_shared_sv_rcfg_macro->add(*(Command<void>*)(new SbCommands::ReconfigureShyneboxCmd() ) );
    m_shared_sv_rcfg_macro->set_is_shared();
  }

  // setup timer
  // This timer is used to we can issue a safe reconfig command.
  // Because when the command is executed we shouldn't do reconfig directly
  // because it could affect ongoing menu stuff so we need to reconfig in
  // the next event "round".
  tk::SimpleCommand<Shynebox> *reconfig_cmd(
    new tk::SimpleCommand<Shynebox>(*this, &Shynebox::timed_reconfigure) );

  m_reconfig_timer.setTimeout(1);
  m_reconfig_timer.setCommand(*reconfig_cmd);
  m_reconfig_timer.fireOnce(true);

  // xmodmap and other tools send a lot of MappingNotify events under some
  // circumstances ("keysym comma = comma semicolon" creates 4 or 5).
  // reloading the keys-file for every one of them is unclever. we postpone
  // the reload() via a timer.
  tk::SimpleCommand<KeyReloadHelper> *rh_cmd(new tk::SimpleCommand<KeyReloadHelper>(s_key_reloader, &KeyReloadHelper::reload) );
  m_key_reload_timer.setTimeout(250 * tk::SbTime::IN_MILLISECONDS);
  m_key_reload_timer.setCommand(*rh_cmd);
  m_key_reload_timer.fireOnce(true);

  if (xsync)
    XSynchronize(disp, True);

  s_singleton = this;

#ifdef SHAPE
  int shape_err;
  s_have_shape = XShapeQueryExtension(disp, &s_shape_eventbase, &shape_err);
#endif

  int randr_error_base;
  XRRQueryExtension(disp, &s_randr_event_type, &randr_error_base);

  grab();

  if (! XSupportsLocale() )
    cerr<<_SB_CONSOLETEXT(Shynebox, WarningLocale,
                          "Warning: X server does not support locale",
                          "XSupportsLocale returned false")<<"\n";

  if (XSetLocaleModifiers("") == 0)
    cerr<<_SB_CONSOLETEXT(Shynebox, WarningLocaleModifiers,
                          "Warning: cannot set locale modifiers",
                          "XSetLocaleModifiers returned false")<<"\n";

#ifdef HAVE_GETPID
    m_shynebox_pid = XInternAtom(disp, "_SHYNEBOX_PID", False);
    // previously _BLACKBOX_PID ???
#endif // HAVE_GETPID

  // setup theme manager to have our style file ready to be scanned
  tk::ThemeManager::instance().load(getStyleFilename(), getStyleOverlayFilename() );

  // Create keybindings handler and load keys file
  // Note: this needs to be done before creating screens
  m_key = new Keys;
  m_key->reconfigure();
  tk::MenuSearch::setMode(m_config.menusearch);

  unsigned int opts = OPT_TOOLBAR;
  vector<int> screens;
  int i;

  /* was going to reduce to a single screen, however...
     unfortunately idk how to configure this and never have.
     i've read others have done it but i'm sure X has changed
     since and i can't tell if it's a multi-video card thing
     or how to go about actually doing it. everything for me
     makes '1 screen' (:0.0) and xrandr takes care of the rest,
     which i imagine is what 99% of people do. maybe there's
     some edge use case to have virtual stuff, or run a diff
     window manager on different 'physical monitors'
     but i cant tell how to set it up
     so for now leaving this as is - zlice
  */
  // default is "use all screens"
  for (i = 0; i < ScreenCount(disp); i++)
    screens.push_back(i);

  // find out, on what "screens" shynebox should run
  for (i = 1; i < m_argc; i++) {
    if (! strcmp(m_argv[i], "-screen") ) {
      if ((++i) >= m_argc) {
        cerr << _SB_CONSOLETEXT(main, ScreenRequiresArg,
                                "error, -screen requires argument",
                                "the -screen option requires a file argument") << "\n";
        exit(EXIT_FAILURE);
      }

      // "all" is default
      if (!strcmp(m_argv[i], "all") )
        break; // strcmp is 0 for equal

      vector<string> vals;
      vector<int> scrtmp;
      int scrnr = 0;
      tk::StringUtil::stringtok(vals, m_argv[i], ",:");
      for (vector<string>::iterator scrit = vals.begin();
           scrit != vals.end(); ++scrit) {
        scrnr = atoi(scrit->c_str() );
        if (scrnr >= 0 && scrnr < ScreenCount(disp) )
          scrtmp.push_back(scrnr);
      }

      if (!vals.empty() )
        swap(scrtmp, screens);
    } else if (!strcmp(m_argv[i], "-no-toolbar") )
      opts &= ~OPT_TOOLBAR;
  } // for m_argc

  // create screens
  for (i = 0; i < static_cast<int>(screens.size() ); i++) {
    std::string sc_nr = tk::StringUtil::number2String(screens[i]);
    BScreen *screen = new BScreen(m_configmanager,
                                  screens[i], (int)tk::ResLayers_e::NUM_LAYERS, opts);

    // already handled
    if (! screen->isScreenManaged() ) {
      delete screen;
      continue;
    }

    // add to our list
    m_screens.push_back(screen);
  }

  if (m_screens.empty() ) {
    throw _SB_CONSOLETEXT(Shynebox, ErrorNoScreens,
      "Couldn't find screens to manage.\nMake sure you don't have another window manager running.",
      "Error message when no unmanaged screens found - usually means another window manager is running");
  }

  m_active_screen.key = m_active_screen.mouse = m_screens.front();

  m_ewmh = new Ewmh();
  // parse apps file after creating screens (so we can tell if it's a restart
  // for [startup] items) but before creating windows
  // this needs to be after ewmh and gnome, so state atoms don't get
  // overwritten before they're applied
  m_remember = new Remember();
  // init all "screens"
  for (auto it : m_screens) {
    m_ewmh->initForScreen(*it);

    // now we can create menus (which needs this screen to be in screen_list)
    it->reconfigureStruts();
    it->initMenus();
    it->initWindows();

    FocusControl::revertFocus(*it); // make sure focus style is correct
  }

  XAllowEvents(disp, ReplayPointer, CurrentTime);

  sync(false);

  m_reconfigure_wait = false;

  ungrab();

  m_state.starting = false;
} // Shynebox class init

Shynebox::~Shynebox() {
  delete m_sbatoms;

  // destroy screens (after others, as they may do screen things)
  for (auto it : m_screens)
    delete it;
  m_screens.clear();

  // this needs to be after screens, because toolbar has hotkeys
  // prevents restarts
  delete m_key;

  delete m_ewmh;
  delete m_remember;

  if (m_shared_sv_rcfg_macro)
    delete m_shared_sv_rcfg_macro;
  if (m_shared_saverc) // ^above macro uses this
    delete m_shared_saverc;
} // Shynebox class destroy


void Shynebox::eventLoop() {
  Display *disp = display();

  while (!m_state.shutdown) {
    if (XPending(disp) ) {
      XEvent e;
      XNextEvent(disp, &e);

      if (last_bad_window != None && e.xany.window == last_bad_window
          && e.type != DestroyNotify) { // we must let the actual destroys through
        if (e.type == FocusOut) {
          sbdbg<<"Shynebox::eventLoop(): reverting focus from bad window\n";
          revertFocus();
        }
        else
          sbdbg<<"Shynebox::eventLoop(): removing bad window from event queue\n";
      } else if ((e.type == EnterNotify || e.type == LeaveNotify)
          && (e.xcrossing.mode == NotifyGrab || e.xcrossing.mode == NotifyUngrab) ) {
        continue;
      } else {
        last_bad_window = None;
        handleEvent(&e);
      }
    } else
      tk::Timer::updateTimers(ConnectionNumber(disp) );
  } // while not shutdown
} // eventLoop

bool Shynebox::validateWindow(Window window) const {
  XEvent event;
  if (XCheckTypedWindowEvent(display(), window, DestroyNotify, &event) ) {
    XPutBackEvent(display(), &event);
    return false;
  }

  return true;
}

void Shynebox::grab() {
  if (! m_server_grabs++)
    XGrabServer(display() );
}

void Shynebox::ungrab() {
  if (! --m_server_grabs)
    XUngrabServer(display() );

  if (m_server_grabs < 0)
    m_server_grabs = 0;
}

void Shynebox::handleEvent(XEvent * const e) {
  _SB_USES_NLS;
  m_last_event = *e;

  // it is possible (e.g. during moving) for a window
  // to mask all events to go to it
  if ((m_masked == e->xany.window) && m_masked_window) {
    if (e->type == MotionNotify) {
      m_last_time = e->xmotion.time;
      m_masked_window->motionNotifyEvent(e->xmotion);
      return;
    } else if (e->type == ButtonRelease)
      e->xbutton.window = m_masked_window->sbWindow().window();
  }

  // update key/mouse screen and last time before we enter other eventhandlers
  if (e->type == KeyPress || e->type == KeyRelease) {
    m_active_screen.key = searchScreen(e->xkey.root);
  } else if (e->type == ButtonPress
             || e->type == ButtonRelease
             || e->type == MotionNotify) {
    m_last_time = e->xbutton.time;
    if (e->type == MotionNotify)
      m_last_time = e->xmotion.time;

    m_active_screen.mouse = searchScreen(e->xbutton.root);
  } else if (e->type == EnterNotify || e->type == LeaveNotify) {
    m_last_time = e->xcrossing.time;
    m_active_screen.mouse = searchScreen(e->xcrossing.root);
  } else if (e->type == VisibilityNotify) {
    WinClient *cli = searchWindow(e->xvisibility.window);
    if (cli && cli->sbwindow() )
      cli->sbwindow()->setInView(
        e->xvisibility.state == VisibilityFullyObscured ? false : true);
    return; // nothing else handles these right now
  }

  // try tk::EventHandler first
  tk::EventManager::instance()->handleEvent(*e);

  switch (e->type) {
  case ButtonPress:
  case ButtonRelease:
    break; // handled right above
  case ConfigureRequest: {
    if (!searchWindow(e->xconfigurerequest.window) ) {
      grab();

      if (validateWindow(e->xconfigurerequest.window) ) {
        XWindowChanges xwc;

        xwc.x = e->xconfigurerequest.x;
        xwc.y = e->xconfigurerequest.y;
        xwc.width = e->xconfigurerequest.width;
        xwc.height = e->xconfigurerequest.height;
        xwc.border_width = e->xconfigurerequest.border_width;
        xwc.sibling = e->xconfigurerequest.above;
        xwc.stack_mode = e->xconfigurerequest.detail;

        XConfigureWindow(display(),
                         e->xconfigurerequest.window,
                         e->xconfigurerequest.value_mask, &xwc);
      }

      ungrab();
    } // else already handled in ShyneboxWindow::handleEvent
  } // ConfigureRequest
      break;
  case MapRequest: {
    sbdbg<<"MapRequest for 0x"<<hex<<e->xmaprequest.window<<dec<<"\n";

    WinClient *winclient = searchWindow(e->xmaprequest.window);

    if (! winclient) {
      BScreen *screen = 0;
      XWindowAttributes attr;
      // find screen
      if (XGetWindowAttributes(display(),
                               e->xmaprequest.window,
                               &attr) && attr.screen != 0) {
        screen = findScreen(XScreenNumberOfScreen(attr.screen) );
      }
      // try with parent if we failed to find screen num
      if (screen == 0)
        screen = searchScreen(e->xmaprequest.parent);

      if (screen == 0) {
        cerr << "Shynebox "<<_SB_CONSOLETEXT(Shynebox, CantMapWindow,
                "Warning! Could not find screen to map window on!", "")<<"\n";
      } else
        screen->createWindow(e->xmaprequest.window);

    } else {
      // we don't handle MapRequest in ShyneboxWindow::handleEvent
      if (winclient->sbwindow() )
        winclient->sbwindow()->mapRequestEvent(e->xmaprequest);
    }
  } // Map Request
    break;
  case MapNotify:
    break; // handled directly in ShyneboxWindow::handleEvent
  case UnmapNotify:
    handleUnmapNotify(e->xunmap);
    break;
  case MappingNotify: // Update stored modifier mapping
    if (e->xmapping.request == MappingKeyboard
        || e->xmapping.request == MappingModifier) {
      s_key_reloader.xmapping = e->xmapping;
      m_key_reload_timer.start();
    }
    break;
  case CreateNotify:
    break;
  case DestroyNotify:
    {
    WinClient *winclient = searchWindow(e->xdestroywindow.window);
    if (winclient != 0) {
      ShyneboxWindow *win = winclient->sbwindow();
      if (win)
        win->destroyNotifyEvent(e->xdestroywindow);
    }
    }
    break;
  case MotionNotify:
    m_last_time = e->xmotion.time;
    break;
  case PropertyNotify:
    {
    m_last_time = e->xproperty.time;
    WinClient *winclient = searchWindow(e->xproperty.window);
    if (winclient == 0)
      break;
    // most of them are handled in ShyneboxWindow::handleEvent
    // but some special cases like ewmh propertys needs to be checked
    m_ewmh->propertyNotify(*winclient, e->xproperty.atom);
    } break;
  case EnterNotify:
    {
    m_last_time = e->xcrossing.time;

    BScreen *screen = 0;
    if ((e->xcrossing.window == e->xcrossing.root)
        && (screen = searchScreen(e->xcrossing.window) ) )
      screen->imageControl().installRootColormap();
    } break;
  case LeaveNotify:
    m_last_time = e->xcrossing.time;
    break;
  case Expose:
    break;
  case KeyRelease:
  case KeyPress:
    break;
  case ColormapNotify:
    {
    BScreen *screen = searchScreen(e->xcolormap.window);

    screen->setRootColormapInstalled((e->xcolormap.state ==
                                      ColormapInstalled) ? true : false);
    } break;
  case FocusIn:
    {
    // a grab is something of a pseudo-focus event, so we ignore
    // them, here we ignore some window receiving it
    if (e->xfocus.mode == NotifyGrab
        || e->xfocus.mode == NotifyUngrab
        || e->xfocus.detail == NotifyPointer
        || e->xfocus.detail == NotifyInferior)
      break;

    if (tk::Menu::focused()
        && tk::Menu::focused()->window() == e->xfocus.window) {
      m_active_screen.key = findScreen(tk::Menu::focused()->screenNumber() );
      FocusControl::setFocusedWindow(0);
      break;
    }

    WinClient *winclient = searchWindow(e->xfocus.window);
    if (winclient)
      m_active_screen.key = &winclient->screen();
    FocusControl::setFocusedWindow(winclient);
    } break;
  case FocusOut:
    {
    // and here we ignore some window losing the special grab focus
    if (e->xfocus.mode == NotifyGrab
        || e->xfocus.mode == NotifyUngrab
        || e->xfocus.detail == NotifyPointer
        || e->xfocus.detail == NotifyInferior)
      break;

    WinClient *winclient = searchWindow(e->xfocus.window);
    if ((winclient == FocusControl::focusedWindow()
          || FocusControl::focusedWindow() == 0)
        && // we don't unfocus a moving window
        (!winclient || !winclient->sbwindow()
          || !winclient->sbwindow()->isMoving() ) )
      revertFocus();
    }
    break;
  case ClientMessage:
    handleClientMessage(e->xclient);
    break;
  default:
    {
    if (e->type == s_randr_event_type) {
      XRRUpdateConfiguration(e);
      // update root window size in screen
      BScreen *scr = searchScreen(e->xany.window);
      scr->updateSize();
    }
    } // default
  } // switch(e->type)
} // handleEvent

void Shynebox::handleUnmapNotify(XUnmapEvent &ue) {
  BScreen *screen = searchScreen(ue.event);

  // Ignore all EnterNotify events until the pointer actually moves
  screen->focusControl().ignoreAtPointer();

  if (ue.event != ue.window && !ue.send_event)
    return;

  WinClient *winclient = searchWindow(ue.window);

  if (winclient != 0) {
    ShyneboxWindow *win = winclient->sbwindow();
    if (!win) {
      delete winclient;
      return;
    }

    // this should delete client and adjust m_focused_window if necessary
    win->unmapNotifyEvent(ue);

  // according to http://tronche.com/gui/x/icccm/sec-4.html#s-4.1.4
  // a XWithdrawWindow is
  //   1) unmapping the window (which leads to the upper branch)
  //   2) sends an synthetic unampevent (which is handled below)
  } else if (screen && ue.send_event) {
    XDeleteProperty(display(), ue.window, SbAtoms::instance()->getWMStateAtom() );
    XUngrabButton(display(), AnyButton, AnyModifier, ue.window);
  }
} //handleUnmapNotify

/**
 * Handles XClientMessageEvent
 */
void Shynebox::handleClientMessage(XClientMessageEvent &ce) {
#ifdef DEBUG
  char * atom = 0;
  if (ce.message_type)
    atom = XGetAtomName(tk::App::instance()->display(), ce.message_type);

  sbdbg << "ClientMessage. data.l[0]=0x"<<hex<<ce.data.l[0]<<
           "  message_type=0x"<<ce.message_type<<dec<<" = \""<<atom<<"\"\n";

  if (ce.message_type && atom)
    XFree((char *) atom);
#endif

  if (ce.format != 32) // format is essentially data size
    return;

  if (ce.message_type == m_sbatoms->getWMChangeStateAtom() ) {
    WinClient *winclient = searchWindow(ce.window);
    if (!winclient || !winclient->sbwindow() || !winclient->validateClient() )
      return;

    if (ce.data.l[0] == IconicState)
      winclient->sbwindow()->iconify();
    if (ce.data.l[0] == NormalState)
      winclient->sbwindow()->deiconify();
  } else {
    WinClient *winclient = searchWindow(ce.window);
    BScreen *screen = searchScreen(ce.window);
#if USE_TOOLBAR
    for (auto s : m_screens)
      s->systrayCheckClientMsg(ce, screen, winclient);
#endif
    m_ewmh->checkClientMessage(ce, screen, winclient);
  } // if getWMChangeStateAtom
} // handleClientMessage

void Shynebox::windowDied(Focusable &focusable) {
  ShyneboxWindow *sbwin = focusable.sbwindow();

  // make sure each workspace get this
  BScreen &scr = focusable.screen();
  scr.removeWindow(sbwin);
  // not sure if this is right? want revert? but 'die' is rare
  // haven't noticed wacky focus so leaving alone
  if (FocusControl::focusedSbWindow() == sbwin)
    FocusControl::setFocusedSbWindow(0);
}

void Shynebox::clientDied(Focusable &focusable) {
  WinClient &client = dynamic_cast<WinClient &>(focusable);

  m_ewmh->updateClientClose(client);
  m_remember->updateClientClose(client);

  BScreen &screen = client.screen();

  // At this point, we trust that this client is no longer in the
  // client list of its frame (but it still has reference to the frame)
  // We also assume that any remaining active one is the last focused one

  // This is where we revert focus on window close
  // NOWHERE ELSE!!!
  if (FocusControl::focusedWindow() == &client) {
    FocusControl::unfocusWindow(client);
    // make sure nothing else uses this window before focus reverts
    FocusControl::setFocusedWindow(0);
  } else if (FocusControl::expectingFocus() == &client) {
    FocusControl::setExpectingFocus(0);
    revertFocus();
  }

  screen.removeClient(client);
} // clientDied

void Shynebox::windowWorkspaceChanged(ShyneboxWindow &win) {
  m_ewmh->updateWorkspace(win);
  if (win.isMoving() ) {
    win.raise();
    win.focus();
  }
}

void Shynebox::windowStateChanged(ShyneboxWindow &win) {
  m_ewmh->updateState(win);
  // if window changed to iconic state
  // add to icon list
  if (win.isIconic() ) {
    win.screen().addIcon(&win);
    Workspace *space = win.screen().getWorkspace(win.workspaceNumber() );
    if (space != 0)
      space->removeWindow(&win, true);
  }

  // if we're sticky then reassociate window
  // to all workspaces
  if (win.isStuck() ) {
    BScreen &scr = win.screen();
    if (scr.currentWorkspaceID() != win.workspaceNumber() )
      scr.reassociateWindow(&win, scr.currentWorkspaceID(), true);
  }
}

void Shynebox::windowLayerChanged(ShyneboxWindow &win) {
  m_ewmh->updateState(win);
}

void Shynebox::setupFrame(ShyneboxWindow &win) {
  m_remember->setupFrame(win);
  m_ewmh->setupFrame(win);
}

void Shynebox::setupClient(WinClient &winclient) {
#if USE_TOOLBAR
  for (auto s : m_screens)
    s->setupSystrayClient(winclient);
#endif
  m_ewmh->setupClient(winclient);
  m_remember->setupClient(winclient);
}

/* Not implemented until we know how it'll be used
 * Recall that this refers to ICCCM groups, not shynebox tabgroups
 * See ICCCM 4.1.11 for details
 */
/*
WinClient *Shynebox::searchGroup(Window window) {
}
*/

void Shynebox::saveWindowSearch(Window window, WinClient *data) {
  m_window_search[window] = data;
}

/* some windows relate to the whole group */
void Shynebox::saveWindowSearchGroup(Window window, ShyneboxWindow *data) {
  m_window_search_group[window] = data;
}

void Shynebox::saveGroupSearch(Window window, WinClient *data) {
  m_group_search.insert(pair<const Window, WinClient *>(window, data) );
}


void Shynebox::removeWindowSearch(Window window) {
  m_window_search.erase(window);
}

void Shynebox::removeWindowSearchGroup(Window window) {
  m_window_search_group.erase(window);
}

void Shynebox::removeGroupSearch(Window window) {
  m_group_search.erase(window);
}

/// restarts shynebox
void Shynebox::restart(const char *prog) {
  shutdown();
  m_state.restarting = true;

  if (prog && *prog != '\0')
    m_restart_argument = prog;
}

// prepares shynebox for a shutdown. when x_wants_down is != 0 we assume that
// the xserver is about to shutdown or is in the midst of shutting down
// already. trying to cleanup over a shaky xserver connection is pointless and
// might lead to hangups.
void Shynebox::shutdown(int x_wants_down) {
  if (m_state.shutdown)
    return;

  Display *dpy = tk::App::instance()->display();
  m_state.shutdown = true;

  XSetInputFocus(dpy, PointerRoot, None, CurrentTime);

  if (x_wants_down == 0) {
    for (auto it : m_screens)
      it->shutdown();
    sync(false);
  }
}

/// saves resources
void Shynebox::save_rc() {
  _SB_USES_NLS;

  string cfgfile(getRcFilename() );

  if (!cfgfile.empty() ) {
    if (!m_configmanager.save(cfgfile.c_str() ) )
      cerr << "Error writing rc file! Check path, file permissions or if file is open.\n";
  } else
    cerr<<_SB_CONSOLETEXT(Shynebox, BadRCFile, "rc filename is empty!", "Bad settings file")<<"\n";
} // save_rc

/// filename of resource file
string Shynebox::getRcFilename() {
  if (m_config.file.empty() )
    return getDefaultDataFilename(RC_INIT_FILE);
  return m_config.file;
}

/// Provides default filename of data file
string Shynebox::getDefaultDataFilename(const char *name) const {
  return m_config.path + string("/") + name;
}

/// loads resources
void Shynebox::load_rc() {
  _SB_USES_NLS;

  string cfgfile(getRcFilename() );

  // loads defaults regardless of file
  if (!m_configmanager.load(cfgfile.c_str() ) )
    cerr << _SB_CONSOLETEXT(Shynebox, CantLoadRCFile,
           "Failed to load config. Using defaults.",
           "Failed trying to read rc file") << ":" << cfgfile << "\n";

  m_config.colors_per_channel = std::clamp(m_config.colors_per_channel, 2, 6);
} // load_rc

// used in ReconfigureShyneboxCmd (shared commands)
void Shynebox::reconfigure() {
  m_key_reload_timer.stop();
  load_rc();
  m_reconfigure_wait = true;
  m_reconfig_timer.start();
}

// only caller SbCommands setStyleCmd
void Shynebox::reconfigThemes() {
  for (auto it: m_screens)
    it->reconfigThemes();
} // reconfigThemes

BScreen *Shynebox::searchScreen(Window window) {
  // search root windows
  for (auto it : m_screens)
    if (it && it->rootWindow() == window)
      return it;
  return m_screens.front();
}

WinClient *Shynebox::searchWindow(Window window) {
  for (auto &it : m_window_search)
    if (it.first == window)
      return it.second;

  for (auto &it : m_window_search_group)
    if (it.first == window)
      return &it.second->winClient();

  return 0;
}

// not sure how multi screen works
BScreen *Shynebox::findScreen(int id) {
  for (auto &s : m_screens)
    if (s->screenNumber() == id)
      return s;
  // this shouldn't be reachable
  return m_screens.front();
}

void Shynebox::timed_reconfigure() {
  if (m_reconfigure_wait) {
    for (auto it : m_screens)
      it->reconfigure();
    m_key->reconfigure();
    m_remember->reconfigure();
    tk::MenuSearch::setMode(m_config.menusearch);
  }

  m_reconfigure_wait = false;
}

void Shynebox::revertFocus() {
  bool revert = m_active_screen.key && !m_showing_dialog;
  if (revert) {
    // see if there are any more focus events in the queue
    XEvent ev;
    while (XCheckMaskEvent(display(), FocusChangeMask, &ev) )
      handleEvent(&ev);
    if (FocusControl::focusedWindow() || FocusControl::expectingFocus() )
      return; // already handled

    Window win;
    int ignore;
    XGetInputFocus(display(), &win, &ignore);

    // we only want to revert focus if it's left dangling, as some other
    // application may have set the focus to an unmanaged window
    if (win != None && win != PointerRoot && !searchWindow(win)
        && win != m_active_screen.key->rootWindow().window() )
      revert = false;
  }

  if (revert)
    FocusControl::revertFocus(*m_active_screen.key);
  else
    FocusControl::setFocusedWindow(0);
}

bool Shynebox::validateClient(const WinClient *client) const {
  for (auto it : m_window_search)
    if (it.second == client)
      return true;
  return false;
}

void Shynebox::updateFrameExtents(ShyneboxWindow &win) {
  m_ewmh->updateFrameExtents(win);
}

void Shynebox::workspaceCountChanged(BScreen& screen) {
  m_ewmh->updateWorkspaceCount(screen);
}

void Shynebox::workspaceChanged(BScreen& screen) {
  if (!m_state.starting)
    m_ewmh->updateCurrentWorkspace(screen);
}

void Shynebox::workspaceNamesChanged(BScreen &screen) {
  m_ewmh->updateWorkspaceNames(screen);
}

void Shynebox::clientListChanged(BScreen &screen) {
  m_ewmh->updateClientList(screen);
  screen.updateClientMenus();
}

void Shynebox::focusedWindowChanged(BScreen &screen, WinClient *client) {
  m_ewmh->updateFocusedWindow(screen, client ? client->window() : 0);
#if USE_TOOLBAR
  screen.updateToolbar(false);
#endif
}

void Shynebox::workspaceAreaChanged(BScreen &screen) {
  m_ewmh->updateWorkarea(screen);
  m_ewmh->updateGeometry(screen);
}

// Copyright (c) 2023 Shynebox - zlice
//
// Copyright (c) 2001 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
//
// blackbox.cc for blackbox - an X11 Window manager
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
