// MenuCreator.cc for Shynebox Window Manager

#include "MenuCreator.hh"

#include "defaults.hh"
#include "Screen.hh"
#include "shynebox.hh"
#include "Window.hh"
#include "WindowCmd.hh"
#include "CurrentWindowCmd.hh"
#include "WindowMenuAccessor.hh"

#include "ClientMenu.hh"
#include "WorkspaceMenu.hh"
#include "LayerMenu.hh"
#include "SendToMenu.hh"

#include "SbMenuParser.hh"
#include "StyleMenuItem.hh"
#include "RootCmdMenuItem.hh"
#include "SbCommands.hh"

#include "tk/I18n.hh"
#include "tk/BoolMenuItem.hh"
#include "tk/CommandParser.hh"
#include "tk/FileUtil.hh"
#include "tk/LayerManager.hh"
#include "tk/MacroCommand.hh"
#include "tk/MenuSeparator.hh"
#include "tk/SimpleCommand.hh"
#include "tk/StringUtil.hh"

#include <iostream>
#include <list>

#include "Remember.hh"

using std::cerr;
using std::string;
using std::vector;
using std::list;
using std::less;
using tk::AutoReloadHelper;

namespace {
tk::StringConvertor s_stringconvertor(tk::StringConvertor::ToSbString);

list<string> s_encoding_stack;
list<size_t> s_stacksize_stack;

enum {
    L_SHADE = 0,
    L_MAXIMIZE,
    L_ICONIFY,
    L_CLOSE,
    L_KILL,
    L_LOWER,
    L_RAISE,
    L_STICK,
    L_TITLE,
    L_SENDTO,
    L_LAYER,

    L_REMEMBER,

    L_MENU_EXIT,
    L_MENU_ICONS,
};

// returns 'label' if not empty, otherwise a (translated) default
// value based upon 'type'
const tk::SbString& _l(const tk::SbString& label, size_t type) {
  _SB_USES_NLS;
  static const tk::SbString _default_labels[] = {
      _SB_XTEXT(Windowmenu, Shade, "Shade", "Shade the window"),
      _SB_XTEXT(Windowmenu, Maximize, "Maximize", "Maximize the window"),
      _SB_XTEXT(Windowmenu, Iconify, "Iconify", "Iconify the window"),
      _SB_XTEXT(Windowmenu, Close, "Close", "Close the window"),
      _SB_XTEXT(Windowmenu, Kill, "Kill", "Kill the window"),
      _SB_XTEXT(Windowmenu, Lower, "Lower", "Lower the window"),
      _SB_XTEXT(Windowmenu, Raise, "Raise", "Raise the window"),
      _SB_XTEXT(Windowmenu, Stick, "Stick", "Stick the window"),
      _SB_XTEXT(Windowmenu, SetTitle, "Set Title", "Change the title of the window"),
      _SB_XTEXT(Windowmenu, SendTo, "Send To...", "Send to menu item name"),
      _SB_XTEXT(Windowmenu, Layer, "Layer ...", "Layer menu"),

      _SB_XTEXT(Remember, MenuItemName, "Remember...", "Remember item in menu"),

      _SB_XTEXT(Menu, Exit, "Exit", "Exit Command"),
      _SB_XTEXT(Menu, Icons, "Icons", "Iconic windows menu title"),
  };

  if (label.empty() )
    return _default_labels[type];

  return label;
}

/**
 * Push the encoding onto the stack, and make it active.
 */
void startEncoding(const string &encoding) {
  // we push it regardless of whether it's valid, since we
  // need to stay balanced with the endEncodings.
  s_encoding_stack.push_back(encoding);

  // this won't change if it doesn't succeed
  s_stringconvertor.setSource(encoding);
}

/**
 * Pop the encoding from the stack, unless we are at our stacksize limit.
 * Restore the previous (valid) encoding.
 */
void endEncoding() {
  size_t min_size = s_stacksize_stack.back();
  if (s_encoding_stack.size() <= min_size) {
    _SB_USES_NLS;
    cerr<<_SB_CONSOLETEXT(Menu, ErrorEndEncoding, "Warning: unbalanced [encoding] tags",
                                 "User menu file had unbalanced [encoding] tags")<<"\n";
    return;
  }

  s_encoding_stack.pop_back();
  s_stringconvertor.reset();

  list<string>::reverse_iterator it = s_encoding_stack.rbegin();
  list<string>::reverse_iterator it_end = s_encoding_stack.rend();
  while (it != it_end && !s_stringconvertor.setSource(*it) )
    ++it;

  if (it == it_end)
    s_stringconvertor.setSource("");
}

/* push our encoding-stacksize onto the stack */
void startFile() {
  if (s_encoding_stack.empty() )
    s_stringconvertor.setSource("");
  s_stacksize_stack.push_back(s_encoding_stack.size() );
}

/**
 * Pop necessary encodings from the stack
 * (and endEncoding the final one) to our matching encoding-stacksize.
 */
void endFile() {
  size_t target_size = s_stacksize_stack.back();
  size_t curr_size = s_encoding_stack.size();

  if (target_size != curr_size) {
    _SB_USES_NLS;
    cerr<<_SB_CONSOLETEXT(Menu, ErrorEndEncoding, "Warning: unbalanced [encoding] tags",
                                 "User menu file had unbalanced [encoding] tags")<<"\n";
  }

  for (; curr_size > (target_size+1); --curr_size)
    s_encoding_stack.pop_back();

  if (curr_size == (target_size+1) )
    endEncoding();

  s_stacksize_stack.pop_back();
}

void createStyleMenu(tk::Menu &parent, AutoReloadHelper *reloader,
                     const string &directory) {
  // perform shell style ~ home directory expansion
  string stylesdir(tk::StringUtil::expandFilename(directory) );

  if (!tk::FileUtil::isDirectory(stylesdir.c_str() ) )
    return;

  if (reloader)
    reloader->addFile(stylesdir);

  tk::Directory dir(stylesdir.c_str() );

  // create a vector of all the filenames in the directory
  // add sort it
  vector<string> filelist(dir.entries() );
  for (size_t file_index = 0; file_index < dir.entries(); ++file_index)
    filelist[file_index] = dir.readFilename();

  sort(filelist.begin(), filelist.end(), less<string>() );

  // for each file in directory add filename and path to menu
  for (size_t file_index = 0; file_index < dir.entries(); file_index++) {
    string style(stylesdir + '/' + filelist[file_index]);
    // add to menu only if the file is a regular file, and not a
    // .file or a backup~ file
    if ((tk::FileUtil::isRegularFile(style.c_str() )
           && (filelist[file_index][0] != '.')
           && (style[style.length() - 1] != '~') )
         || tk::FileUtil::isRegularFile((style + "/theme.cfg").c_str() )
         || tk::FileUtil::isRegularFile((style + "/style.cfg").c_str() ) )
      parent.insertItem(new StyleMenuItem(filelist[file_index], style) );
  }
  // update menu graphics
  parent.updateMenu();
} // createStyleMenu

void createRootCmdMenu(tk::Menu &parent, const string &directory,
                       AutoReloadHelper *reloader, const string &cmd) {
  // perform shell style ~ home directory expansion
  string rootcmddir(tk::StringUtil::expandFilename(directory) );

  if (!tk::FileUtil::isDirectory(rootcmddir.c_str() ) )
    return;

  if (reloader)
    reloader->addFile(rootcmddir);

  tk::Directory dir(rootcmddir.c_str() );

  // create a vector of all the filenames in the directory
  // add sort it
  vector<string> filelist(dir.entries() );
  for (size_t file_index = 0; file_index < dir.entries(); ++file_index)
    filelist[file_index] = dir.readFilename();

  sort(filelist.begin(), filelist.end(), less<string>() );

  // for each file in directory add filename and path to menu
  for (size_t file_index = 0; file_index < dir.entries(); file_index++) {
    string rootcmd(rootcmddir+ '/' + filelist[file_index]);
    // add to menu only if the file is a regular file, and not a
    // .file or a backup~ file
    if ((tk::FileUtil::isRegularFile(rootcmd.c_str() )
         && (filelist[file_index][0] != '.')
         && (rootcmd[rootcmd.length() - 1] != '~') ) )
      parent.insertItem(new RootCmdMenuItem(filelist[file_index], rootcmd, cmd) );
  }
  // update menu graphics
  parent.updateMenu();
} // createRootCmdMenu

class ParseItem {
public:
  explicit ParseItem(tk::Menu *menu):m_menu(menu) {}

  void load(SbMenuParser &p, tk::StringConvertor &m_labelconvertor) {
    p>>m_key>>m_label>>m_cmd>>m_icon;
    m_label.second = m_labelconvertor.recode(m_label.second);
  }
  const string &icon() const { return m_icon.second; }
  const string &command() const { return m_cmd.second; }
  const string &label() const { return m_label.second; }
  const string &key() const { return m_key.second; }
  tk::Menu *menu() { return m_menu; }
private:
  SbMenuParser::Item m_key, m_label, m_cmd, m_icon;
  tk::Menu *m_menu;
};

class LayerMenuAccessor: public LayerObject {
public:
  void moveToLayer(int layer_number) {
    if (SbMenu::window() == 0)
      return;
    SbMenu::window()->moveToLayer(layer_number);
  }
  int layerNumber() const {
    if (SbMenu::window() == 0)
      return -1;
    return SbMenu::window()->layerItem().getLayerNum();
  }
};

void translateMenuItem(SbMenuParser &parse, ParseItem &item,
                       tk::StringConvertor &labelconvertor,
                       AutoReloadHelper *reloader);

void parseMenu(SbMenuParser &pars, tk::Menu &menu,
               tk::StringConvertor &label_convertor,
               AutoReloadHelper *reloader) {
  ParseItem pitem(&menu);
  while (!pars.eof() ) {
    pitem.load(pars, label_convertor);
    if (pitem.key() == "end")
      return;
    translateMenuItem(pars, pitem, label_convertor, reloader);
  }
}

void translateMenuItem(SbMenuParser &parse, ParseItem &pitem,
                       tk::StringConvertor &labelconvertor,
                       AutoReloadHelper *reloader) {
  if (pitem.menu() == 0)
    throw string("translateMenuItem: We must have a menu in ParseItem!");

  tk::Menu &menu = *pitem.menu();
  const string &str_key = pitem.key();
  const string &str_cmd = pitem.command();
  const string &str_label = pitem.label();

  const int screen_number = menu.screenNumber();
  _SB_USES_NLS;

  if (str_key == "end")
    return;
  else if (str_key == "nop") {
    int menuSize = menu.insert(str_label);
    menu.setItemEnabled(menuSize-1, false);
  //} else if (str_key == "icons") {
  //   TODO: reinstate ? this could be part of a custom menu ???
  //         would need to grab from WorkspaceMenu and set as 'internal'/shared/managed
  //   NOTE: this was previously here but moved directly into the WorkspaceMenu init()
  //         since it's the only user
  //    tk::Menu *submenu = MenuCreator::createMenuType("iconmenu", menu.screenNumber() );
  //    if (submenu == 0)
  //      return;
  //    menu.insertSubmenu(_l(str_label, L_MENU_ICONS), submenu);
  } else if (str_key == "exit") { // exit
    tk::Command<void> *exit_cmd = new SbCommands::ExitShyneboxCmd();
    menu.insertCommand(_l(str_label, L_MENU_EXIT), *exit_cmd);
  } else if (str_key == "exec") {
    tk::Command<void> *exec_cmd(tk::CommandParser<void>::instance().parse("exec " + str_cmd) );
    menu.insertCommand(str_label, *exec_cmd);
  } else if (str_key == "macrocmd") {
    tk::Command<void> *macro_cmd(tk::CommandParser<void>::instance().parse("macrocmd " + str_cmd) );
    menu.insertCommand(str_label, *macro_cmd);
  } else if (str_key == "style") {
    menu.insertItem(new StyleMenuItem(str_label, str_cmd) );
  } else if (str_key == "config") {
    BScreen *screen = Shynebox::instance()->findScreen(screen_number);
    menu.insertSubmenu(str_label, &screen->configMenu() );
  } // end of config
  else if (str_key == "include") { // include
    // this will make sure we dont get stuck in a loop
    static size_t safe_counter = 0;
    if (safe_counter > 10)
      return;

    safe_counter++;

    string newfile = tk::StringUtil::expandFilename(str_label);
    if (tk::FileUtil::isDirectory(newfile.c_str() ) ) {
      // inject every file in this directory into the current menu
      tk::Directory dir(newfile.c_str() );

      vector<string> filelist(dir.entries() );
      for (size_t file_index = 0; file_index < dir.entries(); ++file_index)
        filelist[file_index] = dir.readFilename();
      sort(filelist.begin(), filelist.end(), less<string>() );

      for (size_t file_index = 0; file_index < dir.entries(); file_index++) {
        string thisfile(newfile + '/' + filelist[file_index]);
        if (tk::FileUtil::isRegularFile(thisfile.c_str() )
            && (filelist[file_index][0] != '.')
            && (thisfile[thisfile.length() - 1] != '~') )
          MenuCreator::createFromFile(thisfile, menu, reloader, false);
      } // for files in dir
    } else // inject this file into the current menu
        MenuCreator::createFromFile(newfile, menu, reloader, false);
    // if directory
    safe_counter--;
  } else if (str_key == "submenu") {
    tk::Menu *submenu = MenuCreator::createMenu("", screen_number);
    if (submenu == 0)
      return;

    if (!str_cmd.empty() )
      submenu->setLabel(str_cmd);
    else
      submenu->setLabel(str_label);

    parseMenu(parse, *submenu, labelconvertor, reloader);
    submenu->updateMenu();
    menu.insertSubmenu(str_label, submenu);
  } // end of submenu
  else if (str_key == "stylesdir" || str_key == "stylesmenu") {
    createStyleMenu(menu, reloader,
                    str_key == "stylesmenu" ? str_cmd : str_label);
  } // end of stylesdir
  else if (str_key == "themesdir" || str_key == "themesmenu") {
    createStyleMenu(menu, reloader,
                    str_key == "themesmenu" ? str_cmd : str_label);
  } // end of themesdir
  else if (str_key == "wallpapers" || str_key == "wallpapermenu"
           || str_key == "rootcommands") {
    createRootCmdMenu(menu, str_label, reloader,
                      str_cmd == "" ? realProgramName("sbsetbg") : str_cmd);
  } // end of wallpapers
  else if (str_key == "workspaces") {
    BScreen *screen = Shynebox::instance()->findScreen(screen_number);
    screen->workspaceMenu().setInternalMenu();
    menu.insertSubmenu(str_label, &screen->workspaceMenu() );
  } else if (str_key == "separator") {
    menu.insertItem(new tk::MenuSeparator() );
  } else if (str_key == "encoding") {
    startEncoding(str_cmd);
  } else if (str_key == "endencoding") {
    endEncoding();
  } else if (!MenuCreator::createWindowMenuItem(str_key, str_label, menu) ) {
    // if we didn't find any special menu item we try with command parser
    // we need to attach command to arguments so command parser can parse it
    string line = str_key + " " + str_cmd;
    tk::Command<void> *command(tk::CommandParser<void>::instance().parse(line) );
    if (command != 0) {
      // special NLS default labels
      if (str_label.empty() ) {
        if (str_key == "reconfig" || str_key == "reconfigure") {
            menu.insertCommand(_SB_XTEXT(Menu, Reconfigure, "Reload Config",
                               "Reload all the configs"), *command);
            return;
        } else if (str_key == "restart") {
            menu.insertCommand(_SB_XTEXT(Menu, Restart, "Restart", "Restart Command"), *command);
            return;
        }
      } else
        menu.insertCommand(str_label, *command);
    }
  } // if-chain menu string parsing

  if (menu.numberOfItems() != 0) {
    tk::MenuItem *item = menu.find(menu.numberOfItems() - 1);
    if (item != 0 && !pitem.icon().empty() )
      item->setIcon(pitem.icon(), menu.screenNumber() );
  }
} // translateMenuItem

bool getStart(SbMenuParser &parser, string &label, tk::StringConvertor &labelconvertor) {
  ParseItem pitem(0);
  while (!parser.eof() ) {
    // get first begin line
    pitem.load(parser, labelconvertor);
    if (pitem.key() == "begin")
      break;
  }
  if (parser.eof() )
    return false;

  label = pitem.label();
  return true;
}

} // end of anonymous namespace

SbMenu *MenuCreator::createMenu(const std::string& label, BScreen& screen) {
  tk::Layer* layer = screen.layerManager().getLayer((int)tk::ResLayers_e::MENU);
  SbMenu *menu = new SbMenu(screen.menuTheme(), screen.imageControl(), *layer);
  if (!label.empty() )
    menu->setLabel(label);
  return menu;
}

SbMenu *MenuCreator::createMenu(const string &label, int screen_number) {
  BScreen *screen = Shynebox::instance()->findScreen(screen_number);
  return MenuCreator::createMenu(label, *screen);
}

bool MenuCreator::createFromFile(const string &filename,
                                 tk::Menu &inject_into,
                                 AutoReloadHelper *reloader, bool begin) {
  string real_filename = tk::StringUtil::expandFilename(filename);

  SbMenuParser parser(real_filename);
  if (!parser.isLoaded() )
    return false;

  startFile();
  if (begin) {
    string label;
    if (!getStart(parser, label, s_stringconvertor) ) {
      endFile();
      return false;
    }
    inject_into.setLabel(label);
  }

  // save menu filename, so we can check if it changes
  if (reloader)
    reloader->addFile(real_filename);

  parseMenu(parser, inject_into, s_stringconvertor, reloader);
  endFile();

  return true;
}

bool MenuCreator::createWindowMenuItem(const string &type,
                                       const string &label,
                                       tk::Menu &menu) {
  int screen = menu.screenNumber();

  if (type == "shade") {
    menu.insertItem(new WindowMenuAccessor(_l(label, L_SHADE),
                              &ShyneboxWindow::isShaded, &ShyneboxWindow::setShaded) );
  } else if (type == "maximize") {
    tk::Command<void> *maximize_cmd(new WindowCmd<void>(&ShyneboxWindow::maximizeFull) );
    menu.insertCommand(_l(label, L_MAXIMIZE), *maximize_cmd);
    // maximize wont work like shade/sticky/iconic, needs void(bool) func .-.
  } else if (type == "iconify") {
    menu.insertItem(new WindowMenuAccessor(_l(label, L_ICONIFY),
                              &ShyneboxWindow::isIconic, &ShyneboxWindow::setIconic) );
  } else if (type == "close") {
    tk::Command<void> *close_cmd(new WindowCmd<void>(&ShyneboxWindow::close) );
    menu.insertCommand(_l(label, L_CLOSE), *close_cmd);
  } else if (type == "kill" || type == "killwindow") {
    tk::Command<void> *kill_cmd(new WindowCmd<void>(&ShyneboxWindow::kill) );
    menu.insertCommand(_l(label, L_KILL), *kill_cmd);
  } else if (type == "lower") {
    tk::Command<void> *lower_cmd(new WindowCmd<void>(&ShyneboxWindow::lower) );
    menu.insertCommand(_l(label, L_LOWER), *lower_cmd);
  } else if (type == "raise") {
    tk::Command<void> *raise_cmd(new WindowCmd<void>(&ShyneboxWindow::raise) );
    menu.insertCommand(_l(label, L_RAISE), *raise_cmd);
  } else if (type == "stick") {
    menu.insertItem(new WindowMenuAccessor(_l(label, L_STICK),
                              &ShyneboxWindow::isStuck, &ShyneboxWindow::setStuck) );
  } else if (type == "settitledialog") {
    tk::Command<void> *setname_cmd(new SetTitleDialogCmd() );
    menu.insertCommand(_l(label, L_TITLE), *setname_cmd);
  } else if (type == "remember" || type == "extramenus") {
    BScreen* s = Shynebox::instance()->findScreen(screen);
    menu.insertSubmenu(_l("", L_REMEMBER), Remember::createMenu(*s) );
  } else if (type == "sendto") {
    SbMenu *sendto = new SendToMenu(*Shynebox::instance()->findScreen(screen) );
    menu.insertSubmenu(_l(label, L_SENDTO), sendto);
  } else if (type == "layer") {
    static LayerMenuAccessor lm_obj;

    BScreen* s = Shynebox::instance()->findScreen(screen);
    tk::Menu *submenu = new LayerMenu(s->menuTheme(),
                                        s->imageControl(),
                                        *(s->layerManager() ).getLayer((int)tk::ResLayers_e::MENU),
                                        &lm_obj,
                                        false);
    submenu->disableTitle();
    menu.insertSubmenu(_l(label, L_LAYER), submenu);
  } else if (type == "separator")
    menu.insertItem(new tk::MenuSeparator() );
  else
    return false;
  // if-chain window menu items
  return true;
} // createWindowMenuItem

// Copyright (c) 2023 Shynebox - zlice
//
// Copyright (c) 2004-2008 Henrik Kinnunen (fluxgen at fluxbox dot org)
//                and Simon Bowden    (rathnor at users.sourceforge.net)
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
