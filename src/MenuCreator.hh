// MenuCreator.hh for Shynebox Window Manager

/*
  Creates menus from scratch or from config files
  e.g. 'windowmenu'(titlebar) or 'menu'(rootmenu/desktop)
  Uses SbMenuParser to parse files then creates objects.
*/

#ifndef MENUCREATOR_HH
#define MENUCREATOR_HH

#include <string>

namespace tk {
class AutoReloadHelper;
class Menu;
}

class SbMenu;
class ShyneboxWindow;
class BScreen;
class SendToMenu;

namespace MenuCreator {
  SbMenu* createMenu(const std::string& label, BScreen& screen);
  SbMenu* createMenu(const std::string& label, int screen_num);
  bool createFromFile(const std::string &filename,
                      tk::Menu &inject_into,
                      tk::AutoReloadHelper *reloader = NULL,
                      bool begin = true);
  bool createWindowMenuItem(const std::string &type, const std::string &label,
                            tk::Menu &inject_into);
};

#endif // MENUCREATOR_HH

// Copyright (c) 2023 Shynebox - zlice
//
// Copyright (c) 2004 Henrik Kinnunen (fluxgen at fluxbox dot org)
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
