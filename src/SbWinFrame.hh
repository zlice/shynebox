// SbWinFrame.hh for Shynebox Window Manager

/*
  The actual visible frame and tabs for windows.
  Creates and manages tabs. Handles 'client area' (WinClient, actual window view)
  Determines event 'context' for 'key' actions.
*/

#ifndef SBWINFRAME_HH
#define SBWINFRAME_HH

#include "WindowState.hh"

#include "tk/SbWindow.hh"
#include "tk/EventHandler.hh"
#include "tk/Color.hh"
#include "tk/TextButton.hh"
#include "tk/ButtonTrain.hh"
#include "tk/Timer.hh"

#include <vector>

class SbWinFrameTheme;
class BScreen;
class IconButton;
class Focusable;
template <class T> class FocusableTheme;

namespace tk {
class ImageControl;
class LayerItem;
template <class T> class Command;
class Shape;
class Texture;
}

class SbWinFrame:public tk::EventHandler {
public:
  enum TabMode { NOTSET = 0, INTERNAL = 1, EXTERNAL };

  // create a top level window - aka (multi-)client holder
  SbWinFrame(BScreen &screen, int client_depth, WindowState &state,
             FocusableTheme<SbWinFrameTheme> &theme);

  ~SbWinFrame();

  void hide();
  void show();
  bool isVisible() const { return m_visible; }

  void move(int x, int y);
  void resize(unsigned int width, unsigned int height);
  // resize client to specified size and resize frame to it
  void resizeForClient(unsigned int width, unsigned int height,
                       int win_gravity=ForgetGravity, unsigned int client_bw = 0);

  // for when there needs to be an atomic move+resize operation
  void moveResizeForClient(int x, int y,
                           unsigned int width, unsigned int height,
                           int win_gravity=ForgetGravity, unsigned int client_bw = 0,
                           bool move = true, bool resize = true);

  void moveResize(int x, int y,
                  unsigned int width, unsigned int height,
                  bool move = true, bool resize = true, bool force = false);

  // move without special effects (generally when dragging)
  void quietMoveResize(int x, int y,
                       unsigned int width, unsigned int height);

  void clearAll();

  // set focus/unfocus style
  void setFocus(bool newvalue);

  void setFocusTitle(const tk::BiDiString &str) { m_label.setText(str); }
  bool setTabMode(TabMode tabmode);
  void updateTabProperties() { alignTabs(); }

  // titlebar buttons are [left][label/tabs][right]
  void addLeftButton(tk::Button *btn);
  void addRightButton(tk::Button *btn);
  void removeAllButtons();
  void createTab(tk::Button &button);
  void removeTab(IconButton *id);
  void moveLabelButtonLeft(tk::TextButton &btn);
  void moveLabelButtonRight(tk::TextButton &btn);
  void moveLabelButtonLeftOf(tk::TextButton &btn, const tk::TextButton &dest);
  void moveLabelButtonRightOf(tk::TextButton &btn, const tk::TextButton &dest);
  // attach a client window for client area
  void setClientWindow(tk::SbWindow &win);
  void removeClient();

  // redirect events to another eventhandler
  void setEventHandler(tk::EventHandler &evh);
  void removeEventHandler();

  // EWMH requested size dimensions
  const SizeHints &sizeHints() const { return m_state.size_hints; }
  void setSizeHints(const SizeHints &hint) { m_state.size_hints = hint; }
  void applySizeHints(unsigned int &width, unsigned int &height,
                      bool maximizing = false) const;

  void displaySize(unsigned int width, unsigned int height) const;
  void setDecorationMask(unsigned int mask) { m_state.deco_mask = mask; }
  void applyDecorations(bool do_move = true);
  void applyState();

  // this function translates its arguments according to win_gravity
  // if win_gravity is negative, it does an inverse translation
  void gravityTranslate(int &x, int &y, int win_gravity,
                        unsigned int client_bw);
  void setActiveGravity(int gravity, unsigned int orig_client_bw) {
    m_state.size_hints.win_gravity = gravity;
    m_active_orig_client_bw = orig_client_bw;
  }

  // event handlers
  void exposeEvent(XExposeEvent &event);
  void configureNotifyEvent(XConfigureEvent &event);
  void handleEvent(XEvent &event);

  void reconfigure();
  void setShapingClient(tk::SbWindow *win, bool always_update);
  void updateShape();

  void resetLock();
  void unlockSig();

  int x() const { return m_window.x(); }
  int y() const { return m_window.y(); }
  unsigned int width() const { return m_window.width(); }
  unsigned int height() const { return m_window.height(); }

  // extra bits for tabs
  int xOffset() const;
  int yOffset() const;
  int widthOffset() const;
  int heightOffset() const;

  const tk::SbWindow &window() const { return m_window; }
  tk::SbWindow &window() { return m_window; }
  const tk::SbWindow &titlebar() const { return m_titlebar; }
  tk::SbWindow &titlebar() { return m_titlebar; }
  const tk::SbWindow &label() const { return m_label; }
  tk::SbWindow &label() { return m_label; }

  const tk::ButtonTrain &tabcontainer() const { return m_tab_container; }
  tk::ButtonTrain &tabcontainer() { return m_tab_container; }

  const tk::SbWindow &clientArea() const { return m_clientarea; }
  tk::SbWindow &clientArea() { return m_clientarea; }
  const tk::SbWindow &handle() const { return m_handle; }
  tk::SbWindow &handle() { return m_handle; }
  const tk::SbWindow &gripLeft() const { return m_grip_left; }
  tk::SbWindow &gripLeft() { return m_grip_left; }
  const tk::SbWindow &gripRight() const { return m_grip_right; }
  tk::SbWindow &gripRight() { return m_grip_right; }
  bool focused() const { return m_state.focused; }
  FocusableTheme<SbWinFrameTheme> &theme() const { return m_theme; }
  unsigned int titlebarHeight() const { return (m_use_titlebar?m_titlebar.height()+m_titlebar.borderWidth():0); }
  unsigned int handleHeight() const { return (m_use_handle?m_handle.height()+m_handle.borderWidth():0); }
  unsigned int buttonHeight() const;
  bool externalTabMode() const { return m_tabmode == EXTERNAL && m_use_tabs; }

  const tk::LayerItem &layerItem() const { return *m_layeritem; }
  tk::LayerItem &layerItem() { return *m_layeritem; }

  bool insideTitlebar(Window win) const;

  // context of 'keys' actions: titlebar, border, window, etc
  int getContext(Window win, int x=0, int y=0, int last_x=0, int last_y=0, bool doBorders=false);

private:
  void redrawTitlebar();

  // reposition titlebar items
  void reconfigureTitlebar();

  void renderAll();
  void renderTitlebar();
  void renderHandles();
  void renderTabs(); // and buttons
  void renderButtons();

  // these return true/false for if something changed
  bool hideTitlebar();
  bool showTitlebar();
  bool hideTabs();
  bool showTabs();
  bool hideHandle();
  bool showHandle();
  bool setBorderWidth(bool do_move = true);

  // check which corners should be rounded
  int getShape() const;

  // apply pixmaps depending on focus
  void applyAll();
  void applyTitlebar();
  void applyHandles();
  void applyTabs(); // and buttons
  void applyButtons();
  // initiate inserted button for current theme
  void applyButton(tk::Button &btn);
  void alignTabs();

  // initiate some common variables and themes
  void init();

  BScreen &m_screen;

  FocusableTheme<SbWinFrameTheme> &m_theme;
  tk::ImageControl &m_imagectrl;
  WindowState &m_state;

  // the sub boxes that make up a frame
  tk::SbWindow m_window; // base window that holds each decorations (ie titlebar, handles)
  // order for layer item is important iirc, leave here
  tk::LayerItem *m_layeritem = 0;

  tk::SbWindow    m_titlebar;      // titlebar
  tk::ButtonTrain m_tab_container; // holds tabs
  tk::TextButton  m_label;         // holds title if tabs are external
  tk::SbWindow    m_handle;        // handle between grips
  tk::SbWindow    m_grip_right,    // right grip
                  m_grip_left;     // left grip
  tk::SbWindow    m_clientarea;    // where the client window sits inside the frame

  typedef std::vector<tk::Button *> ButtonList;
  ButtonList m_buttons_left,  // buttons on the left of titlebar
             m_buttons_right; // buttons on the right of titlebar
  int m_bevel;                // bevel between titlebar items and titlebar
  bool m_use_titlebar;        // if we should use titlebar
  bool m_use_tabs;            // if we should use tabs (turns them off in external mode only)
  bool m_use_handle;          // if we should use handle
  bool m_visible;             // if we are currently showing
  // ^Note: m_visible is not linked to ShyneboxWindow::isIconic()

  // Faces are pixmaps and colors for rendering
  // 0-unfocus, 1-focus
  struct Face { Pixmap pm[2]; tk::Color color[2]; };
  // 0-unfocus, 1-focus, 2-pressed
  struct BtnFace { Pixmap pm[3]; tk::Color color[3]; };

  Face m_title_face;
  Face m_label_face;
  Face m_tabcontainer_face;
  Face m_handle_face;
  Face m_grip_face;
  BtnFace m_button_face;

  TabMode m_tabmode;

  unsigned int m_active_orig_client_bw;

  bool m_need_render;
  int m_button_size; // size for all titlebar buttons

  tk::Shape *m_shape = 0;
};

#endif // SBWINFRAME_HH

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
