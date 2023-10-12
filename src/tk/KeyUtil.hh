// KeyUtil.hh for Shynebox Window Manager

/*
  X functions for grabbing keyboard keys and mouse buttons.
  Helper functions for stripping out mods and getting pointer coords.
*/

#ifndef TK_KEYUTIL_HH
#define TK_KEYUTIL_HH

#include <X11/Xlib.h>

namespace tk {

class KeyUtil {
public:
  KeyUtil();
  ~KeyUtil();

  static KeyUtil &instance();

  // Grab the specified key
  static void grabKey(unsigned int key, unsigned int mod, Window win);
  static void grabButton(unsigned int button, unsigned int mod, Window win,
                         unsigned int event_mask, Cursor cursor = None);

  // convert the string to the keysym
  static unsigned int getKey(const char *keystr);

  // the modifier for the modstr, zero on failure
  static unsigned int getModifier(const char *modstr);

  // ungrabs all keys
  static void ungrabKeys(Window win);
  static void ungrabButtons(Window win);

  // strip out modifiers we want to ignore
  unsigned int cleanMods(unsigned int mods) {
    return mods & ~(capslock() | numlock() | scrolllock() ) & ((1<<13) - 1);
    // remove numlock, capslock, and scrolllock
    // and anything beyond Button5Mask
  }

  /**
     strip away everything which is actually not a modifier
     eg, xkb-keyboardgroups are encoded as bit 13 and 14
  */
  unsigned int isolateModifierMask(unsigned int mods) {
    return mods & (ShiftMask|LockMask|ControlMask|Mod1Mask|Mod2Mask|Mod3Mask|Mod4Mask|Mod5Mask);
  }

  /**
     Convert the specified key into appropriate modifier mask
     @return corresponding modifier mask
  */
  static unsigned int keycodeToModmask(unsigned int keycode);
  int numlock() const { return m_numlock; }
  int capslock() const { return LockMask; }
  int scrolllock() const { return m_scrolllock; }

  void loadModmap();

  static void get_pointer_coords(Display *d, Window w,
                                 int &x, int &y);

private:
  XModifierKeymap *m_modmap;
  int m_numlock, m_scrolllock;
  static KeyUtil *s_keyutil;
};

} // end namespace tk

#endif // TK_KEYUTIL_HH

// Copyright (c) 2023 Shynebox - zlice
//
// Copyright (c) 2003 Henrik Kinnunen (fluxgen at fluxbox dot org)
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
