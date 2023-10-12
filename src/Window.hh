// Window.hh for Shynebox Window Manager

/*
  The main class that drives window behavior and creation.
  Creates a frame. Adds WinClients (actual X windows) to self
  and has frame add them as tabs. If Screen is the brain, then
  this is the heart of the Window Manager.
*/

#ifndef WINDOW_HH
#define WINDOW_HH

#include "SbWinFrame.hh"
#include "Focusable.hh"
#include "FocusableTheme.hh"
#include "FocusControl.hh"
#include "WinButton.hh"

#include "tk/Timer.hh"
#include "tk/SbTime.hh"
#include "tk/EventHandler.hh"
#include "tk/LayerItem.hh"

#include <vector>
#include <map>
#include <inttypes.h>

class WinClient;
class SbWinFrameTheme;
class BScreen;
class SbMenu;

namespace tk {
class MenuTheme;
class ImageControl;
class Layer;
}

namespace Focus {
  enum {
      NoProtection = 0,
      Gain = 1,
      Refuse = 2,
      Lock = 4,
      Deny = 8
  };
  typedef unsigned int Protection;
}

// Creates the window frame and handles any window event for it
class ShyneboxWindow: public Focusable,
                      public tk::EventHandler {
public:
  // Motif wm Hints
  enum {
      MwmHintsFunctions   = (1l << 0), // use motif wm functions
      MwmHintsDecorations = (1l << 1)  // use motif wm decorations
  };

  // Motif wm functions
  enum MwmFunc{
      MwmFuncAll          = (1l << 0), // all motif wm functions
      MwmFuncResize       = (1l << 1), // resize
      MwmFuncMove         = (1l << 2), // move
      MwmFuncIconify      = (1l << 3), // iconify
      MwmFuncMaximize     = (1l << 4), // maximize
      MwmFuncClose        = (1l << 5)  // close
  };

  // Motif wm decorations
  enum MwmDecor {
      MwmDecorAll         = (1l << 0), // all decorations
      MwmDecorBorder      = (1l << 1), // border
      MwmDecorHandle      = (1l << 2), // handle
      MwmDecorTitle       = (1l << 3), // title
      MwmDecorMenu        = (1l << 4), // menu
      MwmDecorIconify     = (1l << 5), // iconify
      MwmDecorMaximize    = (1l << 6)  // maximize
  };

  // Different resize modes when resizing a window
  // used in CurrentWindowCmd
  enum ResizeModel {
      CENTERRESIZE,                     // resizes from center
      TOPLEFTRESIZE,                    // resizes top left corner
      TOPRESIZE,                        // resizes top edge
      TOPRIGHTRESIZE,                   // resizes top right corner
      LEFTRESIZE,                       // resizes left edge
      RIGHTRESIZE,                      // resizes right edge
      BOTTOMLEFTRESIZE,                 // resizes bottom left corner
      BOTTOMRESIZE,                     // resizes bottom edge
      BOTTOMRIGHTRESIZE,                // resizes bottom right corner - default
      EDGEORCORNERRESIZE,               // resizes nearest edge or corner
  };

   // for moves and resizes
   // Remember and CurrentWindowCmd::MoveTo
   // have different default than resize
   enum ReferenceCorner {
       LEFTTOP      = 0, // default
       TOP          = 1,
       RIGHTTOP     = 2,
       RIGHT        = 3,
       RIGHTBOTTOM  = 4,
       BOTTOM       = 5,
       LEFTBOTTOM   = 6,
       LEFT         = 7,
       CENTER       = 8
  };

  typedef std::list<WinClient *> ClientList;

  // create a window from a client
  ShyneboxWindow(WinClient &client);
  virtual ~ShyneboxWindow();

  // check if X or Y are outside of view and move inside view
  void fitToScreen();
  // attach client to our client list and remove it from old window
  void attachClient(WinClient &client, int x=-1, int y=-1);
  // detach client (remove it from list) and create a new window for it
  bool detachClient(WinClient &client);
  // detach current working client if we have more than one
  void detachCurrentClient();
  // remove client from client list
  bool removeClient(WinClient &client);
  // set new current client and raise it
  bool setCurrentClient(WinClient &client, bool setinput = true);
  WinClient *findClient(Window win);
  void nextClient();
  void prevClient();
  void moveClientLeft();
  void moveClientRight();
  void moveClientRightOf(WinClient &win, WinClient &dest);
  void moveClientLeftOf(WinClient &win, WinClient &dest);
  void moveClientTo(WinClient &win, int x, int y);
  // try to focus self, clients, etc.
  // returns true if attempt assumed succesful (see comments in c)
  bool focus();
  bool focusRequestFromClient(WinClient &from);

  void raiseAndFocus() { raise(); focus(); }
  // sets the internal focus flag
  void setFocusFlag(bool flag);
  // make this window visible
  void show();
  // hide window
  void hide(bool interrupt_moving = true);
  // iconify window - aka minimize
  void iconify();
  // pop-up and raise to top of current layer by default
  void deiconify(bool do_raise = true);

  void close();
  void kill();
  void setFullscreen(bool flag);
  // _actually_ sets the maximized state
  void setMaximizedState(int type);
  // set maximize state (wrapper)
  void maximize(int type = WindowState::MAX_FULL);
  void maximizeHorizontal();
  void maximizeVertical();
  void maximizeFull();
  // disables maximization, without restoring the old size
  void disableMaximization();

  // toggles shade
  void shade();
  // wrappers to shade toggle
  void shadeOn();
  void shadeOff();
  void setShaded(bool val);
  // toggles sticky
  void stick();
  // sets stuck state
  void setStuck(bool val);
  // toggles iconic
  void toggleIconic();
  // sets iconic state
  void setIconic(bool val);
  // override XVisibility if in view
  void setInView(bool in_v);
  // gets XVisibility if in view
  bool isInView() const;
  // raise / lower within current layer
  void raise();
  void lower();
  void tempRaise();
  // moves the window to a new layer
  void moveToLayer(int layernum, bool force = false);
  int getOnHead() const;
  void setOnHead(int head);
  // sets the window focus hidden state
  void placeWindow(int head);
  void setFocusHidden(bool value);
  // sets the window icon hidden state
  void setIconHidden(bool value);
  // sets whether or not the window normally gets focus when mapped
  void setFocusNew(bool value) {
    if (value)
      m_focus_protection = (m_focus_protection & ~Focus::Refuse) | Focus::Gain;
    else
      m_focus_protection = (m_focus_protection & ~Focus::Gain) | Focus::Refuse;
  }
  // sets how to protect the focus on or against this window
  void setFocusProtection(Focus::Protection value) { m_focus_protection = value; }
  // sets whether or not the window gets focused with mouse
  void setMouseFocus(bool value) { m_mouse_focus = value; }
  // sets whether or not the window gets focused with click
  void setClickFocus(bool value) { m_click_focus = value; }
  void reconfigure();


  void installColormap(bool);
  void restore(WinClient *client, bool remap);
  void restore(bool remap);
  // move frame to x, y
  void move(int x, int y);
  // resize frame to width, height
  void resize(unsigned int width, unsigned int height);
  // move and resize frame to pox x,y and size width, height
  void moveResize(int x, int y, unsigned int width, unsigned int height, bool send_event = false);
  // move to pos x,y and resize client window to size width, height
  void moveResizeForClient(int x, int y, unsigned int width, unsigned int height,
                           int gravity = ForgetGravity, unsigned int client_bw = 0);
  // get max size from all clients using window hints
  void getMaxSize(unsigned int* width, unsigned int* height) const;
  void setWorkspace(int n);
  // update enabled 'functions' : resize, move, maximize, etc
  void updateFunctions();
  void showMenu(int mx, int my);
  void popupMenu(int x, int y);
  // popup menu on last button press position
  void popupMenu();

  // event handlers
  void handleEvent(XEvent &event);
  void keyPressEvent(XKeyEvent &ke);
  void buttonPressEvent(XButtonEvent &be);
  void buttonReleaseEvent(XButtonEvent &be);
  void motionNotifyEvent(XMotionEvent &me);
  void destroyNotifyEvent(XDestroyWindowEvent &dwe);
  void mapRequestEvent(XMapRequestEvent &mre);
  void mapNotifyEvent(XMapEvent &mapev);
  void unmapNotifyEvent(XUnmapEvent &unmapev);
  void exposeEvent(XExposeEvent &ee);
  void configureRequestEvent(XConfigureRequestEvent &ce);
  void propertyNotifyEvent(WinClient &client, Atom a);
  void enterNotifyEvent(XCrossingEvent &ev);
  void leaveNotifyEvent(XCrossingEvent &ev);

  void applyDecorations();
  void toggleDecoration();

  unsigned int decorationMask() const;
  void setDecorationMask(unsigned int mask, bool apply = true);
  // Start moving process, grabs the pointer and draws move rectangle
  void startMoving(int x, int y);
  void stopMoving(bool interrupted = false);
  // Start resizing process, grabs the pointer and draws move rectangle
  void startResizing(int x, int y, ReferenceCorner dir);
  void stopResizing(bool interrupted = false);
  // Start tabbing process, grabs the pointer and draws move rectangle
  void startTabbing(const XButtonEvent &be);
  // determine which edge or corner to resize
  ReferenceCorner getResizeDirection(int x, int y, ResizeModel model, int corner_size_px, int corner_size_pc) const;
  // determine the reference corner from a string
  static ReferenceCorner getCorner(std::string str);
  // convert to coordinates on the root window
  void translateXCoords(int &x, ReferenceCorner dir = LEFTTOP) const;
  void translateYCoords(int &y, ReferenceCorner dir = LEFTTOP) const;
  void translateCoords(int &x, int &y, ReferenceCorner dir = LEFTTOP) const;

  // whether this window can be tabbed with other windows,
  // and others tabbed with it
  void setTabable(bool tabable) { functions.tabable = tabable; }
  bool isTabable() const { return functions.tabable; }
  void setMovable(bool movable) { functions.move = movable; }
  void setResizable(bool resizable) { functions.resize = resizable; }

  bool isFocusHidden() const { return m_state.focus_hidden; }
  bool isIconHidden() const { return m_state.icon_hidden; }
  bool isManaged() const { return m_initialized; }
  bool isVisible() const;
  bool isIconic() const { return m_state.iconic; }
  bool isShaded() const { return m_state.shaded; }
  bool isFullscreen() const { return m_state.fullscreen; }
  bool isMaximized() const { return m_state.isMaximized(); }
  bool isMaximizedVert() const { return m_state.isMaximizedVert(); }
  bool isMaximizedHorz() const { return m_state.isMaximizedHorz(); }
  int maximizedState() const { return m_state.maximized; }
  bool isIconifiable() const { return functions.iconify; }
  bool isMaximizable() const { return functions.maximize; }
  bool isResizable() const { return functions.resize; }
  bool isClosable() const { return functions.close; }
  bool isMoveable() const { return functions.move; }
  bool isStuck() const { return m_state.stuck; }
  bool isFocusNew() const;
  Focus::Protection focusProtection() const { return m_focus_protection; }
  bool hasTitlebar() const { return decorations.titlebar; }
  bool isMoving() const { return moving; }
  bool isResizing() const { return resizing; }
  bool isGroupable() const;
  int numClients() const { return m_clientlist.size(); }
  bool empty() const { return m_clientlist.empty(); }
  ClientList &clientList() { return m_clientlist; }
  const ClientList &clientList() const { return m_clientlist; }
  WinClient &winClient() { return *m_client; }
  const WinClient &winClient() const { return *m_client; }

  WinClient* winClientOfLabelButtonWindow(Window w);

  bool isTyping() const;

  const tk::LayerItem &layerItem() const { return m_frame.layerItem(); }
  tk::LayerItem &layerItem() { return m_frame.layerItem(); }

  Window clientWindow() const;

  tk::SbWindow &sbWindow();
  const tk::SbWindow &sbWindow() const;

  SbMenu &menu();
  const SbMenu &menu() const;

  const tk::SbWindow &parent() const { return m_parent; }
  tk::SbWindow &parent() { return m_parent; }

  bool acceptsFocus() const;
  bool isModal() const;
  const tk::PixmapWithMask &icon() const;
  const tk::BiDiString &title() const;
  const tk::SbString &getWMClassName() const;
  const tk::SbString &getWMClassClass() const;
  std::string getWMRole() const;
  long getCardinalProperty(Atom prop,bool*exists=NULL) const;
  tk::SbString getTextProperty(Atom prop,bool*exists=NULL) const;
  void setWindowType(WindowState::WindowType type);
  bool isTransient() const;

  int x() const { return frame().x(); }
  int y() const { return frame().y(); }
  unsigned int width() const { return frame().width(); }
  unsigned int height() const { return frame().height(); }

  int normalX() const { return m_state.x; }
  int normalY() const { return m_state.y; }
  unsigned int normalWidth() const { return m_state.width; }
  unsigned int normalHeight() const { return m_state.height; }

  int xOffset() const { return frame().xOffset(); }
  int yOffset() const { return frame().yOffset(); }
  int widthOffset() const { return frame().widthOffset(); }
  int heightOffset() const { return frame().heightOffset(); }

  unsigned int workspaceNumber() const { return m_workspace_number; }

  int layerNum() const { return m_state.layernum; }
  void setLayerNum(int layernum);

  unsigned int titlebarHeight() const;

  int initialState() const;

  SbWinFrame &frame() { return m_frame; }
  const SbWinFrame &frame() const { return m_frame; }

  bool oplock; // Used to help stop transient loops occurring by locking a window during certain operations

  // callback for title changes by clients
  void setTitle(const std::string &title, Focusable &client);
  void themeReconfigured();

private:
  void updateButtons();

  void updateClientLeftWindow();
  void grabButtons();

  // gets tab position
  ClientList::iterator getClientInsertPosition(int x, int y);
  // try to attach current attaching client to a window at pos x, y
  void attachTo(int x, int y, bool interrupted = false);

  bool getState();
  void updateMWMHintsFromClient(WinClient &client);
  void updateSizeHints();
  void associateClientWindow();

  void setState(unsigned long stateval, bool setting_up);
  // set the layer of a fullscreen window
  void setFullscreenLayer();

  // modifies left and top if snap is necessary
  void doSnapping(int &left, int &top, bool resize = false);
  // don't allow small windows, negative sizes, and obey window size hints
  void fixSize();
  void moveResizeClient(WinClient &client);
  // sends configurenotify to all clients
  void sendConfigureNotify();
  void updateResize() { moveResize(m_last_resize_x, m_last_resize_y, m_last_resize_w, m_last_resize_h); }

  static void grabPointer(Window grab_window,
                   Bool owner_events,
                   unsigned int event_mask,
                   int pointer_mode, int keyboard_mode,
                   Window confine_to,
                   Cursor cursor,
                   Time time);
  static void ungrabPointer(Time time);

  void associateClient(WinClient &client);
  // Called when workspace area on screen changed.
  void workspaceAreaChanged(BScreen &screen);
  void frameExtentChanged();

  uint64_t m_creation_time;
  uint64_t m_last_keypress_time;
  tk::Timer m_raise_timer,
            m_tab_activate_timer,
            m_resize_timer;

  // Window states - note: different than WindowState class
  bool moving, resizing, m_initialized;

  WinClient *m_attaching_tab;

  Display *display;

  int m_button_grab_x, m_button_grab_y, // handles last button press event for move
      m_last_resize_x, m_last_resize_y, // handles last button press event for resize
      m_last_move_x,   m_last_move_y,   // handles last pos for non opaque moving
      m_last_resize_h, m_last_resize_w, // handles height/width for resize
      resize_base_x, resize_base_y, resize_base_w, resize_base_h,
      m_last_action_button; // tracker so actions aren't interrupted

  unsigned int m_workspace_number;
  unsigned long m_current_state; // NormalState | IconicState | Withdrawn

  unsigned int m_old_decoration_mask;

  ClientList m_clientlist;
  WinClient *m_client = 0; // current client
  typedef std::map<WinClient *, IconButton *> Client2ButtonMap;
  Client2ButtonMap m_labelbuttons;
  tk::Timer m_reposLabels_timer;
  bool m_has_tooltip = false;

  SizeHints m_size_hint;
  struct {
      bool titlebar:1, handle:1, border:1, iconify:1,
          maximize:1, close:1, menu:1, sticky:1, shade:1, tab:1, enabled:1;
  } decorations;

  int m_titlebar_but_sizes[2] = { 0 }; // tracker for need_update
  bool m_toggled_decos;

  struct {
      bool resize:1, move:1, iconify:1, maximize:1, close:1, tabable:1;
  } functions;

  // if the window is normally focused when mapped
  // special focus permissions
  Focus::Protection m_focus_protection;
  // if the window is focused with EnterNotify
  bool m_mouse_focus = false;
  // not used with keys config to focus
  bool m_click_focus;  // if the window is focused by clicking
  int m_last_button_x, // last known x position of the mouse button
      m_last_button_y; // last known y position of the mouse button

  FocusableTheme<WinButtonTheme> m_button_theme;
  FocusableTheme<SbWinFrameTheme> m_theme;

  WindowState m_state;
  SbWinFrame m_frame; // the actual window frame

  bool m_placed; // determine whether or not we should place the window

  int m_old_layernum;

  tk::SbWindow &m_parent; // window on which we draw move/resize rectangle  (the "root window")

  ReferenceCorner m_resize_corner; // the current corner used while resizing

  static int s_num_grabs; // number of XGrabPointer's
};

#endif // WINDOW_HH

// Copyright (c) 2023 Shynebox - zlice
//
// Copyright (c) 2001 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
//
// Window.hh for Blackbox - an X11 Window manager
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
