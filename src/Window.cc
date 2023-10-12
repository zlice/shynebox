// Window.cc for Shynebox Window Manager

#include "Window.hh"

#include "shynebox.hh"
#include "Screen.hh"
#include "CurrentWindowCmd.hh"
#include "WinClient.hh"
#include "Keys.hh"
#include "SbWinFrameTheme.hh"
#include "SbAtoms.hh"
#include "RootTheme.hh"
#include "Workspace.hh" // window list
#include "SbWinFrame.hh"
#include "WinButtonTheme.hh"
#include "WindowCmd.hh"
#include "Remember.hh"
#include "MenuCreator.hh"
#include "FocusControl.hh"
#include "IconButton.hh"
#include "ScreenPlacement.hh"
#include "Debug.hh"

#include "tk/CommandParser.hh"
#include "tk/EventManager.hh"
#include "tk/LayerManager.hh"
#include "tk/KeyUtil.hh"
#include "tk/SimpleCommand.hh"
#include "tk/StringUtil.hh"

#ifdef SHAPE
#include <X11/extensions/shape.h>
#endif

#include <X11/Xatom.h>
#include <X11/keysym.h>

#include <cstring>
#include <cstdio>
#include <iostream>
#include <cassert>
#include <functional>

#include <unistd.h>

using std::string;
using std::vector;
using std::max;
using std::swap;
using std::dec;
using std::hex;

using namespace tk;

#define RLEnum (int)ResLayers_e

namespace {

// X event scanner for enter/leave notifies - adapted from twm
typedef struct scanargs {
  Window w;
  Bool leave, inferior, enter;
} scanargs;

// look for valid enter or leave events (that may invalidate the earlier one we are interested in)
extern "C" int queueScanner(Display *, XEvent *e, char *args) {
  if (e->type == LeaveNotify
      && e->xcrossing.window == ((scanargs *) args)->w
      && e->xcrossing.mode == NotifyNormal) {
    ((scanargs *) args)->leave = true;
    ((scanargs *) args)->inferior = (e->xcrossing.detail == NotifyInferior);
  } else if (e->type == EnterNotify && e->xcrossing.mode == NotifyUngrab)
    ((scanargs *) args)->enter = true;

  return false;
}

// returns the deepest transientFor, asserting against a close loop
WinClient *getRootTransientFor(WinClient *client) {
  while (client && client->transientFor() ) {
    assert(client != client->transientFor() );
    client = client->transientFor();
  }
  return client;
}

// raise window and do the same for each transient of the current window
void raiseShyneboxWindow(ShyneboxWindow &win) {
  if (win.oplock || win.isIconic() )
    return;

  win.oplock = true;

  // we need to lock actual restacking so that raising above active transient
  // won't do anything nasty
  if (!win.winClient().transientList().empty() )
      win.screen().layerManager().lock();

  if (win.layerItem().raise() )
    for (auto it : win.winClient().transientList() )
      if (it->sbwindow() && !it->sbwindow()->isIconic() )
        raiseShyneboxWindow(*it->sbwindow() );

  win.oplock = false;

  if (!win.winClient().transientList().empty() )
    win.screen().layerManager().unlock();
}

// lower window and do the same for each transient it holds
void lowerShyneboxWindow(ShyneboxWindow &win) {
  if (win.oplock || win.isIconic() )
    return;

  win.oplock = true;

  // we need to lock actual restacking so that raising above active transient
  // won't do anything nasty
  if (!win.winClient().transientList().empty() )
      win.screen().layerManager().lock();

  // lower the windows from the top down, so they don't change stacking order
  #define transients win.winClient().transientList()
  WinClient::TransientList::const_reverse_iterator it =     transients.rbegin();
  WinClient::TransientList::const_reverse_iterator it_end = transients.rend();
  #undef transients
  for (; it != it_end; ++it)
    if ((*it)->sbwindow() && !(*it)->sbwindow()->isIconic() )
      lowerShyneboxWindow(*(*it)->sbwindow() );
      // TODO: should we also check if it is the active client?

  win.layerItem().lower();

  win.oplock = false;
  if (!win.winClient().transientList().empty() )
    win.screen().layerManager().unlock();
}

// raise window and do the same for each transient it holds
void tempRaiseShyneboxWindow(ShyneboxWindow &win) {
  if (win.oplock)
    return;
  win.oplock = true;

  if (!win.isIconic() )
    win.layerItem().tempRaise();

  for (auto it : win.winClient().transientList() )
    if (it->sbwindow() && !it->sbwindow()->isIconic() )
      tempRaiseShyneboxWindow(*it->sbwindow() );

  win.oplock = false;
}

class SetClientCmd:public tk::Command<void> {
public:
  explicit SetClientCmd(WinClient &client):m_client(client) { }
  void execute() {
    m_client.focus();
  }
private:
  WinClient &m_client;
};


// helper class for some STL routines
class ChangeProperty {
public:
  ChangeProperty(Display *disp, Atom prop, int mode,
                 unsigned char *state, int num):m_disp(disp),
                                                m_prop(prop),
                                                m_state(state),
                                                m_num(num),
                                                m_mode(mode) { }
  void operator () (tk::SbWindow *win) {
    XChangeProperty(m_disp, win->window(), m_prop, m_prop,
                    32, m_mode, m_state, m_num);
  }
private:
  Display *m_disp;
  Atom m_prop;
  unsigned char *m_state;
  int m_num;
  int m_mode;
};


// Helper class for getResizeDirection below (only user)
// Tests whether a point is on an edge or the corner.
struct TestCornerHelper {
  int corner_size_px, corner_size_pc;
  bool operator()(int xy, int wh) {
    return xy < corner_size_px  ||  100 * xy < corner_size_pc * wh;
    // The % checking must be right: 0% must fail, 100% must succeed.
  }
};

// create a button ready to be used in the frame-titlebar
WinButton* makeButton(ShyneboxWindow& win, FocusableTheme<WinButtonTheme>& btheme, WinButton::Type btype) {
  tk::ThemeProxy<WinButtonTheme>& theme = win.screen().pressedWinButtonTheme();
  SbWinFrame& frame = win.frame();
  tk::SbWindow& parent = frame.titlebar();
  const unsigned int h = frame.buttonHeight();
  const unsigned int w = h;

  return new WinButton(win, btheme, theme, btype, parent, 0, 0, w, h);
}

} // end anonymous namespace

int ShyneboxWindow::s_num_grabs = 0;
static int s_original_workspace = 0;

ShyneboxWindow::ShyneboxWindow(WinClient &client):
      Focusable(client.screen(), this),
      oplock(false),
      m_creation_time(0),
      m_last_keypress_time(0),
      moving(false), resizing(false),
      m_initialized(false),
      m_attaching_tab(0),
      display(tk::App::instance()->display() ),
      m_button_grab_x(0), m_button_grab_y(0),
      m_last_move_x(0), m_last_move_y(0),
      m_last_resize_h(1), m_last_resize_w(1),
      m_last_action_button(0),
      m_workspace_number(0),
      m_current_state(0),
      m_old_decoration_mask(0),
      m_client(&client),
      m_toggled_decos(false),
      m_focus_protection(Focus::NoProtection),
      m_click_focus(true),
      m_last_button_x(0),  m_last_button_y(0),
      m_button_theme(*this, screen().focusedWinButtonTheme(),
                     screen().unfocusedWinButtonTheme() ),
      m_theme(*this, screen().focusedWinFrameTheme(),
              screen().unfocusedWinFrameTheme() ),
      m_frame(client.screen(), client.depth(), m_state, m_theme),
      m_placed(false),
      m_old_layernum(RLEnum::NORMAL),
      m_parent(client.screen().rootWindow() ),
      m_resize_corner(RIGHTBOTTOM) {

  Shynebox &shynebox = *Shynebox::instance();

  // fetch client size and placement
  XWindowAttributes wattrib;
  if (m_client->getAttrib(wattrib)
      && wattrib.screen  // no screen? shouldn't be possible
      && !wattrib.override_redirect // override redirect
      && m_client->initial_state != WithdrawnState
      && m_client->getWMClassClass() != "DockApp") { // Slit client (removed)

    if (m_client->initial_state == IconicState)
      m_state.iconic = true;

    m_client->setShyneboxWindow(this);
    m_client->setGroupLeftWindow(None); // nothing to the left.

    if (shynebox.haveShape() )
      Shape::setShapeNotify(winClient() );

    m_clientlist.push_back(m_client);

    sbdbg << "ShyneboxWindow::init(this=" << this
          << ", client="<< hex << m_client->window()
          << ", frame = " << frame().window().window() << dec <<")\n";

    associateClient(*m_client);

    frame().setFocusTitle(title() );

    // redirect events from frame to us
    frame().setEventHandler(*this);
    shynebox.saveWindowSearchGroup(frame().window().window(), this);
    shynebox.saveWindowSearchGroup(frame().tabcontainer().window(), this);

    // start on current workspace, may be moved (Remember or restarts)
    m_workspace_number = m_screen.currentWorkspaceID();

    setDecorationMask(WindowState::getDecoMaskFromString(screen().defaultDeco() ), false);

    functions.resize = functions.move = functions.iconify = functions.maximize
                     = functions.close = functions.tabable = true;

    updateMWMHintsFromClient(*m_client);

    m_raise_timer.setTimeout(shynebox.getAutoRaiseDelay() * tk::SbTime::IN_MILLISECONDS);
    tk::SimpleCommand<ShyneboxWindow> *raise_cmd(new tk::SimpleCommand<ShyneboxWindow>(*this,
                                                                       &ShyneboxWindow::raise) );
    m_raise_timer.setCommand(*raise_cmd);
    m_raise_timer.fireOnce(true);

    m_tab_activate_timer.setTimeout(shynebox.getAutoRaiseDelay() * tk::SbTime::IN_MILLISECONDS);
    ActivateTabCmd *activate_tab_cmd(new ActivateTabCmd() );
    m_tab_activate_timer.setCommand(*activate_tab_cmd);
    m_tab_activate_timer.fireOnce(true);

    m_resize_timer.setTimeout(screen().opaqueResizeDelay() * tk::SbTime::IN_MILLISECONDS);
    tk::SimpleCommand<ShyneboxWindow> *resize_cmd(new tk::SimpleCommand<ShyneboxWindow>(*this,
                                                                 &ShyneboxWindow::updateResize) );
    m_resize_timer.setCommand(*resize_cmd);
    m_resize_timer.fireOnce(true);

    if (m_client->isTransient() && m_client->transientFor()->sbwindow() )
      m_state.stuck = m_client->transientFor()->sbwindow()->isStuck();

    if (!m_client->sizeHints().isResizable() ) // no tab for this window
      functions.resize = functions.maximize = decorations.tab = false;

    associateClientWindow();

    setWindowType(m_client->getWindowType() );

    if (shynebox.isStartup() )
      m_placed = true;
    else if (m_client->normal_hint_flags & (PPosition|USPosition) ) {
      m_placed = true; // program/user specified positions
      fitToScreen();
    } else {
      int cur = screen().getHead(sbWindow() );
      // if fully out of x or y view, move in view
      if ((signed)(frame().x() + frame().width() ) < 0
           || frame().x() > (signed)screen().width()
           || (signed)(frame().y() + frame().height() ) < 0
           || frame().y() > (signed)screen().height() )
        move(screen().getHeadX(cur), screen().getHeadY(cur) );
      setOnHead(cur);
      m_placed = false; // allow placement strategy to fix position
      if (m_state.maximized || m_state.fullscreen) {
        frame().applyState();
        frameExtentChanged();
        shynebox.windowStateChanged(*this);
      }
    } // shynebox.isStartup()

    updateButtons(); // add buttons BEFORE decoration redraw
    // we must do this now, or else resizing may not work properly
    applyDecorations();

    // if a client request no decor (gedit, splash screens) the
    // default of using a titlebar has offset it lower than
    // where it should be in the frame.
    if (!decorations.titlebar)
      moveResizeClient(client);

    shynebox.setupFrame(*this); // setup remember and ewmh

    // TODO: currently whole screen, could do per head
    // fit oversized windows to screen
    if (!m_state.fullscreen) {
      unsigned int new_width = 0, new_height = 0;
      if (m_client->width() >= screen().width() )
        new_width = 2 * screen().width() / 3;
      if (m_client->height() >= screen().height() )
        new_height = 2 * screen().height() / 3;
      if (new_width || new_height) {
        resize(new_width ? new_width : width(), new_height ? new_height : height() );
        m_placed = false;
      }
    }

    // this window is managed, we are now allowed to modify actual state
    m_initialized = true;

    // this is done up a bit, assume it's in case Remember changes it
    if (m_workspace_number >= screen().numberOfWorkspaces() )
      m_workspace_number = screen().currentWorkspaceID();

    // if we're a transient then we should be on the same layer and workspace
    ShyneboxWindow* twin = m_client->transientFor() ? m_client->transientFor()->sbwindow() : 0;
    // how can you be transient for yourself?
    if (twin && twin != this) {
      if (twin->layerNum() < RLEnum::DESKTOP) { // don't confine layer for desktops
        layerItem().setLayer(twin->layerItem().getLayer() );
        m_state.layernum = twin->layerNum();
      }
      m_workspace_number = twin->workspaceNumber();
      const int x = twin->frame().x() + int(twin->frame().width() - frame().width() ) / 2;
      const int y = twin->frame().y() + int(twin->frame().height() - frame().height() ) / 2;
      frame().move(x, y); // fit to parent
      fitToScreen(); // fit to screen just in case
      m_placed = true;
    } else // if no parent then set default layer
      moveToLayer(m_state.layernum, m_state.layernum != RLEnum::NORMAL);

    sbdbg << "ShyneboxWindow::init("<<title().logical()
          << ") transientFor: "<<m_client->transientFor()<<"\n";

    if (twin)
      sbdbg << "ShyneboxWindow::init("<<title().logical()
            << ") transientFor->title(): "<<twin->title().logical()<<"\n";

    screen().getWorkspace(m_workspace_number)->addWindow(*this);

    // move window to a requested or determined viewable space
    if (m_placed) {
      unsigned int real_width = frame().width();
      unsigned int real_height = frame().height();
      if (!shynebox.isStartup() )
        frame().applySizeHints(real_width, real_height);
      moveResize(frame().x(), frame().y(), real_width, real_height);
    } else
      placeWindow(screen().getCurHead() ); // monitor where mouse is

    setFocusFlag(false); // update graphics before mapping

    if (m_state.stuck) {
      m_state.stuck = false;
      stick();
    }

    if (m_state.shaded) { // start shaded
      m_state.shaded = false;
      shade();
    }

    if (m_state.iconic) {
      m_state.iconic = false;
      iconify();
    } else if (m_workspace_number == screen().currentWorkspaceID() ) {
      m_state.iconic = true;
      deiconify(false);
      // check if we should prevent this window from gaining focus
      m_focused = false; // deiconify sets this
      if (!shynebox.isStartup() && isFocusNew() ) {
        Focus::Protection fp = m_focus_protection;
        m_focus_protection &= ~Focus::Deny; // new windows run as "Refuse"
        m_focused = focusRequestFromClient(*m_client);
        m_focus_protection = fp;
        if (!m_focused)
          lower();
      }
    } // if iconic - else on ws

    if (m_state.fullscreen) {
      m_state.fullscreen = false;
      setFullscreen(true);
    }

    if (m_state.maximized) {
      int tmp = m_state.maximized;
      m_state.maximized = WindowState::MAX_NONE;
      setMaximizedState(tmp);
    }

    shynebox.windowWorkspaceChanged(*this);
    m_creation_time = tk::SbTime::mono();
    shynebox.sync(false);
  } // end old init - basically not slit client (removed) and valid env

  if (!m_initialized)
    return;

  // add the window to the focus list
  // always add to front on startup to keep the focus order the same
  if (isFocused() || shynebox.isStartup() )
    screen().focusControl().addFocusWinFront(*this);
  else
    screen().focusControl().addFocusWinBack(*this);

  // allows click to focus and maps other key-actions
  shynebox.keys()->registerWindow(frame().window().window(),
                                    *this, Keys::ON_WINDOW);

} // ShyneboxWindow class "init"

ShyneboxWindow::~ShyneboxWindow() {
  if (WindowCmd<void>::window() == this)
    WindowCmd<void>::setWindow(0);
  if (SbMenu::window() == this)
    SbMenu::setWindow(0);
  if (Shynebox::instance()->keys() != 0)
    Shynebox::instance()->keys()->
      unregisterWindow(frame().window().window() );

  sbdbg << "starting ~ShyneboxWindow(" << this << ","
        << (m_client ? m_client->title().logical().c_str() : "") << ")\n"
        << "num clients = " << numClients() << "\n"
        << "curr client = "<< m_client << "\n"
        << "m_labelbuttons.size = " << m_labelbuttons.size() << "\n";

  if (moving)
    stopMoving(true);
  if (resizing)
    stopResizing(true);
  if (m_attaching_tab)
    attachTo(0, 0, true);

  // no longer a valid window to do stuff with
  Shynebox::instance()->removeWindowSearchGroup(frame().window().window() );
  Shynebox::instance()->removeWindowSearchGroup(frame().tabcontainer().window() );

  for (auto it : m_labelbuttons)
    frame().removeTab(it.second);

  m_labelbuttons.clear();

  m_raise_timer.stop();
  m_tab_activate_timer.stop();

  Shynebox::instance()->windowDied(*this);

  if (m_client != 0 && !m_screen.isShuttingdown() )
    delete m_client; // this also removes client from our list
  m_client = 0;

  if (m_clientlist.size() > 1) {
    sbdbg<<"(~ShyneboxWindow() ) WARNING! clientlist > 1\n";
    while (!m_clientlist.empty() )
      detachClient(*m_clientlist.back() );
  }

  if (!screen().isShuttingdown() )
    screen().focusControl().removeWindow(*this);

  sbdbg<<"~ShyneboxWindow("<<this<<")\n";
} // ShyneboxWindow class destroy

void ShyneboxWindow::fitToScreen() {
  // NOTE: setOnHead does similar but for head
  int       left = 0, top = 0;
  const int bw   = frame().window().borderWidth() * 2,
            btm  = screen().height(),
            rght = screen().width();

  // ensure on-screen horz
  if ((signed)(frame().x() + frame().width() + bw) < left)
    { } // left = left; // nop
  else if (frame().x() > rght)
    left = rght - frame().width() - bw;
  else
    left = frame().x(); // where it was

  // ensure on-screen vert
  if ((signed)(frame().y() + frame().height() + bw) < top)
    { } // top = top; // nop
  else if (frame().y() > btm)
    top = btm - frame().height() - bw;
  else
    top = frame().y(); // where it was

  frame().move(left, top); // checks if actual move or backs out
}

// attach a client to this window and destroy old window
void ShyneboxWindow::attachClient(WinClient &client, int x, int y) {
  //!! TODO: check for isGroupable in client
  if (client.sbwindow() == this)
    return;

  menu().hide();

  // reparent client win to this frame
  frame().setClientWindow(client);
  bool was_focused = false;
  WinClient *focused_win = 0;

  // get the current window on the end of our client list
  Window leftwin = None;
  if (!clientList().empty() )
    leftwin = clientList().back()->window();

  client.setGroupLeftWindow(leftwin);

  if (client.sbwindow() != 0) {
    ShyneboxWindow *old_win = client.sbwindow(); // store old window

    if (FocusControl::focusedSbWindow() == old_win)
      was_focused = true;

    ClientList::iterator client_insert_pos = getClientInsertPosition(x, y);
    tk::TextButton *button_insert_pos = NULL;
    if (client_insert_pos != m_clientlist.end() )
      button_insert_pos = m_labelbuttons[*client_insert_pos];

    // make sure we set new window search for each client
    for (auto &client_it : old_win->clientList() ) {
      // reparent window to this
      frame().setClientWindow(*client_it);

      moveResizeClient(*client_it);

      // create a labelbutton for this client and
      // associate it with the pointer
      associateClient(*client_it);

      // null if we want the new button at the end of the list
      if (x >= 0 && button_insert_pos)
        frame().moveLabelButtonLeftOf(*m_labelbuttons[client_it], *button_insert_pos);
    }

    // add client and move over all attached clients
    // from the old window to this list
    m_clientlist.splice(client_insert_pos, old_win->m_clientlist);
    updateClientLeftWindow();
    old_win->m_client = 0;

    delete old_win;
  } else { // client.sbwindow() == 0
    associateClient(client);
    moveResizeClient(client);

    // right now, this block only happens with new windows or on restart
    bool is_startup = Shynebox::instance()->isStartup();

    // we use m_focused as a signal to focus the window when mapped
    if (isFocusNew() && !is_startup)
      m_focused = focusRequestFromClient(client);
    focused_win = (isFocusNew() || is_startup) ? &client : m_client;

    m_clientlist.push_back(&client);
  } // if client.sbwindow

  // make sure that the state etc etc is updated for the new client
  Shynebox::instance()->windowStateChanged(*this);
  Shynebox::instance()->windowWorkspaceChanged(*this);
  Shynebox::instance()->windowLayerChanged(*this);

  if (was_focused) {
    // don't ask me why, but client doesn't seem to keep focus in new window
    // and we don't seem to get a FocusIn event from setInputFocus
    client.focus();
    FocusControl::setFocusedWindow(&client);
  } else {
    if (!focused_win)
      focused_win = screen().focusControl().lastFocusedWindow(*this);
    if (focused_win) {
      setCurrentClient(*focused_win, false);
      if (isIconic() && m_focused)
        deiconify();
    }
  }
  frame().reconfigure();
} // attachClient

// detach client from window and create a new window for it
bool ShyneboxWindow::detachClient(WinClient &client) {
  if (client.sbwindow() != this || numClients() <= 1)
    return false;

  Window leftwin = None;
  ClientList::iterator client_it, client_it_after;
  client_it = client_it_after =
      find(clientList().begin(), clientList().end(), &client);

  if (client_it != clientList().begin() )
    leftwin = (*(--client_it) )->window();

  if (++client_it_after != clientList().end() )
    (*client_it_after)->setGroupLeftWindow(leftwin);

  removeClient(client);
  screen().createWindow(client);
  return true;
}

void ShyneboxWindow::detachCurrentClient() {
  // should only operate if we had more than one client
  if (numClients() <= 1)
    return;
  WinClient &client = *m_client;
  detachClient(*m_client);
  if (client.sbwindow() != 0)
    client.sbwindow()->show();
}

// removes client from client list, does not create new shyneboxwindow for it
bool ShyneboxWindow::removeClient(WinClient &client) {
  if (client.sbwindow() != this || numClients() == 0)
    return false;

  sbdbg<<"("<<__FUNCTION__<<")["<<this<<"]\n";

  // if it is our active client, deal with it...
  if (m_client == &client) {
    WinClient *next_client = screen().focusControl().lastFocusedWindow(*this, m_client);
    if (next_client != 0)
      setCurrentClient(*next_client, false);
  }

  menu().hide();

  m_clientlist.remove(&client);

  if (m_client == &client) {
    if (m_clientlist.empty() )
      m_client = 0;
    else // this really shouldn't happen
      m_client = m_clientlist.back();
  }

  tk::EventManager &evm = *tk::EventManager::instance();
  evm.remove(client.window() );

  IconButton *label_btn = m_labelbuttons[&client];
  if (label_btn != 0) {
    frame().removeTab(label_btn);
    label_btn = 0;
  }

  m_labelbuttons.erase(&client);
  updateClientLeftWindow();

  sbdbg<<"("<<__FUNCTION__<<")["<<this<<"] numClients = "<<numClients()<<"\n";

  return true;
} // removeClient

WinClient *ShyneboxWindow::findClient(Window win) {
  for (auto it : clientList() )
    if (it->window() == win)
      return it;
  return 0;
}

// raise and focus next client
void ShyneboxWindow::nextClient() {
  if (numClients() <= 1)
    return;

  ClientList::iterator it = find(m_clientlist.begin(), m_clientlist.end(),
                                 m_client);
  if (it == m_clientlist.end() )
    return;

  ++it;
  if (it == m_clientlist.end() )
    it = m_clientlist.begin();

  setCurrentClient(**it, isFocused() );
}

void ShyneboxWindow::prevClient() {
  if (numClients() <= 1)
    return;

  ClientList::iterator it = find(m_clientlist.begin(), m_clientlist.end(),
                                 m_client);

  if (it == m_clientlist.end() )
    return;

  if (it == m_clientlist.begin() )
    it = m_clientlist.end();
  --it;

  setCurrentClient(**it, isFocused() );
}

void ShyneboxWindow::moveClientLeft() {
  if (m_clientlist.size() == 1
      || *m_clientlist.begin() == &winClient() )
    return;

  // move client in clientlist to the left
  ClientList::iterator oldpos = find(m_clientlist.begin(), m_clientlist.end(), &winClient() );
  ClientList::iterator newpos = oldpos; newpos--;
  swap(*newpos, *oldpos);
  frame().moveLabelButtonLeft(*m_labelbuttons[&winClient()] );

  updateClientLeftWindow();
}

void ShyneboxWindow::moveClientRight() {
  if (m_clientlist.size() == 1
      || *m_clientlist.rbegin() == &winClient() )
    return;

  ClientList::iterator oldpos = find(m_clientlist.begin(), m_clientlist.end(), &winClient() );
  ClientList::iterator newpos = oldpos; newpos++;
  swap(*newpos, *oldpos);
  frame().moveLabelButtonRight(*m_labelbuttons[&winClient()]);

  updateClientLeftWindow();
}

ShyneboxWindow::ClientList::iterator ShyneboxWindow::getClientInsertPosition(int x, int y) {
  int dest_x = 0, dest_y = 0;
  Window labelbutton = 0;
  if (!XTranslateCoordinates(display,
                             parent().window(), frame().tabcontainer().window(),
                             x, y, &dest_x, &dest_y,
                             &labelbutton) )
    return m_clientlist.end();

  WinClient* c = winClientOfLabelButtonWindow(labelbutton);

  // label button not found
  if (!c)
    return m_clientlist.end();

  Window child_return=0;
  // make x and y relative to our labelbutton
  if (!XTranslateCoordinates(display,
                             frame().tabcontainer().window(), labelbutton,
                             dest_x, dest_y, &x, &y,
                             &child_return) )
    return m_clientlist.end();

  ClientList::iterator client = find(m_clientlist.begin(),
                                     m_clientlist.end(),
                                     c);

  if (x > static_cast<signed>(m_labelbuttons[c]->width() ) / 2)
    client++;

  return client;
}

// only called from attachTo
void ShyneboxWindow::moveClientTo(WinClient &win, int x, int y) {
  int dest_x = 0, dest_y = 0;
  Window labelbutton = 0;
  if (!XTranslateCoordinates(display,
                             parent().window(), frame().tabcontainer().window(),
                             x, y, &dest_x, &dest_y,
                             &labelbutton) )
    return;

  WinClient* client = winClientOfLabelButtonWindow(labelbutton);

  if (!client)
    return;

  Window child_return = 0;
  //make x and y relative to our labelbutton
  if (!XTranslateCoordinates(display,
                             frame().tabcontainer().window(), labelbutton,
                             dest_x, dest_y, &x, &y,
                             &child_return) )
    return;
  if (x > static_cast<signed>(m_labelbuttons[client]->width() ) / 2)
    moveClientRightOf(win, *client);
  else
    moveClientLeftOf(win, *client);
}

void ShyneboxWindow::moveClientLeftOf(WinClient &win, WinClient &dest) {
  frame().moveLabelButtonLeftOf(*m_labelbuttons[&win], *m_labelbuttons[&dest]);

  ClientList::iterator it = find(m_clientlist.begin(),
                                 m_clientlist.end(),
                                 &win);
  ClientList::iterator new_pos = find(m_clientlist.begin(),
                                      m_clientlist.end(),
                                      &dest);

  // make sure we found them
  if (it == m_clientlist.end() || new_pos==m_clientlist.end() )
    return;
  // moving a button to the left of itself results in no change
  if (new_pos == it)
    return;
  //remove from list
  m_clientlist.erase(it);
  //insert on the new place
  m_clientlist.insert(new_pos, &win);

  updateClientLeftWindow();
}


void ShyneboxWindow::moveClientRightOf(WinClient &win, WinClient &dest) {
  frame().moveLabelButtonRightOf(*m_labelbuttons[&win], *m_labelbuttons[&dest]);

  ClientList::iterator it = find(m_clientlist.begin(),
                                 m_clientlist.end(),
                                 &win);
  ClientList::iterator new_pos = find(m_clientlist.begin(),
                                      m_clientlist.end(),
                                      &dest);

  // make sure we found them
  if (it == m_clientlist.end() || new_pos==m_clientlist.end() )
    return;

  //moving a button to the right of itself results in no change
  if (new_pos == it)
    return;

  //remove from list
  m_clientlist.erase(it);
  //need to insert into the next position
  new_pos++;
  //insert on the new place
  if (new_pos == m_clientlist.end() )
    m_clientlist.push_back(&win);
  else
    m_clientlist.insert(new_pos, &win);

  updateClientLeftWindow();
}

// Update LEFT window atom on all clients.
void ShyneboxWindow::updateClientLeftWindow() {
  if (clientList().empty() )
    return;

  // It should just update the affected clients but that
  // would require more complex code and we're assuming
  // the user dont have alot of windows grouped so this
  // wouldn't be too time consuming and it's easier to
  // implement.
  ClientList::iterator it = clientList().begin();
  ClientList::iterator it_end = clientList().end();
  // set no left window on first tab
  (*it)->setGroupLeftWindow(0);
  WinClient *last_client = *it;
  ++it;
  for (; it != it_end; ++it) {
    (*it)->setGroupLeftWindow(last_client->window() );
    last_client = *it;
  }
}

bool ShyneboxWindow::setCurrentClient(WinClient &client, bool setinput) {
  // make sure it's in our list
  if (client.sbwindow() != this)
    return false;

  IconButton *button = m_labelbuttons[&client];
  // in case the window is being destroyed, but this should never happen
  if (!button)
    return false;

  if (!client.acceptsFocus() )
    setinput = false; // don't try

  WinClient *old = m_client;
  m_client = &client;

  bool ret = setinput && focus();
  if (setinput && old->acceptsFocus() ) {
    m_client = old;
    return ret;
  }

  m_client->raise();

  sbdbg<<"ShyneboxWindow::"<<__FUNCTION__<<": labelbutton[client] = "<<
         button<<"\n";

  if (old != &client) {
    frame().setFocusTitle(title() );
    frame().setShapingClient(&client, false);
    applyDecorations();
  }
  return ret;
} // setCurrentClient

bool ShyneboxWindow::isGroupable() const {
  if (isResizable() && isMaximizable() && !winClient().isTransient() )
    return true;
  return false;
}

bool ShyneboxWindow::isFocusNew() const {
  if (m_focus_protection & Focus::Gain)
    return true;
  if (m_focus_protection & Focus::Refuse)
    return false;
  return screen().focusControl().focusNew();
}

void ShyneboxWindow::associateClientWindow() {
  frame().setShapingClient(m_client, false);

  frame().moveResizeForClient(m_client->x(), m_client->y(),
                              m_client->width(), m_client->height(),
                              m_client->gravity(), m_client->old_bw);

  updateSizeHints();
  frame().setClientWindow(*m_client);
}

void ShyneboxWindow::updateSizeHints() {
  m_size_hint = m_client->sizeHints();

  for (auto it : clientList() ) {
    if (it == m_client)
      continue;

    const SizeHints &hint = it->sizeHints();
    if (m_size_hint.min_width < hint.min_width)
      m_size_hint.min_width = hint.min_width;
    if (m_size_hint.max_width > hint.max_width)
      m_size_hint.max_width = hint.max_width;
    if (m_size_hint.min_height < hint.min_height)
      m_size_hint.min_height = hint.min_height;
    if (m_size_hint.max_height > hint.max_height)
      m_size_hint.max_height = hint.max_height;
    // lcm could end up a bit silly, and the situation is bad no matter what
    if (m_size_hint.width_inc < hint.width_inc)
      m_size_hint.width_inc = hint.width_inc;
    if (m_size_hint.height_inc < hint.height_inc)
      m_size_hint.height_inc = hint.height_inc;
    if (m_size_hint.base_width < hint.base_width)
      m_size_hint.base_width = hint.base_width;
    if (m_size_hint.base_height < hint.base_height)
      m_size_hint.base_height = hint.base_height;
    if (m_size_hint.min_aspect_x * hint.min_aspect_y >
        m_size_hint.min_aspect_y * hint.min_aspect_x) {
      m_size_hint.min_aspect_x = hint.min_aspect_x;
      m_size_hint.min_aspect_y = hint.min_aspect_y;
    }
    if (m_size_hint.max_aspect_x * hint.max_aspect_y >
        m_size_hint.max_aspect_y * hint.max_aspect_x) {
      m_size_hint.max_aspect_x = hint.max_aspect_x;
      m_size_hint.max_aspect_y = hint.max_aspect_y;
    }
  }
  frame().setSizeHints(m_size_hint);
} // updateSizeHints

// needed for click to focus
// (could be moved to Keys? called there)
// important to grab on the CLIENT and NOT THE FRAME
// otherwise you will get fake-pseudo events
// https://tronche.com/gui/x/xlib/events/input-focus/grab.html
// which fuck up firefox and chromium for misc widgets in each
void ShyneboxWindow::grabButtons() {

//  XGrabButton(display, Button1, AnyModifier,
//              frame().window().window(), True, ButtonPressMask,
//              GrabModeSync, GrabModeSync, None, None);
//  XUngrabButton(display, Button1, Mod1Mask|Mod2Mask|Mod3Mask,
//                frame().window().window() );

  // similar to KeyUtil, I don't think mask matters
  XGrabButton(display, Button1, 0,
              m_client->window(), False, ButtonPressMask,
              GrabModeSync, GrabModeSync, None, None);
}

void ShyneboxWindow::reconfigure() {
  updateButtons();
  applyDecorations(); // frame().applyDeco calls frame reconfigure()
  setFocusFlag(m_focused); // can set m_raise_timer on/off
  m_raise_timer.setTimeout(Shynebox::instance()->getAutoRaiseDelay() * tk::SbTime::IN_MILLISECONDS);
  m_tab_activate_timer.setTimeout(Shynebox::instance()->getAutoRaiseDelay() * tk::SbTime::IN_MILLISECONDS);

  for (auto it : m_labelbuttons)
    it.second->setPixmap(screen().getTabsUsePixmap() );
}

void ShyneboxWindow::updateMWMHintsFromClient(WinClient &client) {
  const WinClient::MwmHints *hint = client.getMwmHint();

  if (hint && !m_toggled_decos && hint->flags & MwmHintsDecorations) {
    if (hint->decorations & MwmDecorAll) {
      decorations.titlebar = decorations.handle = decorations.border =
                          decorations.iconify = decorations.maximize =
                          decorations.menu = true;
    } else {
      decorations.titlebar = decorations.handle = decorations.border =
                          decorations.iconify = decorations.maximize =
                          decorations.tab = false;
      decorations.menu = true;
      if (hint->decorations & MwmDecorBorder)
        decorations.border = true;
      if (hint->decorations & MwmDecorHandle)
        decorations.handle = true;
      if (hint->decorations & MwmDecorTitle) {
        decorations.titlebar = decorations.tab = true;
        //only tab on windows with titlebar
      }
      if (hint->decorations & MwmDecorMenu)
        decorations.menu = true;
      if (hint->decorations & MwmDecorIconify)
        decorations.iconify = true;
      if (hint->decorations & MwmDecorMaximize)
        decorations.maximize = true;
    }
  } else {
    decorations.titlebar = decorations.handle = decorations.border =
    decorations.iconify = decorations.maximize = decorations.menu = true;
  }

  unsigned int mask = decorationMask();
  mask &= WindowState::getDecoMaskFromString(screen().defaultDeco() );
  setDecorationMask(mask, false);

  // functions.tabable is ours, not special one
  // note that it means this window is "tabbable"
  if (hint && hint->flags & MwmHintsFunctions) {
    if (hint->functions & MwmFuncAll) {
      functions.resize = functions.move = functions.iconify =
                 functions.maximize = functions.close = true;
    } else {
      functions.resize = functions.move = functions.iconify =
                functions.maximize = functions.close = false;

      if (hint->functions & MwmFuncResize)
        functions.resize = true;
      if (hint->functions & MwmFuncMove)
        functions.move = true;
      if (hint->functions & MwmFuncIconify)
        functions.iconify = true;
      if (hint->functions & MwmFuncMaximize)
        functions.maximize = true;
      if (hint->functions & MwmFuncClose)
        functions.close = true;
    }
  } else {
    functions.resize = functions.move = functions.iconify =
    functions.maximize = functions.close = true;
  } // if hint flags MwmHintFunctions
} // updateMWMHintsFromClient

void ShyneboxWindow::updateFunctions() {
  if (!m_client)
    return;
  bool changed = false;
  if (m_client->isClosable() != functions.close) {
    functions.close = m_client->isClosable();
    changed = true;
  }

  if (changed)
    updateButtons();
}

void ShyneboxWindow::move(int x, int y) {
  moveResize(x, y, frame().width(), frame().height() );
}

void ShyneboxWindow::resize(unsigned int width, unsigned int height) {
  // don't let moveResize set window as placed
  // since we're only resizing
  bool placed = m_placed;
  moveResize(frame().x(), frame().y(), width, height);
  m_placed = placed;
}

// send_event is just an override
void ShyneboxWindow::moveResize(int new_x, int new_y,
                               unsigned int new_width, unsigned int new_height,
                               bool send_event) {
  m_placed = true;
  send_event = send_event || frame().x() != new_x || frame().y() != new_y;

  if ((new_width != frame().width() || new_height != frame().height() )
      && isResizable() && !isShaded() ) {
    // because you're changing size here, fitToScreen() doesn't work
    const int bw = frame().window().borderWidth() * 2;

    if ((signed)new_width + new_x + bw < 0)
      new_x = 0;
    else if (new_x > (signed)screen().width() )
      new_x = screen().width() - new_width - bw;

    if ((signed)new_height + new_y + bw < 0)
      new_y = 0;
    else if (new_y > (signed)screen().height() )
      new_y = (signed)screen().height() - new_height - bw;

    frame().moveResize(new_x, new_y, new_width, new_height);
    setFocusFlag(m_focused);

    send_event = true;
  } else if (send_event)
    frame().move(new_x, new_y);

  if (send_event && !moving)
    sendConfigureNotify();

  if (!moving) {
    m_last_resize_x = new_x;
    m_last_resize_y = new_y;

    /* Ignore all EnterNotify events until the pointer actually moves */
    screen().focusControl().ignoreAtPointer();
  }
} // moveResize

void ShyneboxWindow::moveResizeForClient(int new_x, int new_y,
                               unsigned int new_width, unsigned int new_height,
                               int gravity, unsigned int client_bw) {
  m_placed = true;
  frame().moveResizeForClient(new_x, new_y, new_width, new_height, gravity, client_bw);
  setFocusFlag(m_focused);
  m_state.shaded = false;

  if (!moving) {
    m_last_resize_x = new_x;
    m_last_resize_y = new_y;
  }
}

void ShyneboxWindow::getMaxSize(unsigned int* width, unsigned int* height) const {
  if (width)
    *width = m_size_hint.max_width;
  if (height)
    *height = m_size_hint.max_height;
}

// returns whether the focus was "set" to this window
// it doesn't guarantee that it has focus, but says that we have
// tried. A FocusIn event should eventually arrive for that
// window if it actually got the focus, then setFocusFlag is called,
// which updates all the graphics etc
bool ShyneboxWindow::focus() {
  if (!moving && isFocused() && m_client == FocusControl::focusedWindow() )
    return false;

  fitToScreen();

  if (!m_client->validateClient() )
    return false;

  if (screen().currentWorkspaceID() != workspaceNumber() && !isStuck() )
    screen().changeWorkspaceID(workspaceNumber(), false);

  if (isIconic() ) {
    deiconify();
    m_focused = true; // signal to mapNotifyEvent to set focus when mapped
    return true; // the window probably will get focused, just not yet
  }

  // this needs to be here rather than setFocusFlag because
  // FocusControl::revertFocus will return before FocusIn events arrive
  m_screen.focusControl().setScreenFocusedWindow(*m_client);

  sbdbg<<"ShyneboxWindow::"<<__FUNCTION__<<" isModal() = "<<m_client->isModal()<<"\n";
  sbdbg<<"ShyneboxWindow::"<<__FUNCTION__<<" transient size = "<<m_client->transients.size()<<"\n";

  if (!m_client->transients.empty() && m_client->isModal() ) {
    sbdbg<<__FUNCTION__<<": isModal and have transients client = "<<
                                 hex<<m_client->window()<<dec<<"\n";
    sbdbg<<__FUNCTION__<<": this = "<<this<<"\n";

    for (auto it : m_client->transients) {
      sbdbg<<__FUNCTION__<<": transient 0x"<< it << "\n";
      if (it->isStateModal() )
        return it->focus();
    }
    return false; // client says it's modal but no transients are
  }

  return m_client->sendFocus();
} // focus

// don't hide the frame directly, use this function
void ShyneboxWindow::hide(bool interrupt_moving) {
 sbdbg<<"("<<__FUNCTION__<<")["<<this<<"]"<<"\n";

  // resizing always stops on hides
  if (resizing)
    stopResizing(true);

  if (interrupt_moving) {
    if (moving)
      stopMoving(true);
    if (m_attaching_tab)
      attachTo(0, 0, true);
  }

  setState(IconicState, false);

  menu().hide();
  frame().hide();

  if (FocusControl::focusedSbWindow() == this)
    FocusControl::setFocusedWindow(0);
}

void ShyneboxWindow::show() {
  frame().show();
  setState(NormalState, false);
}

void ShyneboxWindow::toggleIconic() {
  if (isIconic() )
    deiconify();
  else
    iconify();
}

void ShyneboxWindow::iconify() {
  setInView(false);
  if (isIconic() )
    return;

  m_state.iconic = true;
  // remove from workspace list
  Shynebox::instance()->windowStateChanged(*this);

  hide(true);

  screen().focusControl().setFocusBack(*this);

  for (auto client_it : m_clientlist)
    for (auto it : client_it->transientList() )
      if (it->sbwindow() )
        it->sbwindow()->iconify();
  // focus revert is done elsewhere
} // iconify (minimize)

void ShyneboxWindow::deiconify(bool do_raise) {
  //setInView(true);
  // while testing NextWindow (Viewable=no) things sent
  // and set xvisibility on deiconify by their own, so
  // this isn't needed. (but they don't UNSET, see iconify() )
  if (numClients() == 0 || !m_state.iconic || oplock)
    return;

  oplock = true;

  // reassociate first, so it gets removed from screen's icon list
  screen().reassociateWindow(this, m_workspace_number, false);
  m_state.iconic = false;
  Shynebox::instance()->windowStateChanged(*this);

  // deiconify all transients
  for (auto client_it : m_clientlist)
    for (auto it : client_it->transientList() )
      if (it->sbwindow() )
        it->sbwindow()->deiconify(false);

  if (m_workspace_number != screen().currentWorkspaceID() ) {
    oplock = false;
    return;
  }

  show();

  // focus new, OR if it's the only window on the workspace
  // but not on startup: focus will be handled after creating everything
  // we use m_focused as a signal to focus the window when mapped
  if (screen().currentWorkspace()->windowList().size() == 1
      || isFocusNew() || m_client->isTransient() )
    m_focused = true;

  oplock = false;

  if (do_raise)
    raise();
} // deiconify (un-minimize)

// maximize as big as the screen is, dont care about slit / toolbar
// raise to toplayer
void ShyneboxWindow::setFullscreen(bool flag) {
  if (!m_initialized) {
    // this will interfere with window placement, so we delay it
    m_state.fullscreen = flag;
    return;
  }

  if (flag && !isFullscreen() ) {
    m_old_layernum = layerNum();
    m_state.fullscreen = true;
    setFullscreenLayer();
  } else if (!flag && isFullscreen() ) {
    m_state.fullscreen = false;
    moveToLayer(m_old_layernum);
    Shynebox::instance()->windowStateChanged(*this);
  }

  // it's likely that fullscreen changes went through, so update graphics
  frame().applyState();
  frameExtentChanged();
}

void ShyneboxWindow::setFullscreenLayer() {
  ShyneboxWindow *foc = FocusControl::focusedSbWindow();
  // if another window on the same head is focused, make sure we can see it
  if (isFocused() || !foc || &foc->screen() != &screen()
      || getOnHead() != foc->getOnHead()
      || (foc->winClient().isTransient()
        && foc->winClient().transientFor()->sbwindow() == this) ) {
    moveToLayer(RLEnum::ABOVE_DOCK);
  } else {
    moveToLayer(m_old_layernum);
    foc->raise();
  }
  Shynebox::instance()->windowStateChanged(*this);
}

// maximize wrapper
void ShyneboxWindow::maximize(int type) {
  int new_max = m_state.queryToggleMaximized(type);
  setMaximizedState(new_max);
}

// actual maximize
void ShyneboxWindow::setMaximizedState(int type) {
  if (!m_initialized || type == m_state.maximized) {
    // this will interfere with window placement, so we delay it
    m_state.maximized = type;
    return;
  }

  if (resizing)
    stopResizing();

  // maximize will unshade windows
  if (isShaded() )
    m_state.shaded = false;

  m_state.maximized = type;
  frame().applyState();
  frameExtentChanged();

  Shynebox::instance()->windowStateChanged(*this);
}

void ShyneboxWindow::disableMaximization() {
  m_state.maximized = WindowState::MAX_NONE;
  m_state.saveGeometry(frame().x(), frame().y(),
                       frame().width(), frame().height() );
  frame().applyState();
  frameExtentChanged();
  Shynebox::instance()->windowStateChanged(*this);
}


void ShyneboxWindow::maximizeHorizontal() {
  maximize(WindowState::MAX_HORZ);
}

void ShyneboxWindow::maximizeVertical() {
  maximize(WindowState::MAX_VERT);
}

void ShyneboxWindow::maximizeFull() {
  maximize(WindowState::MAX_FULL);
}

void ShyneboxWindow::setWorkspace(int n) {
  unsigned int old_wkspc = m_workspace_number;
  m_workspace_number = n;

  // notify workspace change
  if (m_initialized && old_wkspc != m_workspace_number)
    Shynebox::instance()->windowWorkspaceChanged(*this);
}

void ShyneboxWindow::setLayerNum(int layernum) {
  m_state.layernum = layernum;

  if (m_initialized)
    Shynebox::instance()->windowLayerChanged(*this);
}

void ShyneboxWindow::shade() {
  // we can only shade if we have a titlebar
  if (!decorations.titlebar)
    return;

  m_state.shaded = !m_state.shaded;

  if (m_initialized) {
    frame().applyState();
    frameExtentChanged();
    Shynebox::instance()->windowStateChanged(*this);
  }
}

void ShyneboxWindow::shadeOn() {
  if (!m_state.shaded)
    shade();
}

void ShyneboxWindow::shadeOff() {
  if (m_state.shaded)
    shade();
}

void ShyneboxWindow::setShaded(bool val) {
  if (val != m_state.shaded)
    shade();
}

void ShyneboxWindow::stick() {
  m_state.stuck = !m_state.stuck;

  if (m_initialized) {
    // notify since some things consider "stuck" to be a pseudo-workspace
    Shynebox::instance()->windowStateChanged(*this);
    Shynebox::instance()->windowWorkspaceChanged(*this);
  }

  for (auto client_it : clientList() )
    for (auto it : client_it->transientList() )
      if (it->sbwindow() )
        it->sbwindow()->setStuck(m_state.stuck);
}

void ShyneboxWindow::setStuck(bool val) {
  if (val != m_state.stuck)
    stick();
}

void ShyneboxWindow::setIconic(bool val) {
  if (!val && isIconic() )
    deiconify();
  if (val && !isIconic() )
    iconify();
}

void ShyneboxWindow::setInView(bool in_v) {
  Focusable::setInView(in_v);
  m_client->setInView(in_v);
}

// since both of these are focusables and main event
// loop searches/sets potentially client and ~this~
// (client probably enough)
bool ShyneboxWindow::isInView() const {
  return m_client->isInView() || Focusable::isInView();
}

void ShyneboxWindow::raise() {
  if (isIconic() )
    return;

  sbdbg<<"ShyneboxWindow("<<title().logical()<<")::raise()[layer="<<layerNum()<<"]"<<"\n";

  // get root window
  WinClient *client = getRootTransientFor(m_client);

  // if we have transient_for then we should put ourself last in
  // transients list so we get raised last and thus gets above the other transients
  if (m_client->transientFor()
      && m_client != m_client->transientFor()->transientList().back() ) {
    m_client->transientFor()->transientList().remove(m_client);
    m_client->transientFor()->transientList().push_back(m_client);
  }
  // raise this window and every transient in it with this one last
  if (client->sbwindow() ) {
    // doing this on startup messes up the focus order
    if (!Shynebox::instance()->isStartup() && client->sbwindow() != this
         && &client->sbwindow()->winClient() != client)
      client->sbwindow()->setCurrentClient(*client, false);
      // activate the client so the transient won't get pushed back down
    raiseShyneboxWindow(*client->sbwindow() );
  }
} // raise

void ShyneboxWindow::lower() {
  if (isIconic() )
    return;

  sbdbg<<"ShyneboxWindow("<<title().logical()<<")::lower()\n";

  /* Ignore all EnterNotify events until the pointer actually moves */
  screen().focusControl().ignoreAtPointer();

  // get root window
  WinClient *client = getRootTransientFor(m_client);

  if (client->sbwindow() )
    lowerShyneboxWindow(*client->sbwindow() );
} // lower

void ShyneboxWindow::tempRaise() {
  // Note: currently, this causes a problem with cycling through minimized
  // clients if this window has more than one tab, since the window will not
  // match isIconic() when the rest of the tabs get checked
  if (isIconic() )
    deiconify();

  // the root transient will get raised when we stop cycling
  // raising it here causes problems when it isn't the active tab
  tempRaiseShyneboxWindow(*this);
}

void ShyneboxWindow::moveToLayer(int layernum, bool force) {
  sbdbg<<"ShyneboxWindow("<<title().logical()<<")::moveToLayer("<<layernum<<")\n";

  // menu layer is for menu only
  if (layernum <= RLEnum::MENU)
      layernum = 1;
  else if (layernum >= RLEnum::NUM_LAYERS)
      layernum = RLEnum::DESKTOP;

  if (!m_initialized)
      m_state.layernum = layernum;

  if ((m_state.layernum == layernum && !force) || !m_client)
      return;

  // get root window
  WinClient *client = getRootTransientFor(m_client);

  ShyneboxWindow *win = client->sbwindow();
  if (!win) return;

  win->layerItem().moveToLayer(layernum);
  // remember number just in case a transient happens to revisit this window
  layernum = win->layerItem().getLayerNum();
  win->setLayerNum(layernum);

  // move all the transients, too
  for (auto client_it : m_clientlist) {
    for (auto it : client_it->transientList() ) {
      ShyneboxWindow *sbwin = it->sbwindow();
      if (sbwin && !sbwin->isIconic() ) {
        sbwin->layerItem().moveToLayer(layernum);
        sbwin->setLayerNum(layernum);
      }
    }
  } // for transients
} // moveToLayer

void ShyneboxWindow::setFocusHidden(bool value) {
  m_state.focus_hidden = value;
  if (m_initialized)
    Shynebox::instance()->windowStateChanged(*this);
}

void ShyneboxWindow::setIconHidden(bool value) {
  m_state.icon_hidden = value;
  if (m_initialized)
    Shynebox::instance()->windowStateChanged(*this);
}

// window has actually RECEIVED focus (got a FocusIn event)
// so now we make it a focused frame etc
void ShyneboxWindow::setFocusFlag(bool focus) {
  if (!m_client)
    return;

  bool was_focused = m_focused;
  m_focused = focus;

  sbdbg<<"ShyneboxWindow("<<title().logical()<<")::setFocusFlag("<<focus<<")\n";

  installColormap(focus);

  // if we're fullscreen and another window gains focus, change layers
  if (m_state.fullscreen)
    setFullscreenLayer(); // handle ABOVE_DOCK layer changes

  if (focus != frame().focused() )
    frame().setFocus(focus);

  if (focus && screen().focusControl().isCycling() )
    tempRaise();
  else if (screen().doAutoRaise() ) {
    if (m_focused)
      m_raise_timer.start();
    else
      m_raise_timer.stop();
  }

  if (was_focused != focus)
    Shynebox::instance()->keys()->doAction(focus ? FocusIn : FocusOut, 0, 0,
                                                Keys::ON_WINDOW, m_client);
} // setFocusFlag

void ShyneboxWindow::installColormap(bool install) {
  if (m_client == 0)
    return;

  Shynebox *shynebox = Shynebox::instance();
  shynebox->grab();
  if (! m_client->validateClient() )
      return;

  int i = 0, ncmap = 0;
  Colormap *cmaps = XListInstalledColormaps(display, m_client->window(), &ncmap);
  XWindowAttributes wattrib;
  if (cmaps) {
    if (m_client->getAttrib(wattrib) ) {
      if (install) {
        // install the window's colormap
        for (i = 0; i < ncmap; i++) {
          if (*(cmaps + i) == wattrib.colormap) {
            // this window is using an installed color map... do not install
            install = false;
            break; //end for-loop (we dont need to check more)
          }
        }
        // otherwise, install the window's colormap
        if (install)
          XInstallColormap(display, wattrib.colormap);
      } else {
        for (i = 0; i < ncmap; i++) { // uninstall the window's colormap
          if (*(cmaps + i) == wattrib.colormap)
            XUninstallColormap(display, wattrib.colormap);
        }
      }
    } // if wattrib
    XFree(cmaps);
  } // if cmaps

  shynebox->ungrab();
} // installColormap

/**
 Sets state on each client in our list
 Use setting_up for setting startup state - it may not be committed yet
 That'll happen when its mapped
 */
void ShyneboxWindow::setState(unsigned long new_state, bool setting_up) {
  m_current_state = new_state;
  if (numClients() == 0 || setting_up)
    return;

  unsigned long state[2];
  state[0] = (unsigned long) m_current_state;
  state[1] = (unsigned long) None;

  for (auto &it : m_clientlist)
    XChangeProperty(display, it->window(),
             SbAtoms::instance()->getWMStateAtom(),
             SbAtoms::instance()->getWMStateAtom(),
             32, PropModeReplace,
             (unsigned char*) state, 2);

  for (auto it : m_clientlist) {
    it->setEventMask(NoEventMask);
    if (new_state == IconicState)
      it->hide();
    else if (new_state == NormalState)
      it->show();
    it->setEventMask(PropertyChangeMask | StructureNotifyMask | FocusChangeMask | KeyPressMask);
  }
} // setState

bool ShyneboxWindow::getState() {
  Atom atom_return;
  bool ret = false;
  int foo;
  unsigned long *state, ulfoo, nitems;
  if (!m_client->property(SbAtoms::instance()->getWMStateAtom(),
                          0l, 2l, false, SbAtoms::instance()->getWMStateAtom(),
                          &atom_return, &foo, &nitems, &ulfoo,
                          (unsigned char **) &state) || !state)
    return false;

  if (nitems >= 1) {
    m_current_state = static_cast<unsigned long>(state[0]);
    ret = true;
  }

  XFree(static_cast<void *>(state) );

  return ret;
}

// Show the window menu at pos x, y
void ShyneboxWindow::showMenu(int x, int y) {
  menu().reloadHelper()->checkReload();
  SbMenu::setWindow(this);
  screen().placementStrategy()
      .placeAndShowMenu(menu(), x, y);
}

void ShyneboxWindow::popupMenu(int x, int y) {
  // hide menu if it was opened for this window before
  if (menu().isVisible() && SbMenu::window() == this) {
   menu().hide();
   return;
  }

  menu().disableTitle();
  showMenu(x, y);
}

// Moves the menu to last button press position and shows it,
void ShyneboxWindow::popupMenu() {
  if (m_last_button_x < x()
      || m_last_button_x > x() + static_cast<signed>(width() ) )
    m_last_button_x = x();

  popupMenu(m_last_button_x, frame().titlebarHeight() + frame().y() );
}

void ShyneboxWindow::handleEvent(XEvent &event) {
  switch (event.type) {
  case ConfigureRequest:
     sbdbg<<"ConfigureRequest("<<title().logical()<<")\n";

      configureRequestEvent(event.xconfigurerequest);
      break;
  case MapNotify:
      mapNotifyEvent(event.xmap);
      break;
  case PropertyNotify: {
#ifdef DEBUG
      char *atomname = XGetAtomName(display, event.xproperty.atom);
      sbdbg<<"PropertyNotify("<<title().logical()<<"), property = "<<
            atomname << " - time - " << event.xproperty.time << "\n";
      if (atomname)
        XFree(atomname);
#endif
      WinClient *client = findClient(event.xproperty.window);
      if (client)
        propertyNotifyEvent(*client, event.xproperty.atom);

      }
      break;
  default:
#ifdef SHAPE
      if (Shynebox::instance()->haveShape()
          && event.type == Shynebox::instance()->shapeEventbase() + ShapeNotify) {
      sbdbg<<"ShapeNotify("<<title().logical()<<")\n";

      XShapeEvent *shape_event = (XShapeEvent *)&event;

      if (shape_event->shaped)
          frame().setShapingClient(m_client, true);
      else
          frame().setShapingClient(0, true);

      tk::App::instance()->sync(false);
      break;
      }
#endif
      break;
  }
} // handleEvent

// NOTE: this is called from shynebox handleEvent which happens
//       after other events EventManager dishes out
void ShyneboxWindow::mapRequestEvent(XMapRequestEvent &re) {
  // we're only concerned about client window event
  WinClient *client = findClient(re.window);
  if (client == 0) {
    sbdbg<<"("<<__FUNCTION__<<"): Can't find client!\n";
    return;
  }

  // Note: this function never gets called from WithdrawnState
  // initial state is handled in init()

  setCurrentClient(*client, false); // focus handled on MapNotify
  deiconify();

  if (isFocusNew() ) {
    m_focused = false; // deiconify sets this
    Focus::Protection fp = m_focus_protection;
    m_focus_protection &= ~Focus::Deny; // goes by "Refuse"
    m_focused = focusRequestFromClient(*client);
    m_focus_protection = fp;
    if (!m_focused)
      lower();
  }
}

bool ShyneboxWindow::focusRequestFromClient(WinClient &from) {
  if (from.sbwindow() != this)
    return false;

  bool ret = true;

  ShyneboxWindow *cur = FocusControl::focusedSbWindow();
  WinClient *client = FocusControl::focusedWindow();
  if ((from.sbwindow() && (from.sbwindow()->focusProtection() & Focus::Deny) )
       || (cur && (cur->focusProtection() & Focus::Lock) ) )
    ret = false;
  else if (cur && getRootTransientFor(&from) != getRootTransientFor(client) )
    ret = !cur->isFullscreen() && !cur->isTyping()
          && (!screen().focusControl().focusSameHead()
            || (getOnHead() == cur->getOnHead() ) );

  return ret;
}

void ShyneboxWindow::mapNotifyEvent(XMapEvent &ne) {
  WinClient *client = findClient(ne.window);
  if (!client || client != m_client
      || ne.override_redirect || !isVisible()
      || !client->validateClient() )
    return;

  m_state.iconic = false;

  // setting state will cause all tabs to be mapped, but we only want the
  // original tab to be focused
  if (m_current_state != NormalState)
    setState(NormalState, false);

  // we use m_focused as a signal that this should be focused when mapped
  if (m_focused) {
    m_focused = false;
    focus();
  }
}

// Unmaps frame window and client window if event.window == m_client->window
void ShyneboxWindow::unmapNotifyEvent(XUnmapEvent &ue) {
  WinClient *client = findClient(ue.window);
  if (client == 0)
    return;

  sbdbg<<"("<<__FUNCTION__<<"): 0x"<<hex<<client->window()<<dec<<"\n";
  sbdbg<<"("<<__FUNCTION__<<"): title="<<client->title().logical()<<"\n";

  if (numClients() == 1) // unmapping the last client
    frame().hide(); // hide this now, otherwise compositors will fade out the frame, bug #1110
  restore(client, false);
}

/**
   Checks if event is for m_client->window.
   If it isn't, we leave it until the window is unmapped, if it is,
   we just hide it for now.
*/
void ShyneboxWindow::destroyNotifyEvent(XDestroyWindowEvent &de) {
  if (de.window == m_client->window() ) {
    sbdbg<<"DestroyNotifyEvent this="<<this<<" title = "<<title().logical()<<"\n";
    delete m_client;
    if (numClients() == 0)
      delete this;
  }
}


void ShyneboxWindow::propertyNotifyEvent(WinClient &client, Atom atom) {
  switch(atom) {
  case XA_WM_CLASS:
  case XA_WM_CLIENT_MACHINE:
  case XA_WM_COMMAND:
      break;
  case XA_WM_TRANSIENT_FOR: {
      bool was_transient = client.isTransient();
      client.updateTransientInfo();
      // update our layer to be the same layer as our transient for
      if (client.isTransient() && !was_transient
          && client.transientFor()->sbwindow() )
        layerItem().setLayer(client.transientFor()->sbwindow()->layerItem().getLayer() );
  } break;
  case XA_WM_HINTS:
      client.updateWMHints();
      break;
  case XA_WM_ICON_NAME:
      // we don't use icon title, since many apps don't update it,
      // and we don't show icons anyway
      break;
  case XA_WM_NAME:
      client.updateTitle();
      break;
  case XA_WM_NORMAL_HINTS: {
      sbdbg<<"XA_WM_NORMAL_HINTS("<<title().logical()<<")\n";
      unsigned int old_max_width = client.maxWidth();
      unsigned int old_min_width = client.minWidth();
      unsigned int old_min_height = client.minHeight();
      unsigned int old_max_height = client.maxHeight();
      bool changed = false;
      client.updateWMNormalHints();
      updateSizeHints();

      if (client.minWidth() != old_min_width
          || client.maxWidth() != old_max_width
          || client.minHeight() != old_min_height
          || client.maxHeight() != old_max_height) {
        if (!client.sizeHints().isResizable() ) {
          if (functions.resize || functions.maximize)
            changed = true;
          functions.resize = functions.maximize = false;
        } else {
          if (!functions.maximize || !functions.resize)
            changed = true;
          functions.maximize = functions.resize = true;
        } // if resizable

        if (changed) {
          updateButtons();
          applyDecorations();
        }
     } // if size changes

      moveResize(frame().x(), frame().y(),
                 frame().width(), frame().height() );
      frameExtentChanged();
      break;
  } // XA_WM_NORMAL_HINTS
  default:
      SbAtoms *sbatoms = SbAtoms::instance();
      if (atom == sbatoms->getWMProtocolsAtom() )
        client.updateWMProtocols();
      else if (atom == sbatoms->getMWMHintsAtom() ) {
        client.updateMWMHints();
        updateMWMHintsFromClient(client);
        if (!m_toggled_decos)
          Remember::instance().updateDecoStateFromClient(client);
        applyDecorations(); // update decorations (if they changed)
      }
      break;
  } // switch(atom)
} // propertyNotifyEvent

void ShyneboxWindow::exposeEvent(XExposeEvent &ee) {
  frame().exposeEvent(ee);
}

void ShyneboxWindow::configureRequestEvent(XConfigureRequestEvent &cr) {
  WinClient *client = findClient(cr.window);
  if (client == 0 || isIconic() )
    return;

  int old_x = frame().x(), old_y = frame().y();
  unsigned int old_w = frame().width();
  unsigned int old_h = frame().height() - frame().titlebarHeight()
                     - frame().handleHeight();
  int cx = old_x, cy = old_y, ignore = 0;
  unsigned int cw = old_w, ch = old_h;

  // make sure the new width/height would be ok with all clients, or else they
  // could try to resize the window back and forth
  if (cr.value_mask & CWWidth || cr.value_mask & CWHeight) {
      unsigned int new_w = (cr.value_mask & CWWidth) ? cr.width : cw;
      unsigned int new_h = (cr.value_mask & CWHeight) ? cr.height : ch;
      for (auto it : m_clientlist)
        if (it != client && !it->sizeHints().valid(new_w, new_h) )
          cr.value_mask = cr.value_mask & ~(CWWidth | CWHeight);
  } // if CWWidth || CWHeight

  // don't allow moving/resizing fullscreen or maximized windows
  if (isFullscreen() || (isMaximizedHorz() && screen().getMaxIgnoreIncrement() ) )
    cr.value_mask = cr.value_mask & ~(CWWidth | CWX);
  if (isFullscreen() || (isMaximizedVert() && screen().getMaxIgnoreIncrement() ) )
    cr.value_mask = cr.value_mask & ~(CWHeight | CWY);

  // don't let misbehaving clients (e.g. MPlayer) move/resize their windows
  // just after creation if the user has a saved position/size
  if (m_creation_time) {
    uint64_t now = tk::SbTime::mono();
    Remember& rinst = Remember::instance();

    if (now > (m_creation_time + tk::SbTime::IN_SECONDS) ) {
      m_creation_time = 0;
    } else if (rinst.isRemembered(*client, Remember::REM_MAXIMIZEDSTATE)
               || rinst.isRemembered(*client, Remember::REM_FULLSCREENSTATE) ) {
      cr.value_mask = cr.value_mask & ~(CWWidth | CWHeight);
      cr.value_mask = cr.value_mask & ~(CWX | CWY);
    } else {
      if (rinst.isRemembered(*client, Remember::REM_DIMENSIONS) )
        cr.value_mask = cr.value_mask & ~(CWWidth | CWHeight);
      if (rinst.isRemembered(*client, Remember::REM_POSITION) )
        cr.value_mask = cr.value_mask & ~(CWX | CWY);
    }
  } // if creation_time

  if (cr.value_mask & CWBorderWidth)
    client->old_bw = cr.border_width;

  if ((cr.value_mask & CWX)
      && (cr.value_mask & CWY) ) {
    cx = cr.x;
    cy = cr.y;
    frame().gravityTranslate(cx, cy, client->gravity(), client->old_bw);
    frame().setActiveGravity(client->gravity(), client->old_bw);
  } else if (cr.value_mask & CWX) {
    cx = cr.x;
    frame().gravityTranslate(cx, ignore, client->gravity(), client->old_bw);
    frame().setActiveGravity(client->gravity(), client->old_bw);
  } else if (cr.value_mask & CWY) {
    cy = cr.y;
    frame().gravityTranslate(ignore, cy, client->gravity(), client->old_bw);
    frame().setActiveGravity(client->gravity(), client->old_bw);
  }

  if (cr.value_mask & CWWidth)
    cw = cr.width;

  if (cr.value_mask & CWHeight)
    ch = cr.height;

  // the request is for client window so we resize the frame to it first
  if (old_w != cw || old_h != ch) {
    if (old_x != cx || old_y != cy)
      frame().moveResizeForClient(cx, cy, cw, ch);
    else
      frame().resizeForClient(cw, ch);
  } else if (old_x != cx || old_y != cy)
    frame().move(cx, cy);

  if (cr.value_mask & CWStackMode) {
    switch (cr.detail) {
    case Above:
    case TopIf:
    default:
      if ((isFocused() && focusRequestFromClient(*client) )
          || !FocusControl::focusedWindow() ) {
        setCurrentClient(*client, true);
        raise();
      } else if (getRootTransientFor(client) ==
                   getRootTransientFor(FocusControl::focusedWindow() ) ) {
        setCurrentClient(*client, false);
        raise();
      }
      break;
    case Below:
    case BottomIf:
      lower();
      break;
    }
  } // if CWStackMode
  sendConfigureNotify();
} // configureRequestEvent

// keep track of last keypress in window, so we can decide not to focusNew
void ShyneboxWindow::keyPressEvent(XKeyEvent &ke) {
  // if there's a modifier key down, the user probably expects the new window
  if (tk::KeyUtil::instance().cleanMods(ke.state) )
    return;

  // we need to ignore modifier keys themselves, too
  KeySym ks;
  char keychar[1];
  XLookupString(&ke, keychar, 1, &ks, 0);
  if (IsModifierKey(ks) )
    return;

  // if the key was return/enter, the user probably expects the window
  // e.g., typed the command in a terminal
  if (ks == XK_KP_Enter || ks == XK_Return) {
    // we'll actually reset the time for this one
    m_last_keypress_time = 0;
    return;
  }
  // otherwise, make a note that the user is typing
  m_last_keypress_time = tk::SbTime::mono();
} // keyPressEvent

bool ShyneboxWindow::isTyping() const {
  uint64_t diff = tk::SbTime::mono() - m_last_keypress_time;
  return ((diff / 1000) < screen().noFocusWhileTypingDelay() );
}

void ShyneboxWindow::buttonPressEvent(XButtonEvent &be) {
  const bool was_mv_rsz = m_last_action_button != 0
          || moving || resizing || m_attaching_tab;
  // do NOT change the last positions or button
  // if using motion for moving/resizing/tabbing
  // motionNotifyEvent() uses these values. release events should
  // only acknowledge and stop moves/resize/tab if the last button
  // was what started that action.
  // ewmh _NEW_WM_MOVERESIZE_MOVE can start move/resize too (-1 for 'button')
  //
  // NOTE: there may be a way to break this with certain key setups
  //       as start move only checks 'if moving' and start resize 'if resizing'
  //       other actions can also trigger, and may conflict
  if (!was_mv_rsz) {
    m_last_button_x = be.x_root;
    m_last_button_y = be.y_root;
  }

  tk::Menu::hideShownMenu();

  Keys *k = Shynebox::instance()->keys();
  int context = 0;
  context = frame().getContext(be.subwindow ? be.subwindow : be.window, be.x_root, be.y_root);
  if (!context && be.subwindow)
    context = frame().getContext(be.window);

  if (k->doAction(be.type, be.state, be.button, context, &winClient(), be.time) ) {
    XAllowEvents(display, SyncPointer, CurrentTime);
    if (!was_mv_rsz && (moving || resizing || m_attaching_tab) )
      m_last_action_button = be.button;
    return; // if an action was done, assume that's all we wanted
  }

  // if an action didn't run, probably ended up here because of
  // Button1 grab for focus, replay the pointer to the client
  XAllowEvents(display, ReplayPointer, CurrentTime);

  // at this point, xallowevents could have triggered move/resize
  // in which case m_last_action_button = -1

  // only raise/focus for left clicks and if not cycling
  // (chances are it is button 1, and cycling is intentional)
  if (be.button != Button1 && !screen().focusControl().isCycling() )
    return;

  if (frame().window().window() == be.window) {
    if (screen().clickRaisesWindows() )
      raise();

    m_button_grab_x = be.x_root - frame().x() - frame().window().borderWidth();
    m_button_grab_y = be.y_root - frame().y() - frame().window().borderWidth();
  }

  // CLICKFOCUS is sort of the default here. because if it's
  // MOUSE, then you're already focused before a click
  if (!m_focused && acceptsFocus() && m_click_focus)
    focus();

  WinClient *client = 0;
  // determine if we're in a label button (tab)
  if (!screen().focusControl().isMouseTabFocus() )
    client = winClientOfLabelButtonWindow(be.window);

  if (!screen().focusControl().isMouseTabFocus()
      && client && client != m_client
      && !screen().focusControl().isIgnored(be.x_root, be.y_root) )
    setCurrentClient(*client, isFocused() );
} // buttonPressEvent

const int DEADZONE = 4;

void ShyneboxWindow::buttonReleaseEvent(XButtonEvent &re) {
  // only stop current motion actions on button source
  // see press-event comment above
  if (m_last_action_button == static_cast<int>(re.button)
      || m_last_action_button == -1) { // _NEW_WM_MOVERESIZE_MOVE
    m_last_action_button = 0;
    if (moving)
      stopMoving();
    else if (resizing)
      stopResizing();
    else if (m_attaching_tab) {
      attachTo(re.x_root, re.y_root);
      return; // may be about to commit sudoku
    }
  }
  // you can have  OnTitlebar Mod1 Mouse1 :Something1
  //          and  OnTitlebar Mod1 Click1 :Something2
  // deadzone will likely stop Click1
  // im not sure of the use case but, sort of a backup action?
  if (abs(m_last_button_x - re.x_root)
      + abs(m_last_button_y - re.y_root) < DEADZONE) {
    int context = frame().getContext(re.subwindow ? re.subwindow
                                 : re.window, re.x_root, re.y_root);
    if (!context && re.subwindow)
      context = frame().getContext(re.window);

    Shynebox::instance()->keys()->doAction(re.type, re.state, re.button,
                                          context, &winClient(), re.time);
  }
} // buttonReleaseEvent

void ShyneboxWindow::motionNotifyEvent(XMotionEvent &me) {
  if (moving || m_attaching_tab) {
    if (moving && me.window == parent() )
      me.window = frame().window().window();

    XEvent e;

    if (XCheckTypedEvent(display, MotionNotify, &e) ) {
      XPutBackEvent(display, &e);
      return;
    }

    #define SINT_ static_cast<int>
    const int bw = SINT_(frame().window().borderWidth() );
    int w = m_attaching_tab ? SINT_(m_labelbuttons[m_attaching_tab]->width() / 2)
                            : SINT_(frame().width() ) + 2*bw - 1;
    int h = m_attaching_tab ? SINT_(m_labelbuttons[m_attaching_tab]->height() / 2)
                            : SINT_(frame().height() ) + 2*bw - 1;
    #undef SINT
    const bool xor_outline = (m_attaching_tab || !screen().doOpaqueMove() )
                              && (w > 0) && (h > 0);
    // the m_last move/resize variables kind of step over each other
    // Warp to next or previous workspace?, must have moved sideways some
    int moved_x = me.x_root - m_last_resize_x;
    // Warp to a workspace offset (if treating workspaces like a grid)
    int moved_y = me.y_root - m_last_resize_y;
    // save last event point
    m_last_resize_x = me.x_root;
    m_last_resize_y = me.y_root;

    if (xor_outline)
      parent().drawRectangle(screen().rootTheme()->opGC(),
                             m_last_move_x, m_last_move_y, w, h);

    // check for warping
    //
    // +--monitor-1--+--monitor-2---+
    // |w            |             w|
    // |w            |             w|
    // +-------------+--------------+
    //
    // mouse-warping is enabled, the mouse needs to be in the "warp_pad"
    // zone.
    //
    const int  warp_pad            = screen().getEdgeSnapThreshold();
    const int  workspaces          = screen().numberOfWorkspaces();
    const bool is_warping_horzntal = screen().isWorkspaceWarpingHorizontal(),
               is_warping_vertical = screen().isWorkspaceWarpingVertical(),
               is_warping          = is_warping_horzntal || is_warping_vertical;

    if ((moved_x || moved_y) && is_warping) {
      unsigned int cur_id = screen().currentWorkspaceID(),
                   new_id = cur_id;

      // border threshold
      int bt_left   = warp_pad,
          bt_bottom = warp_pad,
          bt_right  = int(screen().width() ) - warp_pad - 1,
          bt_top    = int(screen().height() ) - warp_pad - 1;

      if (moved_x && is_warping_horzntal) {
        const int warp_offset = screen().getWorkspaceWarpingHorizontalOffset();
        if (me.x_root >= bt_right && moved_x > 0) { //warp right
          new_id          = (cur_id + warp_offset) % workspaces;
          m_last_resize_x = 0;
        } else if (me.x_root <= bt_left && moved_x < 0) { //warp left
          new_id          = (cur_id + workspaces - warp_offset) % workspaces;
          m_last_resize_x = screen().width() - 1;
        }
      } else if (moved_y && is_warping_vertical) { // should only try 1 at a time
        const int warp_offset = screen().getWorkspaceWarpingVerticalOffset();
        if (me.y_root >= bt_top && moved_y > 0) { // warp down
          new_id          = (cur_id + warp_offset) % workspaces;
          m_last_resize_y = 0;
        } else if (me.y_root <= bt_bottom && moved_y < 0) { // warp up
          new_id          = (cur_id + workspaces - warp_offset) % workspaces;
          m_last_resize_y = screen().height() - 1;
        }
      }

      if (new_id != cur_id) { // if we are warping
        // remove motion events from queue to avoid repeated warps
        while (XCheckTypedEvent(display, MotionNotify, &e) )
          m_last_resize_y = e.xmotion.y_root;
          // might as well update the y-coordinate

        // move the pointer to (m_last_resize_x,m_last_resize_y)
        XWarpPointer(display, None, me.root, 0, 0, 0, 0,
                     m_last_resize_x, m_last_resize_y);

        // tabbing grabs the pointer, we must not hide the window!
        if (m_attaching_tab || screen().doOpaqueMove() )
          screen().sendToWorkspace(new_id, this, true);
        else
          screen().changeWorkspaceID(new_id, false);
      }
    } // if moved and warping enabled

    int dx = m_last_resize_x - m_button_grab_x,
        dy = m_last_resize_y - m_button_grab_y;

    dx -= frame().window().borderWidth();
    dy -= frame().window().borderWidth();
    // somehow, this ^ saves bytes?
    //dx -= bw;
    //dy -= bw;

    // dx is current left side, dy is current top
    if (moving)
      doSnapping(dx, dy);

    if (xor_outline) {
      parent().drawRectangle(screen().rootTheme()->opGC(), dx, dy, w, h);

      m_last_move_x = dx;
      m_last_move_y = dy;
    } else {
      frame().quietMoveResize(dx, dy, frame().width(), frame().height() );
    } // if xor_outline

    if (moving)
      screen().showPosition(dx, dy);
  } else if (resizing) { // end if moving / attaching
    const int bw = static_cast<int>(frame().window().borderWidth() );
    int old_resize_x = m_last_resize_x,
        old_resize_y = m_last_resize_y,
        old_resize_w = m_last_resize_w,
        old_resize_h = m_last_resize_h;

    int dx = me.x_root - m_button_grab_x,
        dy = me.y_root - m_button_grab_y;

    if (m_resize_corner == LEFTTOP || m_resize_corner == LEFTBOTTOM
        || m_resize_corner == LEFT) {
      m_last_resize_w = resize_base_w - dx;
      m_last_resize_x = resize_base_x + dx;
    }

    if (m_resize_corner == LEFTTOP || m_resize_corner == RIGHTTOP
        || m_resize_corner == TOP) {
      m_last_resize_h = resize_base_h - dy;
      m_last_resize_y = resize_base_y + dy;
    }

    if (m_resize_corner == LEFTBOTTOM || m_resize_corner == BOTTOM
        || m_resize_corner == RIGHTBOTTOM)
      m_last_resize_h = resize_base_h + dy;

    if (m_resize_corner == RIGHTBOTTOM || m_resize_corner == RIGHTTOP
        || m_resize_corner == RIGHT)
      m_last_resize_w = resize_base_w + dx;

    if (m_resize_corner == CENTER) {
      // dx or dy must be at least 2
      if (abs(dx) >= 2 || abs(dy) >= 2) {
        // take max and make it even
        //int diff = 2 * (max(dx, dy) / 2);
        const int diff = max(dx,dy) & !1;

        m_last_resize_h =  resize_base_h + diff;

        m_last_resize_w = resize_base_w + diff;
        m_last_resize_x = resize_base_x - diff/2;
        m_last_resize_y = resize_base_y - diff/2;
      }
    } // if CENTER

    fixSize(); // adjust size based on window size hints
    frame().displaySize(m_last_resize_w, m_last_resize_h);

    if (old_resize_x != m_last_resize_x
        || old_resize_y != m_last_resize_y
        || old_resize_w != m_last_resize_w
        || old_resize_h != m_last_resize_h) {
      if (screen().getEdgeResizeSnapThreshold() != 0) {
        int tx, ty;
        int botright_x = m_last_resize_x + m_last_resize_w;
        int botright_y = m_last_resize_y + m_last_resize_h;

        switch (m_resize_corner) {
        case LEFTTOP:
            tx = m_last_resize_x;
            ty = m_last_resize_y;
            doSnapping(tx, ty, true);

            m_last_resize_x = tx;
            m_last_resize_y = ty;
            m_last_resize_w = botright_x - m_last_resize_x;
            m_last_resize_h = botright_y - m_last_resize_y;
            break;
        case LEFTBOTTOM:
            tx = m_last_resize_x;
            ty = m_last_resize_y + m_last_resize_h;
            ty += bw * 2;
            doSnapping(tx, ty, true);
            ty -= bw * 2;

            m_last_resize_x = tx;
            m_last_resize_h = ty - m_last_resize_y;
            m_last_resize_w = botright_x - m_last_resize_x;
            break;
        case RIGHTTOP:
            tx = m_last_resize_x + m_last_resize_w;
            ty = m_last_resize_y;
            tx += bw * 2;
            doSnapping(tx, ty, true);
            tx -= bw * 2;

            m_last_resize_w = tx - m_last_resize_x;
            m_last_resize_y = ty;
            m_last_resize_h = botright_y - m_last_resize_y;
            break;
        case RIGHTBOTTOM:
            tx = m_last_resize_x + m_last_resize_w;
            ty = m_last_resize_y + m_last_resize_h;
            tx += bw * 2;
            ty += bw * 2;
            doSnapping(tx, ty, true);
            tx -= bw * 2;
            ty -= bw * 2;

            m_last_resize_w = tx - m_last_resize_x;
            m_last_resize_h = ty - m_last_resize_y;
            break;
        default:
            break;
        } // switch(m_resize_corner)
      } // if snap thresh != 0

      if (m_last_resize_w != old_resize_w || m_last_resize_h != old_resize_h) {
        if (screen().doOpaqueResize() )
          m_resize_timer.start();
        else {
          // draw over old rect
          parent().drawRectangle(screen().rootTheme()->opGC(),
                  old_resize_x, old_resize_y,
                  old_resize_w - 1 + 2 * bw,
                  old_resize_h - 1 + 2 * bw);

          // draw resize rectangle
          parent().drawRectangle(screen().rootTheme()->opGC(),
                  m_last_resize_x, m_last_resize_y,
                  m_last_resize_w - 1 + 2 * bw,
                  m_last_resize_h - 1 + 2 * bw);
        } // opaque resize
      } // m_last_resize_w/h != old
    } // if old_x != last_resize_x
  } // resizing
} // motionNotifyEvent

void ShyneboxWindow::enterNotifyEvent(XCrossingEvent &ev) {
  static ShyneboxWindow *s_last_really_entered = 0;

  // if this results from an ungrab, only act if the window really changed.
  // otherwise we might pollute the focus which could have been assigned
  // by alt+tab (bug #597)
  if (ev.mode == NotifyUngrab && s_last_really_entered == this)
    return;

  // ignore grab activates, or if we're not visible
  if (ev.mode == NotifyGrab || !isVisible() )
    return;

  s_last_really_entered = this;
  if (ev.window == frame().window() )
    Shynebox::instance()->keys()->doAction(ev.type, ev.state, 0,
                                    Keys::ON_WINDOW, m_client);

  // determine if we're in a label button (tab)
  WinClient *client = winClientOfLabelButtonWindow(ev.window);
  if (client) {
    if (IconButton *tab = m_labelbuttons[client]) {
      m_has_tooltip = true;
      tab->showTooltip();
    }
  }

  if ((ev.window == frame().window() || ev.window == m_client->window() || client)
       && screen().focusControl().isMouseFocus() && !isFocused() && acceptsFocus() ) {
    // check that there aren't any subsequent LeaveNotify events in the
    // X event queue
    XEvent dummy;
    scanargs sa;
    sa.w = ev.window;
    sa.enter = sa.leave = False;
    XCheckIfEvent(display, &dummy, queueScanner, (char *) &sa);

    if ((!sa.leave || sa.inferior)
        && !screen().focusControl().isCycling()
        && !screen().focusControl().isIgnored(ev.x_root, ev.y_root) )
      focus();
  } // if ev for window or client label

  if (screen().focusControl().isMouseTabFocus()
      && client && client != m_client
      && !screen().focusControl().isIgnored(ev.x_root, ev.y_root) )
    m_tab_activate_timer.start();
} // enterNotifyEvent

void ShyneboxWindow::leaveNotifyEvent(XCrossingEvent &ev) {
  // ignore grab activates, or if we're not visible
  if (ev.mode == NotifyGrab || ev.mode == NotifyUngrab
      || !isVisible() )
    return;

  if (m_has_tooltip) {
    m_has_tooltip = false;
    screen().hideTooltip();
  }

  // still inside?
  if (ev.x_root > frame().x() && ev.y_root > frame().y()
      && ev.x_root <= (int)(frame().x() + frame().width() )
      && ev.y_root <= (int)(frame().y() + frame().height() ) )
    return;

  Shynebox::instance()->keys()->doAction(ev.type, ev.state, 0,
                                        Keys::ON_WINDOW, m_client);
}

void ShyneboxWindow::setTitle(const std::string& title, Focusable &client) {
  if (&client != m_client)
    return;

  frame().setFocusTitle(title);
#if USE_TOOLBAR
  screen().updateIconbar();
#endif
  Shynebox::instance()->clientListChanged(screen() ); // updates client menus
}

void ShyneboxWindow::frameExtentChanged() {
  if (!m_initialized)
    return;

  // updates EWMH
  Shynebox::instance()->updateFrameExtents(*this);
  sendConfigureNotify();
}

// focus control calls this
void ShyneboxWindow::themeReconfigured() {
  applyDecorations();
}

// commit current decoration values to actual displayed things
void ShyneboxWindow::applyDecorations() {
  frame().setDecorationMask(decorationMask() );
  frame().applyDecorations();
  frameExtentChanged();
}

void ShyneboxWindow::toggleDecoration() {
  if (isShaded() || isFullscreen() )
    return;

  m_toggled_decos = !m_toggled_decos;

  if (m_toggled_decos) {
    m_old_decoration_mask = decorationMask();
    if (decorations.titlebar | decorations.tab)
      setDecorationMask(WindowState::DECOR_NONE);
    else
      setDecorationMask(WindowState::DECOR_NORMAL);
  } else //revert back to old decoration
    setDecorationMask(m_old_decoration_mask);
}

unsigned int ShyneboxWindow::decorationMask() const {
  unsigned int ret = 0;
  if (decorations.titlebar)
    ret |= WindowState::DECORM_TITLEBAR;
  if (decorations.handle)
    ret |= WindowState::DECORM_HANDLE;
  if (decorations.border)
    ret |= WindowState::DECORM_BORDER;
  if (decorations.iconify)
    ret |= WindowState::DECORM_ICONIFY;
  if (decorations.maximize)
    ret |= WindowState::DECORM_MAXIMIZE;
  if (decorations.close)
    ret |= WindowState::DECORM_CLOSE;
  if (decorations.menu)
    ret |= WindowState::DECORM_MENU;
  if (decorations.sticky)
    ret |= WindowState::DECORM_STICKY;
  if (decorations.shade)
    ret |= WindowState::DECORM_SHADE;
  if (decorations.tab)
    ret |= WindowState::DECORM_TAB;
  if (decorations.enabled)
    ret |= WindowState::DECORM_ENABLED;
  return ret;
}

void ShyneboxWindow::setDecorationMask(unsigned int mask, bool apply) {
  decorations.titlebar = mask & WindowState::DECORM_TITLEBAR;
  decorations.handle   = mask & WindowState::DECORM_HANDLE;
  decorations.border   = mask & WindowState::DECORM_BORDER;
  decorations.iconify  = mask & WindowState::DECORM_ICONIFY;
  decorations.maximize = mask & WindowState::DECORM_MAXIMIZE;
  decorations.close    = mask & WindowState::DECORM_CLOSE;
  decorations.menu     = mask & WindowState::DECORM_MENU;
  decorations.sticky   = mask & WindowState::DECORM_STICKY;
  decorations.shade    = mask & WindowState::DECORM_SHADE;
  decorations.tab      = mask & WindowState::DECORM_TAB;
  decorations.enabled  = mask & WindowState::DECORM_ENABLED;

  // we don't want to do this during initialization
  if (apply)
    applyDecorations();
}

void ShyneboxWindow::startMoving(int x, int y) {
  if (((isMaximized() || isFullscreen() ) && screen().getMaxDisableMove() )
       || moving || s_num_grabs > 0)
    return;

  if (m_last_action_button == 0)
    m_last_action_button = -1; // _NEW_WM_MOVERESIZE_MOVE

  // save first event point
  m_last_resize_x = x;
  m_last_resize_y = y;
  m_button_grab_x = x - frame().x() - frame().window().borderWidth();
  m_button_grab_y = y - frame().y() - frame().window().borderWidth();

  moving = true;

  Shynebox *shynebox = Shynebox::instance();
  // grabbing (and masking) on the root window allows us to
  // freely map and unmap the window we're moving.
  grabPointer(screen().rootWindow().window(), False, ButtonMotionMask |
              ButtonReleaseMask, GrabModeAsync, GrabModeAsync,
              screen().rootWindow().window(), frame().theme()->moveCursor(), CurrentTime);

  if (menu().isVisible() )
    menu().hide();

  shynebox->maskWindowEvents(screen().rootWindow().window(), this);

  m_last_move_x = frame().x();
  m_last_move_y = frame().y();
  if (!screen().doOpaqueMove() ) {
    shynebox->grab();
    parent().drawRectangle(screen().rootTheme()->opGC(),
                           frame().x(), frame().y(),
                           frame().width() + 2*frame().window().borderWidth()-1,
                           frame().height() + 2*frame().window().borderWidth()-1);
    screen().showPosition(frame().x(), frame().y() );
  }
} // startMoving

void ShyneboxWindow::stopMoving(bool interrupted) {
  moving = false;
  Shynebox *shynebox = Shynebox::instance();

  shynebox->maskWindowEvents(0, 0);

  if (!screen().doOpaqueMove() ) {
    parent().drawRectangle(screen().rootTheme()->opGC(),
                           m_last_move_x, m_last_move_y,
                           frame().width() + 2*frame().window().borderWidth()-1,
                           frame().height() + 2*frame().window().borderWidth()-1);
    if (!interrupted) {
      moveResize(m_last_move_x, m_last_move_y, frame().width(), frame().height() );
      if (m_workspace_number != screen().currentWorkspaceID() )
        screen().sendToWorkspace(screen().currentWorkspaceID(), this);
      focus();
    }
    shynebox->ungrab();
  } else if (!interrupted)
    moveResize(frame().x(), frame().y(), frame().width(), frame().height(), true);

  screen().hidePosition();
  ungrabPointer(CurrentTime);

  tk::App::instance()->sync(false); //make sure the redraw is made before we continue

  if (m_state.maximized || m_state.fullscreen) {
    frame().applyState();
    frameExtentChanged();
    Shynebox::instance()->windowStateChanged(*this);
  }
} // stopMoving

namespace {
// previously inlined, took up ~500 more bytes for i assume little performance
void snapToWindow(int &xlimit, int &ylimit,
                  int left, int right, int top, int bottom,
                  int oleft, int oright, int otop, int obottom,
                  bool resize) {
  // Only snap if we're adjacent to the edge we're looking at

  // for left + right, need to be in the right y range
  if (top <= obottom && bottom >= otop) {
    // left
    if (abs(left-oleft)  < abs(xlimit) )               xlimit = -(left-oleft);
    if (abs(right-oleft) < abs(xlimit) )               xlimit = -(right-oleft);
    // right
    if (abs(left-oright)  < abs(xlimit) )              xlimit = -(left-oright);
    if (!resize && abs(right-oright) < abs(xlimit) )   xlimit = -(right-oright);
  }

  // for top + bottom, need to be in the right x range
  if (left <= oright && right >= oleft) {
    // top
    if (abs(top-otop)    < abs(ylimit) )               ylimit = -(top-otop);
    if (abs(bottom-otop) < abs(ylimit) )               ylimit = -(bottom-otop);
    // bottom
    if (abs(top-obottom) < abs(ylimit) )               ylimit = -(top-obottom);
    if (!resize && abs(bottom-obottom) < abs(ylimit) ) ylimit = -(bottom-obottom);
  }
} // snapToWindow

} // anonymous namespace

void ShyneboxWindow::doSnapping(int &orig_left, int &orig_top, bool resize) {
  int threshold; // also bool 'do_snapping?'

  if (resize)
    threshold = screen().getEdgeResizeSnapThreshold();
  else
    threshold = screen().getEdgeSnapThreshold();

  if (0 == threshold)
    return;

  const bool external_tabs = frame().externalTabMode() && !screen().getMaxOverTabs();

  int dx = threshold + 1, // Keep track of our best offsets so far
      dy = threshold + 1, // We need to find things less than or equal to the threshold
      my_bw = decorationMask() & (WindowState::DECORM_BORDER|WindowState::DECORM_HANDLE)
              ? frame().window().borderWidth() * 2 : 0,
      my_top   = orig_top, // orig include the borders
      my_left  = orig_left,
      // border width centering is handled elsewhere, only apply to right/bot
      my_right = orig_left + width() + my_bw,
      my_bot   = orig_top + height() + my_bw,
      cur_head = 0, // head "0" == whole screen width + height
      max_head = screen().numHeads();

  // only check self against tabs, other windows will check w/ and w/o tab
  if (external_tabs) {
    my_left  -= xOffset();
    my_right += widthOffset() - xOffset();
    my_top   -= yOffset();
    my_bot   += heightOffset() - yOffset();
  }

  // begin by checking the screen (xrandr 'head') edges

  if (!screen().doObeyHeads() )  // skip head snapping
    max_head = 0;
  else if (max_head > 1)
    cur_head = 1;

  for ( ; cur_head < max_head ; cur_head++)
    snapToWindow(dx, dy, my_left, my_right, my_top, my_bot,
                 screen().maxLeft(cur_head),
                 screen().maxRight(cur_head),
                 screen().maxTop(cur_head),
                 screen().maxBottom(cur_head),
                 resize);

  // now check window edges
  for (auto &it : screen().currentWorkspace()->windowList() ) {
    if (it == this || it->isIconic() )
      continue; // skip self and minimized windows

    const int it_bw = it->decorationMask() & (WindowState::DECORM_BORDER|WindowState::DECORM_HANDLE)
                      ? it->frame().window().borderWidth() * 2 : 0,
              it_x = it->x(),
              it_y = it->y(),
              it_w = it->width() + it_bw,
              it_h = it->height() + it_bw;

    snapToWindow(dx, dy, my_left, my_right, my_top, my_bot,
                 it_x,
                 it_x + it_w,
                 it_y,
                 it_y + it_h,
                 resize);

    // also snap to the box containing the tabs
    if (it->frame().externalTabMode() ) {
      const int   it_xoff = it->xOffset(),
                  it_yoff = it->yOffset(),
                  it_woff = it->widthOffset(),
                  it_hoff = it->heightOffset();

      snapToWindow(dx, dy, my_left, my_right, my_top, my_bot,
                   it_x - it_xoff,
                   it_x - it_xoff + it_w + it_woff,
                   it_y - it_yoff,
                   it_y - it_yoff + it_h + it_hoff,
                   resize);
    }
  } // for ws windows

  // commit
  if (dx <= threshold)
    orig_left += dx;
  if (dy <= threshold)
    orig_top  += dy;
} // doSnapping

// This is initiated by CurrentWindowCmd and the corner sizes, pixels, percents
// are set there
ShyneboxWindow::ReferenceCorner ShyneboxWindow::getResizeDirection(int x, int y,
        ResizeModel model, int corner_size_px, int corner_size_pc) const {
  switch (model) {
  case TOPLEFTRESIZE:     return LEFTTOP;
  case TOPRESIZE:         return TOP;
  case TOPRIGHTRESIZE:    return RIGHTTOP;
  case LEFTRESIZE:        return LEFT;
  case RIGHTRESIZE:       return RIGHT;
  case BOTTOMLEFTRESIZE:  return LEFTBOTTOM;
  case BOTTOMRESIZE:      return BOTTOM;
  case CENTERRESIZE:      return CENTER;
  default:
  break;
  }

  if (model == EDGEORCORNERRESIZE) {
    int w = frame().width();
    int h = frame().height();
    int cx = w / 2;
    int cy = h / 2;
    TestCornerHelper test_corner = { corner_size_px, corner_size_pc };
    if (x < cx  &&  test_corner(x, cx) ) {
      if (y < cy  &&  test_corner(y, cy) )
        return LEFTTOP;
      else if (test_corner(h - y - 1, h - cy) )
        return LEFTBOTTOM;
    } else if (test_corner(w - x - 1, w - cx) ) {
      if (y < cy  &&  test_corner(y, cy) )
        return RIGHTTOP;
      else if (test_corner(h - y - 1, h - cy) )
        return RIGHTBOTTOM;
    }

    /* Nope, not a corner; find the nearest edge instead. */
    if (cy - abs(y - cy) < cx - abs(x - cx) ) // y is nearest
      return (y > cy) ? BOTTOM : TOP;
    else
      return (x > cx) ? RIGHT : LEFT;
  } // if corner
  return RIGHTBOTTOM;
} // getResizeDirection

void ShyneboxWindow::startResizing(int x, int y, ReferenceCorner dir) {
  if (((isMaximized() || isFullscreen() ) && screen().getMaxDisableResize() )
       || resizing || s_num_grabs > 0 || isShaded() || isIconic() )
    return;

  if (m_last_action_button == 0)
    m_last_action_button = -1; // _NEW_WM_MOVERESIZE_MOVE

  m_resize_corner = dir;
  resizing = true;
  disableMaximization();

  const Cursor& cursor = (m_resize_corner == LEFTTOP) ? frame().theme()->upperLeftAngleCursor() :
                         (m_resize_corner == RIGHTTOP) ? frame().theme()->upperRightAngleCursor() :
                         (m_resize_corner == RIGHTBOTTOM) ? frame().theme()->lowerRightAngleCursor() :
                         (m_resize_corner == LEFT) ? frame().theme()->leftSideCursor() :
                         (m_resize_corner == RIGHT) ? frame().theme()->rightSideCursor() :
                         (m_resize_corner == TOP) ? frame().theme()->topSideCursor() :
                         (m_resize_corner == BOTTOM) ? frame().theme()->bottomSideCursor() :
                                                       frame().theme()->lowerLeftAngleCursor();

  grabPointer(sbWindow().window(),
              false, ButtonMotionMask | ButtonReleaseMask,
              GrabModeAsync, GrabModeAsync, None, cursor, CurrentTime);

  m_button_grab_x = x + frame().x();
  m_button_grab_y = y + frame().y();
  resize_base_x = m_last_resize_x = frame().x();
  resize_base_y = m_last_resize_y = frame().y();
  resize_base_w = m_last_resize_w = frame().width();
  resize_base_h = m_last_resize_h = frame().height();

  fixSize();
  frame().displaySize(m_last_resize_w, m_last_resize_h);

  if (!screen().doOpaqueResize() )
    parent().drawRectangle(screen().rootTheme()->opGC(),
                   m_last_resize_x, m_last_resize_y,
                   m_last_resize_w - 1 + 2 * frame().window().borderWidth(),
                   m_last_resize_h - 1 + 2 * frame().window().borderWidth() );
} // startResizing

void ShyneboxWindow::stopResizing(bool interrupted) {
  resizing = false;

  if (!screen().doOpaqueResize() )
    parent().drawRectangle(screen().rootTheme()->opGC(),
                       m_last_resize_x, m_last_resize_y,
                       m_last_resize_w - 1 + 2 * frame().window().borderWidth(),
                       m_last_resize_h - 1 + 2 * frame().window().borderWidth() );

  screen().hideGeometry();

  if (!interrupted) {
    fixSize();
    moveResize(m_last_resize_x, m_last_resize_y,
               m_last_resize_w, m_last_resize_h);
    frameExtentChanged();
  }

  ungrabPointer(CurrentTime);
} // stopResizing

WinClient* ShyneboxWindow::winClientOfLabelButtonWindow(Window window) {
  for (auto &it : m_labelbuttons)
    if (it.second->window() == window)
      return it.first;
  return 0;
}

void ShyneboxWindow::startTabbing(const XButtonEvent &be) {
  if (s_num_grabs > 0)
    return;

  m_attaching_tab = winClientOfLabelButtonWindow(be.window);
  if (m_attaching_tab == 0)
    m_attaching_tab = m_client;
    // m_client should always be valid

  s_original_workspace = workspaceNumber();

  if (m_last_action_button == 0)
    m_last_action_button = -1; // see buttonPressEvent

  // start drag'n'drop for tab
  grabPointer(be.window, False, ButtonMotionMask |
              ButtonReleaseMask, GrabModeAsync, GrabModeAsync,
              None, frame().theme()->moveCursor(), CurrentTime);

  // relative position on the button
  m_button_grab_x = be.x;
  m_button_grab_y = be.y;
  // position of the button
  m_last_move_x = be.x_root - be.x;
  m_last_move_y = be.y_root - be.y;
  // hijack extra vars for initial grab location
  m_last_resize_x = be.x_root;
  m_last_resize_y = be.y_root;

  Shynebox::instance()->grab();

  IconButton &active_button = *m_labelbuttons[m_attaching_tab];
  m_last_resize_w = active_button.width() / 2;
  m_last_resize_h = active_button.height() / 2;

  parent().drawRectangle(screen().rootTheme()->opGC(),
                         m_last_move_x, m_last_move_y,
                         m_last_resize_w, m_last_resize_h);

  menu().hide();
} // startTabbing

void ShyneboxWindow::attachTo(int x, int y, bool interrupted) {
  // clear movement rect
  parent().drawRectangle(screen().rootTheme()->opGC(),
                         m_last_move_x, m_last_move_y,
                         m_last_resize_w, m_last_resize_h);

  ungrabPointer(CurrentTime);
  Shynebox::instance()->ungrab();

  // make sure we clean up here
  // since this object may be deleted inside attachClient
  WinClient *old_attached = m_attaching_tab;
  m_attaching_tab = 0;

  if (interrupted || old_attached == 0)
    return;

  int dest_x = 0, dest_y = 0;
  Window child = 0;
  if (XTranslateCoordinates(display, parent().window(),
                            parent().window(),
                            x, y, &dest_x, &dest_y, &child) ) {
    bool inside_titlebar = false;
    // search for a shyneboxwindow
    WinClient *client = Shynebox::instance()->searchWindow(child);
    ShyneboxWindow *attach_to_win = 0;
    if (client) {
      inside_titlebar = client->sbwindow()->hasTitlebar()
          && static_cast<signed>(client->sbwindow()->titlebarHeight() )
             + client->sbwindow()->y() > dest_y;
      attach_to_win = client->sbwindow();
    }

    if (attach_to_win != this && attach_to_win != 0 && attach_to_win->isTabable() )
      attach_to_win->attachClient(*old_attached,x,y );
      // we could be deleted here, DO NOT do anything else that alters this object
    else if (attach_to_win != this || (attach_to_win == this && !inside_titlebar) ) {
      // disconnect client if we didn't drop on a window
      WinClient &client = *old_attached;
      detachClient(*old_attached);
      screen().sendToWorkspace(s_original_workspace, this, false);
      if (ShyneboxWindow *sbwin = client.sbwindow() )
        sbwin->move(m_last_move_x, m_last_move_y);
    } else if (attach_to_win == this && attach_to_win->isTabable() )
      moveClientTo(*old_attached, x, y); //reording of tabs within a frame
  } // if xcords over window
} // attachTo

void ShyneboxWindow::restore(WinClient *client, bool remap) {
  if (client->sbwindow() != this)
    return;

  XChangeSaveSet(display, client->window(), SetModeDelete);
  client->setEventMask(NoEventMask);

  int wx = frame().x(), wy = frame().y();
  // don't move the frame, in case there are other tabs in it
  // just set the new coordinates on the reparented window
  frame().gravityTranslate(wx, wy, -client->gravity(), client->old_bw); // negative to invert

  // Why was this hide done? It broke vncviewer (and mplayer?),
  // since it would reparent when going fullscreen.
  // is it needed for anything? Reparent should imply unmap
  // ok, it should hide sometimes, e.g. if the reparent was sent by a client
  //client->hide();

  // restore old border width
  client->setBorderWidth(client->old_bw);
  frameExtentChanged();

  XEvent xev;
  if (! XCheckTypedWindowEvent(display, client->window(), ReparentNotify, &xev) ) {
    sbdbg<<"ShyneboxWindow::restore: reparent 0x"<<hex<<client->window()<<dec<<" to root\n";
    // reparent to root window
    client->reparent(screen().rootWindow(), wx, wy, false);

    if (!remap)
      client->hide();
  }

  if (remap)
    client->show();

  installColormap(false);

  delete client;

  sbdbg<<"ShyneboxWindow::restore: remap = "<<remap<<"\n";
  sbdbg<<"("<<__FUNCTION__<<"): numClients() = "<<numClients()<<"\n";

  if (numClients() == 0)
    delete this;
} // restore

void ShyneboxWindow::restore(bool remap) {
  if (numClients() == 0)
    return;

  sbdbg<<"restore("<<remap<<")\n";

  // deleting winClient removes it from the clientList
  while (!clientList().empty() )
    restore(clientList().back(), remap);
}

bool ShyneboxWindow::isVisible() const {
  return frame().isVisible();
}

tk::SbWindow &ShyneboxWindow::sbWindow() {
  return frame().window();
}

const tk::SbWindow &ShyneboxWindow::sbWindow() const {
  return frame().window();
}

SbMenu &ShyneboxWindow::menu() {
  return screen().windowMenu();
}

bool ShyneboxWindow::acceptsFocus() const {
  return (m_client ? m_client->acceptsFocus() : false);
}

bool ShyneboxWindow::isModal() const {
  return (m_client ? m_client->isModal() : true);
}

const tk::PixmapWithMask &ShyneboxWindow::icon() const {
  return (m_client ? m_client->icon() : m_icon);
}

const SbMenu &ShyneboxWindow::menu() const {
  return screen().windowMenu();
}

unsigned int ShyneboxWindow::titlebarHeight() const {
  return frame().titlebarHeight();
}

Window ShyneboxWindow::clientWindow() const  {
  if (m_client == 0)
    return 0;
  return m_client->window();
}

const tk::BiDiString& ShyneboxWindow::title() const {
  return (m_client ? m_client->title() : m_title);
}

const tk::SbString& ShyneboxWindow::getWMClassName() const {
  return (m_client ? m_client->getWMClassName() : getWMClassName() );
}

const tk::SbString& ShyneboxWindow::getWMClassClass() const {
  return (m_client ? m_client->getWMClassClass() : getWMClassClass() );
}

tk::SbString ShyneboxWindow::getWMRole() const {
  return (m_client ? m_client->getWMRole() : "ShyneboxWindow");
}

long ShyneboxWindow::getCardinalProperty(Atom prop,bool*exists) const {
  return (m_client ? m_client->getCardinalProperty(prop,exists) : Focusable::getCardinalProperty(prop,exists) );
}

tk::SbString ShyneboxWindow::getTextProperty(Atom prop,bool*exists) const {
  return m_client ? m_client->getTextProperty(prop,exists) : "";
}

bool ShyneboxWindow::isTransient() const {
  return (m_client && m_client->isTransient() );
}

int ShyneboxWindow::initialState() const { return m_client->initial_state; }

// don't allow small windows, negative sizes, and obey window size hints
void ShyneboxWindow::fixSize() {
  // m_last_resize_w / m_last_resize_h could be negative
  // due to user interactions. check here and limit
  unsigned int w = 1;
  unsigned int h = 1;
  if (m_last_resize_w > 0)
    w = m_last_resize_w;
  if (m_last_resize_h > 0)
    h = m_last_resize_h;

  // ends up snapping to WM_SIZE_HINTS / WM_NORMAL_HINTS
  // e.g. urxvt
  frame().applySizeHints(w, h);

  m_last_resize_w = w;
  m_last_resize_h = h;

  // move X if necessary
  if (m_resize_corner == LEFTTOP || m_resize_corner == LEFTBOTTOM
      || m_resize_corner == LEFT)
    m_last_resize_x = frame().x() + frame().width() - m_last_resize_w;

  if (m_resize_corner == LEFTTOP || m_resize_corner == RIGHTTOP
      || m_resize_corner == TOP)
    m_last_resize_y = frame().y() + frame().height() - m_last_resize_h;
} // fixSize

void ShyneboxWindow::moveResizeClient(WinClient &client) {
  client.moveResize(frame().clientArea().x(), frame().clientArea().y(),
                    frame().clientArea().width(),
                    frame().clientArea().height() );
  client.sendConfigureNotify(frame().x() + frame().clientArea().x()
                                   + frame().window().borderWidth(),
                             frame().y() + frame().clientArea().y()
                                   + frame().window().borderWidth(),
                             frame().clientArea().width(),
                             frame().clientArea().height() );
}

void ShyneboxWindow::sendConfigureNotify() {
  // Send event telling where the root position of the client window is.
  // (ie frame pos + client pos inside the frame = send pos)
  for (auto client_it : m_clientlist)
    moveResizeClient(*client_it);
}


void ShyneboxWindow::close() {
  if (WindowCmd<void>::window() == this && WindowCmd<void>::client() )
    WindowCmd<void>::client()->sendClose(false);
  else if (m_client)
    m_client->sendClose(false);
}

void ShyneboxWindow::kill() {
  if (WindowCmd<void>::window() == this && WindowCmd<void>::client() )
    WindowCmd<void>::client()->sendClose(true);
  else if (m_client)
    m_client->sendClose(true);
}

void ShyneboxWindow::updateButtons() {
  size_t lr;   // left/right tracker for auto vec-list
  size_t bidx; // button index

  // check if we need to update our buttons
  if (screen().titlebar_left().size() == (size_t)m_titlebar_but_sizes[0]
      && screen().titlebar_right().size() == (size_t)m_titlebar_but_sizes[1] )
    return;

  frame().removeAllButtons();

  #define BtnCmd tk::SimpleCommand<ShyneboxWindow>

  // for left+right buttons, add
  for (auto &buttons : {screen().titlebar_left(), screen().titlebar_right() } ) {
    if (buttons == screen().titlebar_left() )
      lr = 0;
    else // right
      lr = 1;

    m_titlebar_but_sizes[lr] = buttons.size();

    for (bidx = 0; bidx < buttons.size(); ++bidx) {
      WinButton* btn = 0;

      switch (buttons[bidx]) {
      case WinButton::MINIMIZE:
          if (isIconifiable() && (m_state.deco_mask & WindowState::DECORM_ICONIFY) ) {
              BtnCmd *iconify_cmd = new BtnCmd(*this, &ShyneboxWindow::iconify);
              btn = makeButton(*this, m_button_theme, buttons[bidx]);
              btn->setOnClick(*iconify_cmd);
          }
          break;
      case WinButton::MAXIMIZE:
          if (isMaximizable() && (m_state.deco_mask & WindowState::DECORM_MAXIMIZE) ) {
              BtnCmd *maximize_cmd = new BtnCmd(*this, &ShyneboxWindow::maximizeFull);
              BtnCmd *maximize_vert_cmd = new BtnCmd(*this, &ShyneboxWindow::maximizeVertical);
              BtnCmd *maximize_horiz_cmd = new BtnCmd(*this, &ShyneboxWindow::maximizeHorizontal);
              btn = makeButton(*this, m_button_theme, buttons[bidx]);
              btn->setOnClick(*maximize_cmd, 1);
              btn->setOnClick(*maximize_horiz_cmd, 3);
              btn->setOnClick(*maximize_vert_cmd, 2);
          }
          break;
      case WinButton::CLOSE:
          if (m_client->isClosable() && (m_state.deco_mask & WindowState::DECORM_CLOSE) ) {
              BtnCmd *close_cmd = new BtnCmd(*this, &ShyneboxWindow::close);
              btn = makeButton(*this, m_button_theme, buttons[bidx]);
              btn->setOnClick(*close_cmd);
          }
          break;
      case WinButton::STICK:
          if (m_state.deco_mask & WindowState::DECORM_STICKY) {
              BtnCmd *stick_cmd = new BtnCmd(*this, &ShyneboxWindow::stick);
              btn = makeButton(*this, m_button_theme, buttons[bidx]);
              btn->setOnClick(*stick_cmd);
          }
          break;
      case WinButton::SHADE:
          if (m_state.deco_mask & WindowState::DECORM_SHADE) {
              BtnCmd *shade_cmd = new BtnCmd(*this, &ShyneboxWindow::shade);
              btn = makeButton(*this, m_button_theme, buttons[bidx]);
              btn->setOnClick(*shade_cmd);
          }
          break;
      case WinButton::MENUICON:
          if (m_state.deco_mask & WindowState::DECORM_MENU) {
              BtnCmd *show_menu_cmd = new BtnCmd(*this, &ShyneboxWindow::popupMenu);
              btn = makeButton(*this, m_button_theme, buttons[bidx]);
              btn->setOnClick(*show_menu_cmd);
          }
          break;
      } // switch(buttons[bidx])

      if (btn != 0) {
        btn->show();
        if (lr == 0)
          frame().addLeftButton(btn);
        else
          frame().addRightButton(btn);
      }
    } // for button sizes
  } // for left/right buttons
  #undef BtnCmd
} // updateButtons

// grab pointer and increase counter.
// we need this to count grab pointers,
// especially at startup, where we can drag/resize while starting
// and causing it to send events to windows later on and make
// two different windows do grab pointer which only one window
// should do at the time
void ShyneboxWindow::grabPointer(Window grab_window,
                                Bool owner_events,
                                unsigned int event_mask,
                                int pointer_mode, int keyboard_mode,
                                Window confine_to,
                                Cursor cursor,
                                Time time) {
  XGrabPointer(tk::App::instance()->display(),
               grab_window,
               owner_events,
               event_mask,
               pointer_mode, keyboard_mode,
               confine_to,
               cursor,
               time);
  s_num_grabs++;
}

// ungrab and decrease counter
void ShyneboxWindow::ungrabPointer(Time time) {
  XUngrabPointer(tk::App::instance()->display(), time);
  s_num_grabs--;
  if (s_num_grabs < 0)
    s_num_grabs = 0;
}

void ShyneboxWindow::associateClient(WinClient &client) {
  IconButton *btn = new IconButton(frame().tabcontainer(),
          frame().theme().focusedTheme()->iconbarTheme(),
          frame().theme().unfocusedTheme()->iconbarTheme(), client);
  frame().createTab(*btn);

  btn->setTextPadding(Shynebox::instance()->getTabsPadding() );
  btn->setPixmap(screen().getTabsUsePixmap() );

  m_labelbuttons[&client] = btn;

  tk::EventManager &evm = *tk::EventManager::instance();

  evm.add(*this, btn->window() ); // we take care of button events for this
  evm.add(*this, client.window() );

  client.setShyneboxWindow(this);
}

// This is for Remember and CurrentWindowCmd::MoveTo
ShyneboxWindow::ReferenceCorner ShyneboxWindow::getCorner(string str) {
  str = tk::StringUtil::toLower(str);
  if (str == "top" || str == "upper" || str == "topcenter")
    return TOP;
  if (str == "righttop" || str == "topright" || str == "upperright")
    return RIGHTTOP;
  if (str == "left" || str == "leftcenter")
    return LEFT;
  if (str == "center" || str == "wincenter")
    return CENTER;
  if (str == "right" || str == "rightcenter")
    return RIGHT;
  if (str == "leftbottom" || str == "bottomleft" || str == "lowerleft")
    return LEFTBOTTOM;
  if (str == "bottom" || str == "lower" || str == "bottomcenter")
    return BOTTOM;
  if (str == "rightbottom" || str == "bottomright" || str == "lowerright")
    return RIGHTBOTTOM;

  return LEFTTOP; // default
}

void ShyneboxWindow::translateXCoords(int &x, ReferenceCorner dir) const {
  int head = getOnHead(),
      bw = 2 * frame().window().borderWidth(),
      left = screen().maxLeft(head),
      right = screen().maxRight(head),
      w = width();

  if (dir == LEFTTOP || dir == LEFT || dir == LEFTBOTTOM)
    x += left;
  else if (dir == RIGHTTOP || dir == RIGHT || dir == RIGHTBOTTOM)
    x = right - w - bw - x;
  else if (dir == TOP || dir == CENTER || dir == BOTTOM)
    x += (left + right - w - bw)/2;
}

void ShyneboxWindow::translateYCoords(int &y, ReferenceCorner dir) const {
  int head = getOnHead(),
      bw = 2 * frame().window().borderWidth(),
      top = screen().maxTop(head),
      bottom = screen().maxBottom(head),
      h = height();

  if (dir == LEFTTOP || dir == TOP || dir == RIGHTTOP)
    y += top;
  else if (dir == LEFTBOTTOM || dir == BOTTOM || dir == RIGHTBOTTOM)
    y = bottom - h - bw - y;
  else if (dir == LEFT || dir == CENTER || dir == RIGHT)
    y += (top + bottom - h - bw)/2;
}

void ShyneboxWindow::translateCoords(int &x, int &y, ReferenceCorner dir) const {
  translateXCoords(x, dir);
  translateYCoords(y, dir);
}

int ShyneboxWindow::getOnHead() const {
  return screen().getHead(sbWindow() );
}

void ShyneboxWindow::setOnHead(int head) {
  if (head > 0 && head <= screen().numHeads() ) {
    int cur = getOnHead();
    if (head == cur)
      return;
    bool placed = m_placed; // save placement, move<moveresize<placewindow sets
    // alternatives:
    // - calc by percent of top-left (head to win)
    // - use screen's fitToHead()
    //   then use that to move()
    placeWindow(head);
    m_placed = placed;

    // if Head has been changed we want it to redraw by current state
    if (m_state.maximized || m_state.fullscreen) {
      frame().applyState();
      frameExtentChanged();
      Shynebox::instance()->windowStateChanged(*this);
    }
  }
} // setOnHead

void ShyneboxWindow::placeWindow(int head) {
  int new_x, new_y;
  // we ignore the return value,
  // the screen placement strategy is guaranteed to succeed.
  screen().placementStrategy().placeWindow(*this, head, new_x, new_y);
  m_state.saveGeometry(new_x, new_y, frame().width(), frame().height(), true);
  move(new_x, new_y);
}

void ShyneboxWindow::setWindowType(WindowState::WindowType type) {
  m_state.type = type;
  switch (type) {
  case WindowState::TYPE_DOCK:
      /* From Extended Window Manager Hints, draft 1.3:
       *
       * _NET_WM_WINDOW_TYPE_DOCK indicates a dock or panel feature.
       * Typically a Window Manager would keep such windows on top
       * of all other windows.
       *
       */
      setFocusHidden(true);
      setIconHidden(true);
      setFocusNew(false);
      setMouseFocus(false);
      setClickFocus(false);
      setDecorationMask(WindowState::DECOR_NONE);
      moveToLayer(RLEnum::DOCK);
      break;
  case WindowState::TYPE_DESKTOP:
      /*
       * _NET_WM_WINDOW_TYPE_DESKTOP indicates a "false desktop" window
       * We let it be the size it wants, but it gets no decoration,
       * is hidden in the toolbar and window cycling list, plus
       * windows don't tab with it and is right on the bottom.
       */
      setFocusHidden(true);
      setIconHidden(true);
      setFocusNew(false);
      setMouseFocus(false);
      moveToLayer(RLEnum::DESKTOP);
      setDecorationMask(WindowState::DECOR_NONE);
      setTabable(false);
      setMovable(false);
      setResizable(false);
      setStuck(true);
      break;
  case WindowState::TYPE_SPLASH:
      /*
       * _NET_WM_WINDOW_TYPE_SPLASH indicates that the
       * window is a splash screen displayed as an application
       * is starting up.
       */
      setDecorationMask(WindowState::DECOR_NONE);
      setFocusHidden(true);
      setIconHidden(true);
      setFocusNew(false);
      setMouseFocus(false);
      setClickFocus(false);
      setMovable(false);
      break;
  case WindowState::TYPE_DIALOG:
      setTabable(false);
      break;
  case WindowState::TYPE_MENU:
  case WindowState::TYPE_TOOLBAR:
      /*
       * _NET_WM_WINDOW_TYPE_TOOLBAR and _NET_WM_WINDOW_TYPE_MENU
       * indicate toolbar and pinnable menu windows, respectively
       * (i.e. toolbars and menus "torn off" from the main
       * application). Windows of this type may set the
       * WM_TRANSIENT_FOR hint indicating the main application window.
       */
      setDecorationMask(WindowState::DECOR_TOOL);
      setIconHidden(true);
      moveToLayer(RLEnum::ABOVE_DOCK);
      break;
  case WindowState::TYPE_NORMAL:
  default:
      break;
  }

  /*
   * NOT YET IMPLEMENTED:
   *   _NET_WM_WINDOW_TYPE_UTILITY
   */
} // setWindowType

// Copyright (c) 2023 Shynebox - zlice
//
// Copyright (c) 2001 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
//
// Window.cc for Blackbox - an X11 Window manager
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
