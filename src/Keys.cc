// Keys.cc for Shynebox Window Manager

#include "Keys.hh"

#include "shynebox.hh"
#include "Screen.hh"
#include "WinClient.hh"
#include "WindowCmd.hh"
#include "Debug.hh"

#include "tk/App.hh"
#include "tk/EventManager.hh"
#include "tk/StringUtil.hh"
#include "tk/Command.hh"
#include "tk/KeyUtil.hh"
#include "tk/CommandParser.hh"
#include "tk/LogicCommands.hh"
#include "tk/I18n.hh"
#include "tk/AutoReloadHelper.hh"

#ifdef HAVE_CCTYPE
  #include <cctype>
#else
  #include <ctype.h>
#endif

#ifdef HAVE_CSTDIO
  #include <cstdio>
#else
  #include <stdio.h>
#endif
#ifdef HAVE_CSTDLIB
  #include <cstdlib>
#else
  #include <stdlib.h>
#endif
#ifdef HAVE_CERRNO
  #include <cerrno>
#else
  #include <errno.h>
#endif
#ifdef HAVE_CSTRING
  #include <cstring>
#else
  #include <string.h>
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef	HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#include <X11/Xproto.h>
#include <X11/keysym.h>
#include <X11/Xutil.h>
#include <X11/XKBlib.h>

#include <iostream>
#include <fstream>

using std::cerr;
using std::string;
using std::vector;
using std::ifstream;
using std::pair;

namespace {



// BUG: HACK: !!! enforces the linking of tk/LogicCommands !!!
// silent failure otherwise and commands won't work
tk::Command<void>* link_helper = tk::IfCommand::parse("", "", false);



int extractKeyFromString(const std::string& in, const char* start_pattern, unsigned int& key) {
  int ret = 0;
  if (strstr(in.c_str(), start_pattern) != 0) {
    unsigned int tmp_key = 0;
    if (tk::StringUtil::extractNumber(in.substr(strlen(start_pattern) ), tmp_key) ) {
      key = tmp_key;
      ret = 1;
    }
  }
  return ret;
}

} // end of anonymous namespace

// class that holds a specific key-binding-to-action
class Keys::t_key {
public:
  typedef std::list<t_key*> keylist_t;

  t_key *find(int type_, unsigned int mod_, unsigned int key_,
              int context_, bool isdouble_) {
    // t_key ctor sets context_ of 0 to GLOBAL, so we must here too
    context_ = context_ ? context_ : GLOBAL;
    for (auto it : keylist) {
      if (it && it->type == type_ && it->key == key_
          && (it->context & context_) > 0
          && isdouble_ == it->isdouble
          && it->mod == tk::KeyUtil::instance().isolateModifierMask(mod_) )
        return it;
    } // for keylist

    return 0; // used to set current_key and temp_key
  } // find

  // member variables

  int type; // KeyPress or ButtonPress
  unsigned int mod;
  unsigned int key; // key code or button number
  std::string key_str; // key-symbol, needed for regrab()
  int context; // ON_TITLEBAR, etc.: bitwise-or of all desired contexts
  bool isdouble;
  tk::Command<void> *m_command = 0;

  keylist_t keylist;

t_key(int type_ = 0, unsigned int mod_ = 0, unsigned int key_ = 0,
                 const std::string &key_str_ = std::string(),
                 int context_ = 0, bool isdouble_ = false) :
    type(type_),
    mod(mod_),
    key(key_),
    key_str(key_str_),
    context(context_),
    isdouble(isdouble_),
    m_command(0) {
  context = context_ ? context_ : GLOBAL;
} // t_key class init

~t_key() {
  for (auto k : keylist)
    delete k;
  if (m_command)
    delete m_command;
} // t_key class destroy

}; // t_key

Keys::Keys():
    m_reloader(new tk::AutoReloadHelper() ),
    m_keylist(0),
    next_key(0), saved_keymode(0) {
  tk::Command<void> *reload_cmd = new tk::SimpleCommand<Keys>(*this, &Keys::reload);
  m_reloader->setReloadCmd(*reload_cmd);
} // Keys class init

Keys::~Keys() {
  ungrabKeys();
  ungrabButtons();
  deleteTree();
  if (m_reloader)
    delete m_reloader;
} // Keys class destroy

/// Destroys the keytree
void Keys::deleteTree() {
  ungrabKeys();
  ungrabButtons();
  for (auto m : m_map) {
    if (m.second == m_keylist)
      m_keylist = 0;
    delete m.second;
  }
  m_map.clear();
  if (m_keylist)
    delete m_keylist;
  m_keylist = 0;
  // saved/next keys are just pointers to the actual keylist
  next_key = 0;
  saved_keymode = 0;
}

// keys are only grabbed in global context
void Keys::grabKey(unsigned int key, unsigned int mod) {
  for (auto it : m_window_map)
    if ((it.second & Keys::GLOBAL) > 0)
      tk::KeyUtil::grabKey(key, mod, it.first);
}

// keys are only grabbed in global context
void Keys::ungrabKeys() {
  for (auto it : m_window_map)
    if ((it.second & Keys::GLOBAL) > 0)
      tk::KeyUtil::ungrabKeys(it.first);
}

// ON_DESKTOP context doesn't need to be grabbed
void Keys::grabButton(unsigned int button, unsigned int mod, int context) {
  for (auto it : m_window_map)
    if ((context & it.second & ~Keys::ON_DESKTOP) > 0)
      tk::KeyUtil::grabButton(button, mod, it.first,
                              ButtonPressMask|ButtonReleaseMask);
}

void Keys::ungrabButtons() {
  for (auto it : m_window_map)
    tk::KeyUtil::ungrabButtons(it.first);
}

void Keys::grabWindow(Window win) {
  if (!m_keylist)
    return;

  // make sure the window is in our list
  WindowMap::iterator win_it = m_window_map.find(win);
  if (win_it == m_window_map.end() )
    return;

  // orig click to focus
  m_handler_map[win]->grabButtons();
  for (auto it : m_keylist->keylist) {
    // keys are only grabbed in global context
    if ((win_it->second & Keys::GLOBAL) > 0 && it->type == KeyPress)
      tk::KeyUtil::grabKey(it->key, it->mod, win);
    // ON_DESKTOP buttons don't need to be grabbed
    else if ((win_it->second & it->context & ~Keys::ON_DESKTOP) > 0) {
      if ((it->type & (ButtonPress | ButtonRelease) ) )
        tk::KeyUtil::grabButton(it->key, it->mod, win, ButtonPressMask|ButtonReleaseMask);
    }
  }
} // grabWindow - for click to focus

// Load and grab keys from file
void Keys::reload() {
  // open the 'keys' file (shynebox + config set this up)
  // if the name or file are empty, systems probably borked
  ifstream infile(m_filename.c_str() );
  if (!infile) {
    if (m_map.empty() ) // no current keymaps
      loadDefaults(); // load something, so you're not fubar
    return; // failed to open file - back out
  }

  // free memory of previous grabs
  deleteTree();

  m_map["default:"] = new t_key;

  size_t current_line = 0,
         parsed_cnt = 0;

  while (!infile.eof() ) {
    string linebuffer;
    getline(infile, linebuffer);
    current_line++;

    if (!addBinding(linebuffer) ) {
      _SB_USES_NLS;
      cerr << _SB_CONSOLETEXT(Keys, InvalidKeyMod,
              "Keys: Invalid key/modifier on line",
              "A bad key/modifier string was found on line (number following)")
           <<" "<< current_line<<"): "<<linebuffer<<"\n";
    } else
      parsed_cnt++;
  } // end while eof

  if (parsed_cnt == 0 && current_line == 0)
    loadDefaults(); // empty file somehow, may have corruption?
    // only comments will still allow 0 binds in case that's desired

  keyMode("default");
} // reload

// Load critical key/mouse bindings for when there are fatal errors reading the keyFile.
void Keys::loadDefaults() {
    deleteTree();
    m_map["default:"] = new t_key;
    addBinding("OnDesktop Mouse1 :HideMenus");
    addBinding("OnDesktop Mouse2 :WorkspaceMenu");
    addBinding("OnDesktop Mouse3 :RootMenu");
    addBinding("OnTitlebar Mouse3 :WindowMenu");
    addBinding("OnTitlebar Mouse1 :MacroCmd {Focus} {Raise} {ActivateTab} {StartMoving}");
    addBinding("OnLeftGrip Move1 :StartResizing bottomleft");
    addBinding("OnRightGrip Move1 :StartResizing bottomright");
    addBinding("OnWindowBorder Move1 :StartMoving");
    addBinding("Mod1 Tab :NextWindow (workspace=[current])");
    addBinding("Mod1 Shift Tab :PrevWindow (workspace=[current])");
    addBinding("Control Mod1 Delete :Exit");
    addBinding("Shift Control r :Restart");
    keyMode("default");
}

bool Keys::addBinding(const string &linebuffer) {
  // Parse arguments
  vector<string> val;
  tk::StringUtil::stringtok(val, linebuffer);

  // must have at least 1 argument
  if (val.empty() )
    return true; // empty lines are valid.

  if (val[0][0] == '#' || val[0][0] == '!' ) //the line is commented
    return true; // still a valid line.

  unsigned int key = 0, mod = 0;
  int type = 0, context = 0;
  bool isdouble = false;
  size_t argc = 0;
  t_key *current_key = m_map["default:"];
  t_key *first_new_keylist = m_map["default:"], *first_new_key = 0;

  if (val[0][val[0].length()-1] == ':') {
    argc++;
    keyspace_t::iterator it = m_map.find(val[0] );
    if (it == m_map.end() )
      m_map[val[0] ] = new t_key;
    current_key = m_map[val[0] ];
  }

  // for each argument
  for (; argc < val.size(); argc++) {
    std::string arg = tk::StringUtil::toLower(val[argc]);

    if (arg[0] != ':') { // parse key(s)
      std::string key_str;

      int tmpmod = tk::KeyUtil::getModifier(arg.c_str() );
      if (tmpmod)
          mod |= tmpmod; // If it's a modifier
      else if (arg == "ondesktop")
          context |= ON_DESKTOP;
      else if (arg == "onwindow")
          context |= ON_WINDOW;
      else if (arg == "ontitlebar")
          context |= ON_TITLEBAR;
      else if (arg == "onwinbutton")
          context |= ON_WINBUTTON;
      else if (arg == "onminbutton")
          context |= ON_MINBUTTON;
      else if (arg == "onmaxbutton")
          context |= ON_MAXBUTTON;
      else if (arg == "onwindowborder")
          context |= ON_WINDOWBORDER;
      else if (arg == "onleftgrip")
          context |= ON_LEFTGRIP;
      else if (arg == "onrightgrip")
          context |= ON_RIGHTGRIP;
      else if (arg == "ontab")
          context |= ON_TAB;
      else if (arg == "double")
          isdouble = true;
      else if (arg != "none") {
          if (arg == "focusin") {
              context = ON_WINDOW;
              mod = key = 0;
              type = FocusIn;
          } else if (arg == "focusout") {
              context = ON_WINDOW;
              mod = key = 0;
              type = FocusOut;
          } else if (arg == "changeworkspace") {
              context = ON_DESKTOP;
              mod = key = 0;
              type = FocusIn;
          } else if (arg == "mouseover") {
              type = EnterNotify;
              if (!(context & (ON_WINDOW) ) )
                context |= ON_WINDOW;
              key = 0;
          } else if (arg == "mouseout") {
              type = LeaveNotify;
              if (!(context & (ON_WINDOW) ) )
                context |= ON_WINDOW;
              key = 0;

          // check if it's a mouse button
          } else if (extractKeyFromString(arg, "mouse", key) ) {
              type = ButtonPress;

              // TODO: DELETE ? old tool and i'm not sure this is valid now
              // fluxconf mangles things like OnWindow Mouse# to Mouse#ow
              if (strstr(arg.c_str(), "top") )
                  context = ON_DESKTOP;
              else if (strstr(arg.c_str(), "ebar") )
                  context = ON_TITLEBAR;
              else if (strstr(arg.c_str(), "ow") )
                  context = ON_WINDOW;
          } else if (extractKeyFromString(arg, "click", key) ) {
              type = ButtonRelease;
          } else if ((key = tk::KeyUtil::getKey(val[argc].c_str() ) ) ) { // convert from string symbol
              type = KeyPress;
              key_str = val[argc];

          // keycode covers the following three two-byte cases:
          // 0x       - hex
          // +[1-9]   - number between +1 and +9
          // numbers 10 and above
          //
          } else {
            tk::StringUtil::extractNumber(arg, key);
            type = KeyPress;
          }

          if (key == 0 && (type == KeyPress || type == ButtonPress || type == ButtonRelease) )
            return false;

          if (type != ButtonPress)
            isdouble = false;

          if (!first_new_key) { // means no first_new_key made
            first_new_keylist = current_key;
            current_key = current_key->find(type, mod, key, context, isdouble);
            if (!current_key) { // is new entry
              // these are added to key list, dont delete here
              first_new_key = new t_key(type, mod, key, key_str, context, isdouble);
              current_key = first_new_key; // this should stay until next 'line'
            } else if (current_key->m_command)
              return false; // already being used
          } else {
            t_key *temp_key(new t_key(type, mod, key, key_str, context, isdouble) );
            current_key->keylist.push_back(temp_key);
            current_key = temp_key;
            // first_new_key may be unused...so far
          }
          mod = 0;
          key = 0;
          context = 0;
          isdouble = false;
      } // ! keymode ":"
    } else { // parse command line, non-keybind
      if (!first_new_key)
        return false;

      const char *str = tk::StringUtil::strcasestr(linebuffer.c_str(), val[argc].c_str() );
      if (str) { // +1 to skip ':'
        if (current_key->m_command)
          delete current_key->m_command;
        current_key->m_command = tk::CommandParser<void>::instance().parse(str + 1);
      }
      if (!str || current_key->m_command == 0 || mod)
        return false;

      // success
      first_new_keylist->keylist.push_back(first_new_key);
      return true;
    }  // end if
  } // end for each arg

  return false;
} // addBinding

// return true if bound to a command, else false
bool Keys::doAction(int type, unsigned int mods, unsigned int key,
                    int context, WinClient *current, Time time) {
  if (!m_keylist)
    return false;

  static Time first_key_time = 0;

  static Time last_button_time = 0;
  static unsigned int last_button = 0;

  // need to remember whether or not this is a double-click, e.g. when
  // double-clicking on the titlebar when there's an OnWindow Double command
  // we just don't update it if timestamp is the same
  static bool double_click = false;

  // actual value used for searching
  bool isdouble = false;

  if (type == ButtonPress) {
    if (time > last_button_time) {
      double_click = (time - last_button_time <
          Shynebox::instance()->getDoubleClickInterval() )
          && last_button == key;
    }
    last_button_time = time;
    last_button = key;
    isdouble = double_click;
  }

  if (type == KeyPress && first_key_time && time - first_key_time > 5000) {
    if (saved_keymode)
      setKeyMode(*saved_keymode);
    first_key_time = 0;
    next_key = saved_keymode = 0;
  }

  if (!next_key)
    next_key = m_keylist;

  mods = tk::KeyUtil::instance().cleanMods(mods);

  // temp_key ptr should not be deleted, should never return something 'new'
  t_key *temp_key = next_key->find(type, mods, key, context, isdouble);

  // just because we double-clicked doesn't mean
  // we shouldn't look for single click commands
  if (temp_key == 0 && isdouble)
    temp_key = next_key->find(type, mods, key, context, false);

  if (temp_key && !temp_key->keylist.empty() ) { // emacs-style
    if (!saved_keymode) {
      first_key_time = time;
      saved_keymode = m_keylist;
    }
    next_key = temp_key;
    setKeyMode(*next_key);
    return true;
  }

  if (temp_key == 0 || temp_key->m_command == 0) {
    if (type == KeyPress
        && !tk::KeyUtil::instance().keycodeToModmask(key) ) {
      if (saved_keymode)
        setKeyMode(*saved_keymode);
      first_key_time = 0;
      next_key = saved_keymode = 0;
    }
     // if we're in the middle of an emacs-style keychain, exit it
    return false;
  }

  // if focus changes, windows will get NotifyWhileGrabbed,
  // which they tend to ignore
  if (type == KeyPress)
    XUngrabKeyboard(Shynebox::instance()->display(), CurrentTime);

  WinClient *old = WindowCmd<void>::client();
  WindowCmd<void>::setClient(current);

  temp_key->m_command->execute(); // checks above mean this SHOULD have cmd+exec
  WindowCmd<void>::setClient(old);

  if (saved_keymode) {
    if (next_key == m_keylist) // don't reset keymode if command changed it
      setKeyMode(*saved_keymode);
    saved_keymode = 0;
  }
  next_key = 0;
  return true;
} // doAction

/// adds the window to m_window_map, so we know to grab buttons on it
void Keys::registerWindow(Window win, tk::EventHandler &h, int context) {
  m_window_map[win] = context;
  m_handler_map[win] = &h;
  grabWindow(win);
}

/// remove the window from the window map, probably being deleted
void Keys::unregisterWindow(Window win) {
  tk::KeyUtil::ungrabKeys(win);
  tk::KeyUtil::ungrabButtons(win);
  m_handler_map.erase(win);
  m_window_map.erase(win);
}

/**
 deletes the tree and load configuration
*/
void Keys::reconfigure() {
  m_filename = tk::StringUtil::expandFilename(Shynebox::instance()->getKeysFilename() );
  m_reloader->setMainFile(m_filename);
  m_reloader->checkReload();
}

void Keys::regrab() {
  setKeyMode(*m_keylist);
}

void Keys::keyMode(const string& keyMode) {
  keyspace_t::iterator it = m_map.find(keyMode + ":");
  if (it == m_map.end() )
    setKeyMode(*m_map["default:"]);
  else
    setKeyMode(*it->second);
}

void Keys::setKeyMode(t_key &keyMode) {
  ungrabKeys();
  ungrabButtons();

  // notify handlers that their buttons have been ungrabbed
  for (auto h_it : m_handler_map)
    h_it.second->grabButtons();

  for (auto it : keyMode.keylist) {
    if (it->type == KeyPress) {
      if (!it->key_str.empty() ) {
        int key = tk::KeyUtil::getKey(it->key_str.c_str() );
        it->key = key;
      }
      grabKey(it->key, it->mod);
    } else
      grabButton(it->key, it->mod, it->context);
  } // for keyMode.keylist
  m_keylist = &keyMode;
} // setKeyMode - think layers or emacs

// Copyright (c) 2023 Shynebox - zlice
//
// Copyright (c) 2001 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
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
