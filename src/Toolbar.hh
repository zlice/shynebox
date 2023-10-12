// Toolbar.hh for Shynebox Window Manager

/*
  The area where Systray, Iconbar, Clock, Workspace name tag,
  custom buttons and spacers are located.
  Placed along edge of total screen space or a single head (monitor).
  Creates menus to put in ConfigMenu.
  Can request Struts to reserve screen space for itself.

  Only a SINGLE Toolbar exist per Screen for the time being.
  I have seen old online threads where people wanted multiple toolbars for multi-head
  setup, which is possible, but will take a performance hit with Iconbar
  (which I imagine is the main use case for multi-toolbar).

  Has some similar duplicate logic also in ButtonTrain (see rearrangeItems() )
*/

#ifndef TOOLBAR_HH
#define TOOLBAR_HH

#include "ToolbarTheme.hh"
#include "LayerMenu.hh"
#include "ToolFactory.hh"
#include "ToolTheme.hh"

#include "tk/Timer.hh"
#include "tk/LayerItem.hh"
#include "tk/EventHandler.hh"
#include "tk/SbWindow.hh"

class BScreen;
class Strut;
class SbMenu;
class ToolbarItem;

namespace tk {
class ImageControl;
class MacroCommand;
class Shape;
class TextButton;
}

class Toolbar: public tk::EventHandler,
               public LayerObject {
public:
  Toolbar(BScreen &screen, tk::Layer &layer, size_t width = 200);

  virtual ~Toolbar();

  void raise();
  void lower();
  void updateVisibleState();
  void toggleHidden();
  void toggleAboveDock();
  void moveToLayer(int layernum);

  // eventhandlers
  void enterNotifyEvent(XCrossingEvent &ce);
  void leaveNotifyEvent(XCrossingEvent &ce);
  void exposeEvent(XExposeEvent &ee);

  void relayout();
  void reconfigure();
  void setPlacement(tk::ToolbarPlacement_e where);
  void rearrangeItems();

  const tk::Menu &menu() const { return m_toolbarmenu; }
  tk::Menu &menu() { return m_toolbarmenu; }
  // used for access in the ConfigMenu
  tk::Menu &placementMenu() { return m_placementmenu; }

  tk::Menu &layerMenu() { return m_layermenu; }
  const tk::Menu &layerMenu() const { return m_layermenu; }

  BScreen &screen() { return m_screen; }
  const BScreen &screen() const { return m_screen; }

  // fake frame window
  const tk::SbWindow &window() const { return frame.window; }
  unsigned int width() const { return frame.window.width(); }
  unsigned int height() const { return frame.window.height(); }
  int x() const { return isHidden() ? frame.x_hidden : frame.x; }
  int y() const { return isHidden() ? frame.y_hidden : frame.y; }
  tk::ToolbarPlacement_e placement() const { return m_rc_placement; }

  int layerNumber() const { return const_cast<tk::LayerItem &>(m_layeritem).getLayerNum(); }

  // config items
  bool isHidden() const { return m_hidden; }
  // do we auto hide the toolbar?
  bool doAutoHide() const { return m_rc_auto_hide; }
  int getOnHead() const { return m_rc_on_head; }

  ToolFactory m_tool_factory;

private:
  void deleteItems();

  void setupMenus(bool skip_new_placement=false);
  void clearStrut();
  void updateStrut();

  void updateCrossingState();

  bool m_hidden;

  // fake frame that isn't full class normal windows use
  struct Frame {
      Frame(tk::EventHandler &evh, int screen_num);
      ~Frame();

      tk::SbWindow window;

      int x, y, x_hidden, y_hidden, grab_x, grab_y;
      unsigned int width, height, bevel_w;
  } frame;
  // background pixmap
  Pixmap m_window_pm;

  BScreen &m_screen; // screen connection

  tk::Timer m_hide_timer; // timer to for auto hide toolbar

  tk::LayerItem m_layeritem; // layer item, must be declared before layermenu
  LayerMenu m_layermenu;
  SbMenu m_placementmenu, m_toolbarmenu;

  // themes
  ToolbarTheme m_theme;

  typedef std::list<ToolbarItem *> ItemList;
  ItemList m_item_list;

  Strut *m_strut; // created and destroyed by BScreen

  // config items
  bool &m_rc_auto_hide, &m_rc_auto_raise,
       &m_rc_claim_space, &m_rc_visible;
  int  &m_rc_width_percent, &m_rc_on_head,
       &m_rc_height;
  tk::ResLayers_e &m_rc_layernum;
  tk::ToolbarPlacement_e &m_rc_placement;
  std::string &m_rc_tools;

  tk::Shape *m_shape = 0;
  std::list<std::string> m_tools;

  tk::MacroCommand *m_shared_sv_rcfg_macro = 0;
};

#endif // TOOLBAR_HH

// Copyright (c) 2023 Shynebox - zlice
//
// Copyright (c) 2002 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
//
// Toolbar.hh for Blackbox - an X11 Window manager
// Copyright (c) 1997 - 2000 Brad Hughes (bhughes@tcac.net)
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
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
