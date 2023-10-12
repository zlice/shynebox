// Keys.hh for Shynebox Window Manager

/*
  Reads in and sets up window manager commands of key and mouse combinations.
  Also sets up basic focus event for mouse click raising/focusing windows.
  (registerWindow in ShyneboxWindow)

  How this works:
    Man page overview:
      - Keymodes are 'layers' that key *bindings* are held in.
        Typically the 1 'default' mode is enough, but multiple modes can
        also be switched to which will temporaily only allow bindings in
        THAT keymode/layer.
      - Bindings are the hotkey/mouse combinations that runs commands (actions)
      - Commands/actions are the functions ran

      TestMode: Mod4 k :Close
      ^keymode    ^         ^
                  |-binding |
                            |-action

    Type definitions:
      t_key      - parsed values of 'Mod4 k :CmdHere'
                   also holds a keylist
      keylist    - list of t_key
      keymodes   - string put in map<first,second> 'first', TestMode in this case
                   'TestMode: KeysHere :CmdHere'
      keyspace_t - map of all keymodes

  A keymap <keymode, t_key*> (essentially <string, keylist>) is created.

  addBinding() parses sets up the keymap per config line.

  doAction() searches the current keymap[keymode] t_key(and keylist)
  for matches then executes the configured command (action).

*/

#ifndef KEYS_HH
#define KEYS_HH

#include "tk/NotCopyable.hh"

#include <X11/Xlib.h>
#include <string>
#include <map>

class WinClient;

namespace tk {
    class EventHandler;
    class AutoReloadHelper;
}

class Keys: private tk::NotCopyable  {
public:

  // contexts for events
  // eventHandlers should submit bitwise-or of contexts the event source
  enum {
      GLOBAL =            1 << 0,
      ON_DESKTOP =        1 << 1,
      ON_TOOLBAR =        1 << 2,
      ON_ICONBUTTON =     1 << 3,
      ON_TITLEBAR =       1 << 4,
      ON_WINDOW =         1 << 5,
      ON_WINDOWBORDER =   1 << 6,
      ON_LEFTGRIP =       1 << 7,
      ON_RIGHTGRIP =      1 << 8,
      ON_TAB =            1 << 9,
      ON_WINBUTTON =      1 << 10,
      ON_MINBUTTON =      1 << 11,
      ON_MAXBUTTON =      1 << 12
  };

  explicit Keys();
  ~Keys();

  bool addBinding(const std::string &binding);
  bool doAction(int type, unsigned int mods, unsigned int key, int context,
                WinClient *current = 0, Time time = 0);

  // register a window so that proper keys/buttons get grabbed on it
  void registerWindow(Window win, tk::EventHandler &handler, int context);
  void unregisterWindow(Window win);

  // reset current key mode/map
  // this is called when MappingNotify hits the main WM event handler
  // see Shynebox init() key reloader and timer
  void regrab();
  // reset key maps and load configuration from file
  void reload();
  // calls reload when WM reconfigures
  void reconfigure();

  const std::string& filename() const { return m_filename; }
  void keyMode(const std::string& keyMode);
  bool inKeychain() const { return saved_keymode != 0; }

private:
  class t_key; // holds key mods, key ,context, etc
  typedef std::map<std::string, t_key*> keyspace_t;
  typedef std::map<Window, int> WindowMap;
  typedef std::map<Window, tk::EventHandler*> HandlerMap;

  void deleteTree();

  void grabKey(unsigned int key, unsigned int mod);
  void ungrabKeys();
  void grabButton(unsigned int button, unsigned int mod, int context);
  void ungrabButtons();
  void grabWindow(Window win);

  // Load default keybindings for when there are errors loading the keys file
  void loadDefaults();
  void setKeyMode(t_key &keyMode);

  // member variables
  std::string m_filename;
  tk::AutoReloadHelper* m_reloader = 0;
  t_key *m_keylist; // "Mod4 k :CmdHere"
  keyspace_t m_map; // map of Keymodes
  t_key *next_key;
  t_key *saved_keymode;

  WindowMap m_window_map;
  HandlerMap m_handler_map;
};

#endif // KEYS_HH

// Copyright (c) 2023 Shynebox - zlice
//
// Copyright (c) 2001 - 2003 Henrik Kinnunen (fluxgen at fluxbox dot org)
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
