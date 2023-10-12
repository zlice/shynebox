// Toolbar.cc for Shynebox Window Manager

#include "Toolbar.hh"

#include "ToolbarItem.hh"
#include "ToolbarTheme.hh"

#include "shynebox.hh"
#include "SbCommands.hh"
#include "Keys.hh"
#include "Screen.hh"
#include "ScreenPlacement.hh"
#include "SystemTray.hh"
#include "Strut.hh"

#include "tk/I18n.hh"
#include "tk/ImageControl.hh"
#include "tk/LayerManager.hh"
#include "tk/MacroCommand.hh"
#include "tk/EventManager.hh"
#include "tk/SimpleCommand.hh"
#include "tk/BoolMenuItem.hh"
#include "tk/IntMenuItem.hh"
#include "tk/Shape.hh"
#include "tk/KeyUtil.hh"
#include "tk/StringUtil.hh"
#include "tk/TextUtils.hh"

#include <X11/Xutil.h>
#include <X11/keysym.h>

#ifdef HAVE_CSTRING
  #include <cstring>
#else
  #include <string.h>
#endif
#include <iterator>
#include <typeinfo>
#include <functional>

using std::string;
using std::pair;
using std::list;

#define TBPLC tk::ToolbarPlacement_e
#define RLEnum (int)tk::ResLayers_e

namespace {

// IMPORTANT TO KEEP ORDER
// these correlate to the ResNum (str<>enum) defined in Resources
const struct {
    tk::Orientation orient;
    unsigned int shape;
} _values[] = {
    { tk::ROT0,   tk::Shape::BOTTOMRIGHT                          },
    { tk::ROT0,   tk::Shape::BOTTOMRIGHT | tk::Shape::BOTTOMLEFT  },
    { tk::ROT0,   tk::Shape::BOTTOMLEFT                           },
    { tk::ROT0,   tk::Shape::TOPRIGHT                             },
    { tk::ROT0,   tk::Shape::TOPRIGHT    | tk::Shape::TOPLEFT     },
    { tk::ROT0,   tk::Shape::TOPLEFT                              },
    { tk::ROT270, tk::Shape::TOPRIGHT                             },
    { tk::ROT270, tk::Shape::TOPRIGHT    | tk::Shape::BOTTOMRIGHT },
    { tk::ROT270, tk::Shape::BOTTOMRIGHT                          },
    { tk::ROT90,  tk::Shape::TOPLEFT                              },
    { tk::ROT90,  tk::Shape::TOPLEFT     | tk::Shape::BOTTOMLEFT  },
    { tk::ROT90,  tk::Shape::BOTTOMLEFT                           },
};

class PlaceToolbarMenuItem: public tk::RadioMenuItem {
public:
  PlaceToolbarMenuItem(const tk::SbString &label, Toolbar &toolbar,
      TBPLC place):
      tk::RadioMenuItem(label), m_toolbar(toolbar), m_place(place) {
    setCloseOnClick(false);
  }
  bool isSelected() const { return m_toolbar.placement() == m_place; }
  void click(int button, int time, unsigned int mods) {
    (void) button;
    (void) time;
    (void) mods;
    m_toolbar.setPlacement(m_place);
    Shynebox::instance()->save_rc();
    m_toolbar.placementMenu().reconfigure();
    m_toolbar.reconfigure();
    m_toolbar.relayout(); // everything should update
  }
private:
  Toolbar &m_toolbar;
  TBPLC m_place;
};

} // end anonymous

// toolbar frame
Toolbar::Frame::Frame(tk::EventHandler &evh, int screen_num):
    window(screen_num, // screen (parent)
           0, 0, // pos
           10, 10, // size
           // event mask
           ButtonPressMask | ButtonReleaseMask | ExposureMask
           | EnterWindowMask | LeaveWindowMask | SubstructureNotifyMask,
           true) { // override redirect
  tk::EventManager &evm = *tk::EventManager::instance();
  evm.add(evh, window);
} // Toolbar::Frame struct-class init

Toolbar::Frame::~Frame() {
  tk::EventManager &evm = *tk::EventManager::instance();
  evm.remove(window);
} // Toolbar::Frame struct-class destroy

Toolbar::Toolbar(BScreen &scrn, tk::Layer &layer, size_t width):
      m_hidden(false),
      frame(*this, scrn.screenNumber() ),
      m_window_pm(0),
      m_screen(scrn),
      m_layeritem(frame.window, layer),
      m_layermenu(scrn.menuTheme(),
                  scrn.imageControl(),
                  *scrn.layerManager().getLayer(RLEnum::MENU),
                  this,
                  true),
      m_placementmenu(scrn.menuTheme(),
                      scrn.imageControl(),
                      *scrn.layerManager().getLayer(RLEnum::MENU) ),
      m_toolbarmenu(scrn.menuTheme(),
                    scrn.imageControl(),
                    *scrn.layerManager().getLayer(RLEnum::MENU) ),
      m_theme(scrn.screenNumber() ),
      m_tool_factory(scrn),
      m_strut(0),
      m_rc_auto_hide(*scrn.m_cfgmap["toolbar.autoHide"]),
      m_rc_auto_raise(*scrn.m_cfgmap["toolbar.autoRaise"]),
      m_rc_claim_space(*scrn.m_cfgmap["toolbar.claimSpace"]),
      m_rc_visible(*scrn.m_cfgmap["toolbar.visible"]),
      m_rc_width_percent(*scrn.m_cfgmap["toolbar.widthPercent"]),
      m_rc_on_head(*scrn.m_cfgmap["toolbar.onHead"]),
      m_rc_height(*scrn.m_cfgmap["toolbar.height"]),
      m_rc_layernum((tk::ResLayers_e&)(int&)(*scrn.m_cfgmap["toolbar.layer"]) ),
      m_rc_placement((TBPLC&)(int&)(*scrn.m_cfgmap["toolbar.placement"]) ),
      m_rc_tools(*scrn.m_cfgmap["toolbar.tools"]),
      m_shape(new tk::Shape(frame.window, 0) ) {
  _SB_USES_NLS;

  frame.window.setWindowRole("shynebox-toolbar");

  moveToLayer((int)m_rc_layernum);

  m_layermenu.setLabel(_SB_XTEXT(Toolbar, Layer, "Toolbar Layer",
                                 "Title of toolbar layer menu") );
  m_placementmenu.setLabel(_SB_XTEXT(Toolbar, Placement, "Toolbar Placement",
                                        "Title of toolbar placement menu") );
  m_toolbarmenu.setLabel(_SB_XTEXT(Toolbar, Toolbar, "Toolbar",
                                        "Title of toolbar menu") );

  m_layermenu.setInternalMenu();
  m_placementmenu.setInternalMenu();
  m_toolbarmenu.setInternalMenu();
  setupMenus();
  // add menu to screen
  screen().addConfigMenu(_SB_XTEXT(Toolbar, Toolbar, "Toolbar",
                       "title of toolbar menu item"), menu() );

  // geometry settings
  frame.width = width;
  frame.height = 10;
  frame.bevel_w = 1;
  frame.grab_x = frame.grab_y = 0;

  // setup hide timer
  m_hide_timer.setTimeout(Shynebox::instance()->getAutoRaiseDelay() * tk::SbTime::IN_MILLISECONDS);
  tk::SimpleCommand<Toolbar> *ucs(new tk::SimpleCommand<Toolbar>(*this, &Toolbar::updateCrossingState) );
  m_hide_timer.setCommand(*ucs);
  m_hide_timer.fireOnce(true);

  // show all windows
  frame.window.showSubwindows();

  // setup to listen to child events
  tk::EventManager::instance()->addParent(*this, window() );
  Shynebox::instance()->keys()->registerWindow(window().window(),
                                       *this, Keys::ON_TOOLBAR);
  // get everything together
  // this gets done by the screen later as it loads
  reconfigure();
} // Toolbar class init

Toolbar::~Toolbar() {
  Keys* keys = Shynebox::instance()->keys();
  if (keys)
    keys->unregisterWindow(window().window() );

  tk::EventManager::instance()->remove(window() );

  // remove menu items before we delete tools so we dont end up
  // with dangling pointers to old submenu items (internal menus)
  // from the tools
  screen().removeConfigMenu(menu() );

  menu().removeAll();
  deleteItems();
  clearStrut();

  if (m_window_pm)
    screen().imageControl().removeImage(m_window_pm);

  if (m_shape)
    delete m_shape;
  if (m_shared_sv_rcfg_macro)
    delete m_shared_sv_rcfg_macro;
} // Toolbar class destroy

void Toolbar::clearStrut() {
  if (m_strut == 0)
    return;
  screen().clearStrut(m_strut);
  m_strut = 0;
}

void Toolbar::updateStrut() {
  bool had_strut = m_strut ? true : false;
  clearStrut();
  // we should request space if we're in autohide mode or
  // if the user dont want to request space for toolbar.
  if (doAutoHide() || !m_rc_claim_space || !m_rc_visible) {
    if (had_strut)
      screen().updateAvailableWorkspaceArea();
    return;
  }

  // request area on screen
  int w = static_cast<int>(width() );
  int h = static_cast<int>(height() );
  int bw = m_theme.border().width();
  int top = 0, bottom = 0, left = 0, right = 0;
  switch (placement() ) {
  case TBPLC::TOPCENTER:
  case TBPLC::TOPLEFT:
  case TBPLC::TOPRIGHT:
      top = h + 2 * bw;
      break;
  case TBPLC::BOTTOMCENTER:
  case TBPLC::BOTTOMLEFT:
  case TBPLC::BOTTOMRIGHT:
      bottom = h + 2 * bw;
      break;
  case TBPLC::RIGHTCENTER:
  case TBPLC::RIGHTTOP:
  case TBPLC::RIGHTBOTTOM:
      right = w + 2 * bw;
      break;
  case TBPLC::LEFTCENTER:
  case TBPLC::LEFTTOP:
  case TBPLC::LEFTBOTTOM:
      left = w + 2 * bw;
      break;
  };
  m_strut = screen().requestStrut(getOnHead(), left, right, top, bottom);
  screen().updateAvailableWorkspaceArea();
} // updateStrut

void Toolbar::raise() {
  m_layeritem.raise();
}

void Toolbar::lower() {
  m_layeritem.lower();
}

void Toolbar::relayout() {
  for (auto it : m_item_list)
    it->updateSizing();

  rearrangeItems();
}

void Toolbar::reconfigure() {
  updateVisibleState();

  if (doAutoHide() && !isHidden() && !m_hide_timer.isTiming() )
    m_hide_timer.start();
  if (!doAutoHide() && isHidden() )
    toggleHidden();

  m_tool_factory.updateThemes();

  // parse resource tools and determine if we need to rebuild toolbar

  // parse and transform to lower case
  list<string> tools;
  tk::StringUtil::stringtok(tools, m_rc_tools, ", ");
  transform(tools.begin(),
            tools.end(),
            tools.begin(),
            tk::StringUtil::toLower);

  if (tools.empty() || tools.size() != m_tools.size() || tools != m_tools) {
    // destroy tools and rebuild them
    deleteItems();
    screen().clearToolButtonMap();
    menu().removeAll();
    setupMenus(true); // rebuild menu but skip rebuild of placement menu
    m_tools = tools; // copy values

    if (!m_tools.empty() ) {
      // create items
      for (auto &item_it : m_tools) {
        ToolbarItem *item = m_tool_factory.create(item_it, frame.window, *this);
        if (item == 0)
          continue;
        m_item_list.push_back(item);
      }
      // show all items
      frame.window.showSubwindows();
    }
  } else // just update the menu
    menu().reconfigure();

  frame.bevel_w = m_theme.bevelWidth();

  // destroy shape if the theme wasn't specified with one,
  // or create one
  if (m_theme.shape() == false && m_shape) {
    delete m_shape;
    m_shape = 0;
  } else if (m_theme.shape() && m_shape == 0)
    m_shape = new tk::Shape(frame.window, 0);

  const int old_w = frame.width, old_h = frame.height;
  setPlacement(placement() ); // recalibrate size

  // inlines virtual 'moveResize' in psuedo frame.window
  // less overhead/branches to set then only check hidden
  int frm_width = frame.width, frm_height = frame.height,
      frm_x = frame.x, frm_y = frame.y;
  if (isHidden() ) {
    frm_x = frame.x_hidden;
    frm_y = frame.y_hidden;
  }
  frame.window.moveResize(frm_x, frm_y,
                          frm_width, frm_height);

  // for head changes and style reloads
  // (fixes lazy systray resize by height)
  if (old_w != frm_width || old_h != frm_height)
    relayout();

  // render frame window
  Pixmap tmp = m_window_pm;
  if (!m_theme.toolbar().usePixmap() ) {
    m_window_pm = 0;
    frame.window.setBackgroundColor(m_theme.toolbar().color() );
  } else {
    tk::Orientation orient = tk::ROT0;
    TBPLC where = placement();
    orient = _values[(int)where].orient;
    m_window_pm = screen().imageControl().renderImage(
                  frame.window.width(), frame.window.height(),
                  m_theme.toolbar(), orient);
    frame.window.setBackgroundPixmap(m_window_pm);
  }
  if (tmp)
    screen().imageControl().removeImage(tmp);

  frame.window.setBorderColor(m_theme.border().color() );
  frame.window.setBorderWidth(m_theme.border().width() );

  frame.window.clear();

  if (m_theme.shape() && m_shape)
    m_shape->update();

  // we're done with all resizing and stuff now we
  // can request a new area to be reserved on screen
  updateStrut();
} // reconfigure

void Toolbar::updateCrossingState() {
  const int bw = -m_theme.border().width();
  int x, y;
  bool hovered = false;

  // x/y will be -1 on rare errors
  tk::KeyUtil::get_pointer_coords(Shynebox::instance()->display(),
                                  window().window(), x, y);

  hovered = x >= bw && y >= bw
            && x < int(width() ) && y < int(height() );

  if (hovered) {
    if (m_rc_auto_raise)
      m_layeritem.moveToLayer(RLEnum::ABOVE_DOCK);
    if (m_rc_auto_hide && isHidden() )
      toggleHidden();
  } else {
    if (m_rc_auto_hide && !isHidden() )
      toggleHidden();
    if (m_rc_auto_raise)
      m_layeritem.moveToLayer((int)m_rc_layernum);
  }
} // updateCrossingState

void Toolbar::enterNotifyEvent(XCrossingEvent &ce) {
  Shynebox::instance()->keys()->doAction(ce.type, ce.state, 0, Keys::ON_TOOLBAR);

  if (!m_rc_auto_hide && isHidden() )
    toggleHidden();

  if ((m_rc_auto_hide || m_rc_auto_raise) && !m_hide_timer.isTiming() )
    m_hide_timer.start();
}

void Toolbar::leaveNotifyEvent(XCrossingEvent &event) {
  if (menu().isVisible() )
    return;

  if ((!m_hide_timer.isTiming() && (m_rc_auto_hide && !isHidden() ) )
     || (m_rc_auto_raise && m_layeritem.getLayerNum() != (int)m_rc_layernum) )
    m_hide_timer.start();

  if (!isHidden() )
    Shynebox::instance()->keys()->doAction(event.type, event.state, 0, Keys::ON_TOOLBAR);
}


void Toolbar::exposeEvent(XExposeEvent &ee) {
  if (ee.window == frame.window)
    frame.window.clearArea(ee.x, ee.y, ee.width, ee.height);
}

void Toolbar::setPlacement(TBPLC where) {
  m_rc_placement = where;

  int head_x = screen().getHeadX(m_rc_on_head),
      head_y = screen().getHeadY(m_rc_on_head),
      head_w = screen().getHeadWidth(m_rc_on_head),
      head_h = screen().getHeadHeight(m_rc_on_head);

  int bw = m_theme.border().width();
  int pixel = (bw == 0 ? 1 : 0); // So we get at least one pixel visible in hidden mode

  frame.width = (head_w - 2*bw) * m_rc_width_percent / 100;

  // max height of each toolbar items font...
  unsigned int max_height = m_tool_factory.maxFontHeight() + 2;

  if (m_theme.height() > 0)
    max_height = m_theme.height();

  if (m_rc_height > 0 && m_rc_height < 100)
    max_height = m_rc_height;

  frame.height = max_height;
  frame.height += (frame.bevel_w * 2);

  const tk::Orientation orient = _values[(int)where].orient;

  if (orient != tk::ROT0) { // vertical, 180 not reachable
    frame.width = frame.height;
    frame.height = head_h * m_rc_width_percent / 100;
  }

  frame.x = head_x;
  frame.y = head_y;
  frame.x_hidden = head_x;
  frame.y_hidden = head_y;

  if (m_shape)
    m_shape->setPlaces(_values[(int)where].shape);

  switch (where) {
  case TBPLC::TOPLEFT:
      frame.y_hidden += pixel - bw - frame.height;
      break;
  case TBPLC::BOTTOMLEFT:
      frame.y += head_h - static_cast<int>(frame.height) - 2*bw;
      frame.y_hidden += head_h - bw - pixel;
      break;
  case TBPLC::TOPCENTER:
      frame.x += (head_w - static_cast<int>(frame.width) )/2 - bw;
      frame.x_hidden = frame.x;
      frame.y_hidden += pixel - bw - static_cast<int>(frame.height);
      break;
  case TBPLC::TOPRIGHT:
      frame.x += head_w - static_cast<int>(frame.width) - bw*2;
      frame.x_hidden = frame.x;
      frame.y_hidden += pixel - bw - static_cast<int>(frame.height);
      break;
  case TBPLC::BOTTOMRIGHT:
      frame.x += head_w - static_cast<int>(frame.width) - bw*2;
      frame.y += head_h - static_cast<int>(frame.height) - bw*2;
      frame.x_hidden = frame.x;
      frame.y_hidden += head_h - bw - pixel;
      break;
  case TBPLC::BOTTOMCENTER: // default is BOTTOMCENTER
      frame.x += (head_w - static_cast<int>(frame.width) )/2 - bw;
      frame.y += head_h - static_cast<int>(frame.height) - bw*2;
      frame.x_hidden = frame.x;
      frame.y_hidden += head_h - bw - pixel;
      break;
  case TBPLC::LEFTCENTER:
      frame.y += (head_h - static_cast<int>(frame.height) )/2 - bw;
      frame.y_hidden = frame.y;
      frame.x_hidden += pixel - static_cast<int>(frame.width) - bw;
      break;
  case TBPLC::LEFTTOP:
      frame.x_hidden += pixel - static_cast<int>(frame.width) - bw;
      break;
  case TBPLC::LEFTBOTTOM:
      frame.y = head_h - static_cast<int>(frame.height) - bw*2;
      frame.y_hidden = frame.y;
      frame.x_hidden += pixel - static_cast<int>(frame.width) - bw;
      break;
  case TBPLC::RIGHTCENTER:
      frame.x += head_w - static_cast<int>(frame.width) - bw*2;
      frame.y += (head_h - static_cast<int>(frame.height) )/2 - bw;
      frame.x_hidden += head_w - bw - pixel;
      frame.y_hidden = frame.y;
      break;
  case TBPLC::RIGHTTOP:
      frame.x += head_w - static_cast<int>(frame.width) - bw*2;
      frame.x_hidden += head_w - bw - pixel;
      break;
  case TBPLC::RIGHTBOTTOM:
      frame.x += head_w - static_cast<int>(frame.width) - bw*2;
      frame.y += head_h - static_cast<int>(frame.height) - bw*2;
      frame.x_hidden += head_w - bw - pixel;
      frame.y_hidden = frame.y;
      break;
  } // switch(where)

  for (auto it : m_item_list)
    it->setOrientation(orient);
} // setPlacement

void Toolbar::updateVisibleState() {
  m_rc_visible ? frame.window.show() : frame.window.hide();
}

void Toolbar::toggleHidden() {
  m_hidden = ! m_hidden;
  if (isHidden() )
    frame.window.move(frame.x_hidden, frame.y_hidden);
  else {
    frame.window.move(frame.x, frame.y);
    for (auto it : m_item_list)
      it->parentMoved();
  }
}

void Toolbar::toggleAboveDock() {
  if (m_layeritem.getLayerNum() == (int)m_rc_layernum)
    m_layeritem.moveToLayer(RLEnum::ABOVE_DOCK);
  else
    m_layeritem.moveToLayer((int)m_rc_layernum);
}

void Toolbar::moveToLayer(int layernum) {
  m_layeritem.moveToLayer(layernum);
  m_rc_layernum = (tk::ResLayers_e)layernum;
}

void Toolbar::setupMenus(bool skip_new_placement) {
  _SB_USES_NLS;
  using namespace tk;

#define TOOLBAR_CMD *(Command<void>*) new SimpleCommand<Toolbar>
Command<void> *shared_saverc = Shynebox::instance()->getSharedSaveRC();
#define NEW_SAVERC *(Command<void>*) shared_saverc

  if (!m_shared_sv_rcfg_macro) {
    m_shared_sv_rcfg_macro = new MacroCommand();
    m_shared_sv_rcfg_macro->add(TOOLBAR_CMD(*this, &Toolbar::reconfigure) );
    m_shared_sv_rcfg_macro->add(NEW_SAVERC);
    m_shared_sv_rcfg_macro->set_is_shared();
  }

  menu().insertItem(new BoolMenuItem(_SB_XTEXT(Common, Visible,
                                           "Visible", "Whether this item is visible"),
                                 m_rc_visible, *m_shared_sv_rcfg_macro) );

  menu().insertItem(new BoolMenuItem(_SB_XTEXT(Common, AutoHide,
                                           "Auto hide", "Toggle auto hide of toolbar"),
                                 m_rc_auto_hide, *m_shared_sv_rcfg_macro) );

  menu().insertItem(new BoolMenuItem(_SB_XTEXT(Common, AutoRaise,
                                           "Auto raise", "Toggle auto raise of toolbar"),
                                 m_rc_auto_raise, *m_shared_sv_rcfg_macro) );

  MenuItem *toolbar_menuitem =
      new IntMenuItem(_SB_XTEXT(Toolbar, WidthPercent,
                                   "Toolbar width percent",
                                   "Percentage of screen width taken by toolbar"),
                         m_rc_width_percent, 0, 100, menu() ); // min/max value


  toolbar_menuitem->setCommand(*m_shared_sv_rcfg_macro);
  menu().insertItem(toolbar_menuitem);

  menu().insertItem(new BoolMenuItem(_SB_XTEXT(Common, MaximizeOver, // NLS
                                           "Claim Space",
                                           "Reserve strut space on screen"),
                                 m_rc_claim_space, *m_shared_sv_rcfg_macro) );
  menu().insertSubmenu(_SB_XTEXT(Menu, Layer, "Layer...", "Title of Layer menu"), &layerMenu() );

  MenuItem *head_menuitem =
      new IntMenuItem(_SB_XTEXT(Toolbar, ToolbarHead,
                                   "Head to place toolbar",
                                   "What monitor to put toolbar. 0 stretches all."),
                         m_rc_on_head, 0, screen().numHeads() - 1, menu() ); // min/max value
  head_menuitem->setCommand(*m_shared_sv_rcfg_macro);
  menu().insertItem(head_menuitem);
#undef TOOLBAR_CMD
#undef NEW_SAVERC

  // menu is 3 wide, 5 down
  if (!skip_new_placement) {
      static const struct {
        const SbString label;
        TBPLC placement;
      } pm[] = {
          { _SB_XTEXT(Align, TopLeft, "Top Left", "Top Left"), TBPLC::TOPLEFT},
          { _SB_XTEXT(Align, LeftTop, "Left Top", "Left Top"), TBPLC::LEFTTOP},
          { _SB_XTEXT(Align, LeftCenter, "Left Center", "Left Center"), TBPLC::LEFTCENTER},
          { _SB_XTEXT(Align, LeftBottom, "Left Bottom", "Left Bottom"), TBPLC::LEFTBOTTOM},
          { _SB_XTEXT(Align, BottomLeft, "Bottom Left", "Bottom Left"), TBPLC::BOTTOMLEFT},
          { _SB_XTEXT(Align, TopCenter, "Top Center", "Top Center"), TBPLC::TOPCENTER},
          { "", TBPLC::TOPLEFT},
          { "", TBPLC::TOPLEFT},
          { "", TBPLC::TOPLEFT},
          { _SB_XTEXT(Align, BottomCenter, "Bottom Center", "Bottom Center"), TBPLC::BOTTOMCENTER},
          { _SB_XTEXT(Align, TopRight, "Top Right", "Top Right"), TBPLC::TOPRIGHT},
          { _SB_XTEXT(Align, RightTop, "Right Top", "Right Top"), TBPLC::RIGHTTOP},
          { _SB_XTEXT(Align, RightCenter, "Right Center", "Right Center"), TBPLC::RIGHTCENTER},
          { _SB_XTEXT(Align, RightBottom, "Right Bottom", "Right Bottom"), TBPLC::RIGHTBOTTOM},
          { _SB_XTEXT(Align, BottomRight, "Bottom Right", "Bottom Right"), TBPLC::BOTTOMRIGHT}
      };

      m_placementmenu.setMinimumColumns(3);
      // create items in sub menu
      for (size_t i=0; i< sizeof(pm)/sizeof(pm[0]); ++i) {
        if (pm[i].label == "") {
          m_placementmenu.insert(pm[i].label);
          m_placementmenu.setItemEnabled(i, false);
        } else
          m_placementmenu.insertItem(new PlaceToolbarMenuItem(pm[i].label, *this,
                                                              pm[i].placement) );
      } // for placement menu items
  } // if ! skip_new_placement

  menu().insertSubmenu(_SB_XTEXT(Menu, Placement, "Placement",
               "Title of Placement menu"), &m_placementmenu );
  m_placementmenu.updateMenu();

  menu().updateMenu();
} // setupMenus

/*
 * Place items next to each other, with a bevel width between,
 * above and below each item. BUT, if there is no bevel width, then
 * borders should be merged for evenness.
 */
/* PROBLEM
 * Although this is very similar logic to 'ButtonTrain' it is 110% not
 * worth trying to merge types and put everything in 'ButtonTrain'.
 * Overall size is ~917 bytes, just making a single ToolbarItem a
 * type-of-button instead of having variable-obj-button increases
 * size beyond what you'd save here.
 * (ToolbarItem w/ virtual SbWindow is even more than TBI with a few funcs)
*/
void Toolbar::rearrangeItems() {
  if (screen().isShuttingdown() || m_item_list.empty() )
    return;

  tk::Orientation orient = _values[(int)placement()].orient;

  // calculate size for fixed items
  int bevel_width = m_theme.bevelWidth();
  int fixed_width = bevel_width; // combined size of all fixed items
  int relative_width = 0; // combined *desired* size of all relative items
  int relative_items = 0;
  int last_bw = 0; // we show the largest border of adjoining items
  bool first = true;
  int leftover_width = 0;

  unsigned int width = this->width(), height = this->height();
  unsigned int tmpw = 0, tmph = 0;
  tk::translateSize(orient, width, height);

  for (auto &item_it : m_item_list) {
    if (!item_it->active() )
      continue;

    int it_bw = item_it->borderWidth();

    // the bevel and border are fixed whether relative or not
    if (bevel_width > 0)
      fixed_width += bevel_width + 2*it_bw;
    else if (!first)
      fixed_width += std::max(it_bw, last_bw);
    else
      first = false;

    last_bw = it_bw;

    tmpw = item_it->preferredWidth();
    tmph = item_it->height();
    tk::translateSize(orient, tmpw, tmph);

    if (item_it->type() == ToolbarItem::RELATIVE) {
        ++relative_items;
        relative_width += tmpw;
    } else // ToolbarItem::FIXED:
      fixed_width += tmpw;
  } // for m_item_list

  // calculate what's going to be left over to the relative sized items
  if (relative_items) {
    relative_width = (width - fixed_width) / relative_items;
    leftover_width = width - fixed_width - relative_items * relative_width;
  }

  // now move and resize the items
  // borderWidth added back on straight away
  int next_x = -m_item_list.front()->borderWidth(); // list isn't empty
  if (bevel_width != 0)
    next_x = 0;

  last_bw = 0;
  int it_bw, offset, size_offset, tmpx, tmpy;

  for (auto &item_it : m_item_list) {
    it_bw = item_it->borderWidth();
    if (!item_it->active() ) {
      item_it->hide();
      // make sure it still gets told the toolbar height
      tmpw = 1; tmph = height - 2*(bevel_width+it_bw);
      if (tmph >= (1<<30) )
        tmph = 1;
      tk::translateSize(orient, tmpw, tmph);
      item_it->resize(tmpw, tmph);  // width of 0 changes to 1 anyway
      continue;
    }
    offset = bevel_width;
    size_offset = 2*(it_bw + bevel_width);

    if (bevel_width == 0) {
      offset = -it_bw;
      size_offset = 0;
      next_x += std::max(it_bw, last_bw);
    }
    last_bw = it_bw;

    tmpx = next_x + offset,
    tmpy = offset;
    tmph = height - size_offset;

    if (item_it->type() == ToolbarItem::RELATIVE) {
      tmpw = relative_width;
      if (leftover_width) {
        --leftover_width;
        ++tmpw;
      }
    } else { // ToolbarItem::FIXED:
      unsigned int itemw = item_it->width(), itemh = item_it->height();
      tk::translateSize(orient, itemw, itemh); // dupe? below
      tmpw = itemw;
    }

    if (tmpw >= (1<<30) ) tmpw = 1;
    if (tmph >= (1<<30) ) tmph = 1;
    next_x += tmpw + bevel_width;
    if (bevel_width != 0)
      next_x += 2*it_bw;

    tk::translateCoords(orient, tmpx, tmpy, width, height);
    tk::translatePosition(orient, tmpx, tmpy, tmpw, tmph, it_bw);
    tk::translateSize(orient, tmpw, tmph);
    item_it->moveResize(tmpx, tmpy, tmpw, tmph);
  } // for m_item_list
  frame.window.clear();
} // rearrangeItems

void Toolbar::deleteItems() {
  while (!m_item_list.empty() ) {
    delete m_item_list.back();
    m_item_list.pop_back();
  }
  m_tools.clear();
}

// Copyright (c) 2023 Shynebox - zlice
//
// Copyright (c) 2002 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
//
// Toolbar.cc for Blackbox - an X11 Window manager
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
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
