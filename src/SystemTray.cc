// SystemTray.cc for Shynebox Window Manager

#include "SystemTray.hh"

#include "tk/EventManager.hh"
#include "tk/ImageControl.hh"
#include "tk/StringUtil.hh"
#include "tk/TextUtils.hh"

#include "shynebox.hh"
#include "WinClient.hh"
#include "Screen.hh"
#include "Debug.hh"

#include <X11/Xutil.h>
#include <X11/Xatom.h>

#include <sstream>
#include <vector>

using std::string;
using std::hex;
using std::dec;

namespace {

void getScreenCoordinates(Window win, int x, int y, int &screen_x, int &screen_y) {
  XWindowAttributes attr;
  if (XGetWindowAttributes(tk::App::instance()->display(), win, &attr) == 0)
    return;

  Window unused_win;
  Window parent_win;
  Window root_win = 0;
  Window* unused_childs = 0;
  unsigned int unused_number;

  XQueryTree(tk::App::instance()->display(), win,
             &root_win,
             &parent_win,
             &unused_childs, &unused_number);

  if (unused_childs != 0)
    XFree(unused_childs);

  XTranslateCoordinates(tk::App::instance()->display(),
                        parent_win, root_win,
                        x, y,
                        &screen_x, &screen_y, &unused_win);
} // getScreenCoordinates

};

/// helper class for tray windows, so we dont call XDestroyWindow
class SystemTray::TrayWindow : public tk::SbWindow {
public:
  TrayWindow(Window win, bool using_xembed):tk::SbWindow(win),
         m_order(0),
         m_visible(false),
         m_xembedded(using_xembed) {
    setEventMask(PropertyChangeMask);
  }

  void pinByClassname(const std::vector<std::string> &left, const std::vector<std::string> &right);

  bool isVisible() { return m_visible; }
  bool isXEmbedded() { return m_xembedded; }
  void show() {
    if (!m_visible) {
      m_visible = true;
      tk::SbWindow::show();
    }
  }
  void hide() {
    if (m_visible) {
      m_visible = false;
      tk::SbWindow::hide();
    }
  }

/* Flags for _XEMBED_INFO */
#define XEMBED_MAPPED                   (1 << 0)

  bool getMappedDefault() const {
    Atom actual_type;
    int actual_format;
    unsigned long nitems, bytes_after;
    unsigned long *prop;
    Atom embed_info = SystemTray::getXEmbedInfoAtom();
    if (property(embed_info, 0l, 2l, false, embed_info,
                 &actual_type, &actual_format, &nitems, &bytes_after,
                 (unsigned char **) &prop)
          && prop != 0) {
      sbdbg << "(SystemTray::TrayWindow::getMappedDefault(): XEMBED_MAPPED = "
            << (bool)(static_cast<unsigned long>(prop[1]) & XEMBED_MAPPED) << "\n";
      XFree(static_cast<void *>(prop) );
    }
    return true;
  }

  int m_order = 0; // see pinByClassname(), -2, 0, 1, 4

private:
  bool m_visible;
  bool m_xembedded; // using xembed protocol? (i.e. unmap when done)
}; // SystemTray TrayWindow

// orders the tray list by crawling pin lists
// and assigning TrayWindow's m_order
void SystemTray::TrayWindow::pinByClassname(const std::vector<std::string> &left,
                                           const std::vector<std::string> &right) {
  XClassHint *xclasshint = 0;
  xclasshint = XAllocClassHint();

  if (XGetClassHint(Shynebox::instance()->display(),
                    this->window(), xclasshint) ) {
    const char *classname = xclasshint->res_class;

    using tk::StringUtil::strcasestr;
    int i;
    for (i = (int)left.size() - 1 ; i >= 0 ; i--) {
      // if *classname* contains *string*
      if (strcasestr(classname, left[i].c_str() ) ) {
        m_order = 0 - (left.size() - i); // the more left, the negative (<0)
        break;
      }
    }

    if (i < 0) { // if left not found
    for (i = 0 ; i < (int)right.size() ; i++) {
      if (strcasestr(classname, right[i].c_str() ) ) {
        m_order = 1+i; // the more right, the positive (>0)
        break;
      }
    }
    }
  } // if XGetClassHint

  if (xclasshint) {
    XFree(xclasshint->res_name);
    XFree(xclasshint->res_class);
    XFree(xclasshint);
  }
} // TrayWindow pinByClassName

SystemTray::SystemTray(const tk::SbWindow& parent,
        tk::ThemeProxy<ToolTheme> &theme, BScreen& screen):
      ToolbarItem(ToolbarItem::FIXED),
      m_window(parent, 0, 0, 1, 1, ExposureMask | ButtonPressMask | ButtonReleaseMask |
               SubstructureNotifyMask | SubstructureRedirectMask),
      m_theme(theme),
      m_screen(screen),
      m_pixmap(0), m_num_visible_clients(0),
      m_selection_owner(m_window, 0, 0, 1, 1, SubstructureNotifyMask, false, false,
                        CopyFromParent, InputOnly),
      m_rc_systray_pinleft(*screen.m_cfgmap["systray.pinLeft"]),
      m_rc_systray_pinright(*screen.m_cfgmap["systray.pinRight"]) {
  tk::StringUtil::stringtok(m_pinleft, m_rc_systray_pinleft, " ,");
  tk::StringUtil::stringtok(m_pinright, m_rc_systray_pinright, " ,");

  tk::EventManager::instance()->add(*this, m_window);
  tk::EventManager::instance()->add(*this, m_selection_owner);

  Shynebox* shynebox = Shynebox::instance();
  Display *disp = shynebox->display();

  // get selection owner and see if it's free
  string atom_name = getNetSystemTrayAtom(m_window.screenNumber() );
  Atom tray_atom = XInternAtom(disp, atom_name.c_str(), False);
  Window owner = XGetSelectionOwner(disp, tray_atom);
  if (owner != 0) {
    sbdbg<<"(SystemTray(const tk::SbWindow) ): can't set owner!\n";
    return;  // the're can't be more than one owner
  }

  // ok, it was free. Lets set owner
  sbdbg<<"(SystemTray(const tk::SbWindow) ): SETTING OWNER!\n";

  // set owner
  XSetSelectionOwner(disp, tray_atom, m_selection_owner.window(), CurrentTime);

  // send selection owner msg
  Window root_window = m_screen.rootWindow().window();
  XEvent ce;
  ce.xclient.type = ClientMessage;
  ce.xclient.message_type = XInternAtom(disp, "MANAGER", False);
  ce.xclient.display = disp;
  ce.xclient.window = root_window;
  ce.xclient.format = 32;
  ce.xclient.data.l[0] = CurrentTime; // timestamp
  ce.xclient.data.l[1] = tray_atom; // manager selection atom
  ce.xclient.data.l[2] = m_selection_owner.window(); // the window owning the selection
  ce.xclient.data.l[3] = 0l; // selection specific data
  ce.xclient.data.l[4] = 0l; // selection specific data

  XSendEvent(disp, root_window, false, StructureNotifyMask, &ce);

  update();
} // SystemTray class init

SystemTray::~SystemTray() {
  Display *disp = Shynebox::instance()->display();

  // get selection owner and see if it's free
  string atom_name = getNetSystemTrayAtom(m_window.screenNumber() );
  Atom tray_atom = XInternAtom(disp, atom_name.c_str(), False);

  // Properly give up selection.
  XSetSelectionOwner(disp, tray_atom, None, CurrentTime);
  removeAllClients();

  if (m_pixmap)
    m_screen.imageControl().removeImage(m_pixmap);
  // ~SbWindow cleans EventManager
} // SystemTray class destroy

void SystemTray::move(int x, int y) {
  m_window.move(x, y);
}

void SystemTray::resize(unsigned int width, unsigned int height) {
  if (width != m_window.width() || height != m_window.height() ) {
    m_window.resize(width, height);
    if (m_num_visible_clients)
      rearrangeClients();
    m_screen.updateToolbar();
  }
}

void SystemTray::moveResize(int x, int y,
                        unsigned int width, unsigned int height) {
  if (width != m_window.width() || height != m_window.height() ) {
    m_window.moveResize(x, y, width, height);
    if (m_num_visible_clients)
      rearrangeClients();
    m_screen.updateToolbar();
  } else
    move(x, y);
}

void SystemTray::hide() {
  m_window.hide();
}

void SystemTray::show() {
  update();
  m_window.show();
}

// the items in systray should be squares
unsigned int SystemTray::width() const {
  if (orientation() >= tk::ROT90) // vert
    return m_window.width();
  const unsigned int cli_sz = m_clients.empty() ? 0 : m_clients.front()->width();
  return m_num_visible_clients * (cli_sz + 2 * m_theme->border().width() );
}

unsigned int SystemTray::height() const {
  if (orientation() <= tk::ROT180) // horz
    return m_window.height();
  const unsigned int cli_sz = m_clients.empty() ? 0 : m_clients.front()->height();
  return m_num_visible_clients * (cli_sz + 2 * m_theme->border().width() );
}

unsigned int SystemTray::borderWidth() const {
  return m_window.borderWidth();
}

// wrapper to check before clientMessage
bool SystemTray::checkClientMessage(const XClientMessageEvent &ce,
                        BScreen * screen, WinClient * const winclient) {
  // must be on the same screen
  if ((screen && screen->screenNumber() != window().screenNumber() )
      || (winclient && winclient->screenNumber() != window().screenNumber() ) )
    return false;
  return clientMessage(ce);
}

bool SystemTray::clientMessage(const XClientMessageEvent &event) {
  static const int SYSTEM_TRAY_REQUEST_DOCK  =  0;
  // debug stuff?
  // static const int SYSTEM_TRAY_BEGIN_MESSAGE =  1;
  // static const int SYSTEM_TRAY_CANCEL_MESSAGE = 2;
  static Atom systray_opcode_atom = XInternAtom(tk::App::instance()->display(),
                                               "_NET_SYSTEM_TRAY_OPCODE", False);

  if (event.message_type == systray_opcode_atom) {
    int type = event.data.l[1];
    if (type == SYSTEM_TRAY_REQUEST_DOCK) {
      sbdbg<<"SystemTray::clientMessage(const XClientMessageEvent): SYSTEM_TRAY_REQUEST_DOCK\n";
      sbdbg<<"window = event.data.l[2] = "<<event.data.l[2]<<"\n";

      addClient(event.data.l[2], true);
    }
    /*
    else if (type == SYSTEM_TRAY_BEGIN_MESSAGE)
        sbdbg<<"BEGIN MESSAGE"<<"\n";
    else if (type == SYSTEM_TRAY_CANCEL_MESSAGE)
        sbdbg<<"CANCEL MESSAGE"<<"\n";
    */
    return true;
  } // if _NET_SYSTEM_TRAY_OPCODE
  return false;
} // clientMessage

SystemTray::ClientList::iterator SystemTray::findClient(Window win) {
  ClientList::iterator it;
  for (it = m_clients.begin() ; it != m_clients.end() ; ++it)
    if ((*it)->window() == win)
      break;
  return it;
}

// wrapper to check nulls before addClient
void SystemTray::setupClient(WinClient &winclient) {
  // must be on the same screen
  // TODO: never use multi screen, not sure this is possible?
  //       this was originally called again below
  //if (winclient.screenNumber() != window().screenNumber() )
  //    return;

  // we dont want a managed window, only kde dockapp
  if (winclient.sbwindow() != 0
      || !winclient.screen().isKdeDockapp(winclient.window() ) )
    return;

  winclient.setEventMask(StructureNotifyMask |
                         SubstructureNotifyMask | EnterWindowMask);
  addClient(winclient.window(), false);
}

void SystemTray::addClient(Window win, bool using_xembed) {
  if (win == 0)
    return;

  ClientList::iterator it = findClient(win);
  if (it != m_clients.end() )
    return;

  Display *disp = Shynebox::instance()->display();
  // make sure we have the same screen number
  XWindowAttributes attr;
  attr.screen = 0;
  if (XGetWindowAttributes(disp, win, &attr) != 0 &&
    attr.screen != 0 &&
    XScreenNumberOfScreen(attr.screen) != window().screenNumber() ) {
    return;
  }

  TrayWindow *traywin = new TrayWindow(win, using_xembed);

  sbdbg<<"SystemTray::addClient(Window): 0x"<<hex<<win<<dec<<"\n";

  m_clients.push_back(traywin);
  tk::EventManager::instance()->add(*this, win);
  traywin->reparent(m_window, 0, 0);
  traywin->addToSaveSet();

  if (using_xembed) {
    static Atom xembed_atom = XInternAtom(disp, "_XEMBED", False);

    // send embedded message
    XEvent ce;
    ce.xclient.type = ClientMessage;
    ce.xclient.message_type = xembed_atom;
    ce.xclient.display = disp;
    ce.xclient.window = win;
    ce.xclient.format = 32;
    ce.xclient.data.l[0] = CurrentTime;       // timestamp
    ce.xclient.data.l[1] = 0;                 // XEMBED_EMBEDDED_NOTIFY;
    ce.xclient.data.l[2] = 0l;                // The protocol version we support
    ce.xclient.data.l[3] = m_window.window(); // the window owning the selection
    ce.xclient.data.l[4] = 0l; // unused

    XSendEvent(disp, win, false, NoEventMask, &ce);
  }

  if (traywin->getMappedDefault() )
    showClient(traywin);

  traywin->pinByClassname(m_pinleft, m_pinright);
  m_clients.sort( [](TrayWindow *a, TrayWindow *b)
    { return a->m_order < b->m_order; } );
  rearrangeClients();
} // addClient

void SystemTray::removeClient(Window win, bool destroyed) {
  ClientList::iterator tray_it = findClient(win);
  if (tray_it == m_clients.end() )
    return;

  sbdbg<<"(SystemTray::removeClient(Window) ): 0x"<<hex<<win<<dec<<"\n";

  TrayWindow *traywin = *tray_it;
  m_clients.erase(tray_it);
  if (!destroyed) {
    traywin->setEventMask(NoEventMask);
    traywin->removeFromSaveSet();
  }
  hideClient(traywin, destroyed);
  delete traywin;
}

void SystemTray::exposeEvent(XExposeEvent &event) {
  (void) event;
  m_window.clear();
}

void SystemTray::handleEvent(XEvent &event) {
  if (event.type == DestroyNotify) {
    removeClient(event.xdestroywindow.window, true);
  } else if (event.type == ReparentNotify && event.xreparent.parent != m_window.window() ) {
    removeClient(event.xreparent.window, false);
  } else if (event.type == UnmapNotify && event.xany.send_event) {
    // we ignore server-generated events, which can occur
    // on restart. The ICCCM says that a client must send
    // a synthetic event for the withdrawn state
    ClientList::iterator it = findClient(event.xunmap.window);
    if (it != m_clients.end() )
      hideClient(*it);
  } else if (event.type == ConfigureNotify) {
    // we got configurenotify from an client
    // check and see if we need to update it's size
    // and we must reposition and resize them to fit
    // our toolbar
    ClientList::iterator it = findClient(event.xconfigure.window);
    if (it != m_clients.end() ) {
      if (static_cast<unsigned int>(event.xconfigure.width) != (*it)->width()
          || static_cast<unsigned int>(event.xconfigure.height) != (*it)->height() ) {
        // the position might differ so we update from our local
        // copy of position
        XMoveResizeWindow(tk::App::instance()->display(), (*it)->window(),
                          (*it)->x(), (*it)->y(),
                          (*it)->width(), (*it)->height() );

        // this was why gaim wasn't centring the icon
        // 2023, pidgin and discord both do this
        (*it)->sendConfigureNotify(0, 0, (*it)->width(), (*it)->height() );
        m_screen.updateToolbar();
      }
    }
  } else if (event.type == PropertyNotify) {
    ClientList::iterator it = findClient(event.xproperty.window);
    if (it != m_clients.end() ) {
      if (event.xproperty.atom == getXEmbedInfoAtom() ) {
        if ((*it)->getMappedDefault() )
          showClient(*it);
        else
          hideClient(*it);
      }
    } // if find client
  } // if-chain event.type
} // handleEvent

// draws/places clients
void SystemTray::rearrangeClients() {
  unsigned int w_rot0 = width(), h_rot0 = height();
  const unsigned int bw = m_theme->border().width();
  tk::translateSize(orientation(), w_rot0, h_rot0);
  unsigned int trayw = m_num_visible_clients*h_rot0 + bw, trayh = h_rot0;
  tk::translateSize(orientation(), trayw, trayh);
  resize(trayw, trayh);
  update();

  // move and resize clients
  int next_x = bw;
  int x, y;
  for (auto client_it : m_clients) {
    if (!client_it->isVisible() )
      continue;
    x = next_x;
    y = bw;
    next_x += h_rot0+bw;
    translateCoords(orientation(), x, y, w_rot0, h_rot0);
    translatePosition(orientation(), x, y, h_rot0, h_rot0, 0);
    int screen_x = 0, screen_y = 0;
    getScreenCoordinates(client_it->window(), client_it->x(), client_it->y(), screen_x, screen_y);

    client_it->moveResize(x, y, h_rot0, h_rot0);
    client_it->sendConfigureNotify(screen_x, screen_y, h_rot0, h_rot0);
  }
} // rearrangeClients

void SystemTray::removeAllClients() {
  BScreen *screen = Shynebox::instance()->findScreen(window().screenNumber() );
  while (!m_clients.empty() ) {
    TrayWindow * traywin = m_clients.back();
    traywin->setEventMask(NoEventMask);

    if (traywin->isXEmbedded() )
      traywin->hide();

    if (screen)
      traywin->reparent(screen->rootWindow(), 0, 0, false);
    traywin->removeFromSaveSet();
    delete traywin;
    m_clients.pop_back();
  }
  m_num_visible_clients = 0;
} // removeAllClients

void SystemTray::hideClient(TrayWindow *traywin, bool destroyed) {
  if (!traywin || !traywin->isVisible() )
    return;

  if (!destroyed)
    traywin->hide();
  m_num_visible_clients--;
  rearrangeClients();
}

void SystemTray::showClient(TrayWindow *traywin) {
  if (!traywin || traywin->isVisible() )
    return;

  if (!m_num_visible_clients)
    show();

  traywin->show();
  m_num_visible_clients++;
  rearrangeClients();
}

void SystemTray::update() {
  if (!m_theme->texture().usePixmap() )
    m_window.setBackgroundColor(m_theme->texture().color() );
  else {
    if (m_pixmap)
      m_screen.imageControl().removeImage(m_pixmap);
    m_pixmap = m_screen.imageControl().renderImage(width(), height(),
                                                   m_theme->texture(), orientation() );
    m_window.setBackgroundPixmap(m_pixmap);
  }
}

Atom SystemTray::getXEmbedInfoAtom() {
  static Atom theatom = XInternAtom(Shynebox::instance()->display(), "_XEMBED_INFO", False);
  return theatom;
}

string SystemTray::getNetSystemTrayAtom(int screen_nr) {
  string atom_name("_NET_SYSTEM_TRAY_S");
  atom_name += tk::StringUtil::number2String(screen_nr);
  return atom_name;
}

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
