// Theme.cc for Shynebox Window Manager

#include "Theme.hh"

#include "StringUtil.hh"
#include "FileUtil.hh"
#include "I18n.hh"
#include "Image.hh"

#ifdef HAVE_CSTDIO
  #include <cstdio>
#else
  #include <stdio.h>
#endif
#include <iostream>
#include <fstream>

using std::cerr;
using std::ifstream;
using std::ofstream;
using std::string;

namespace tk {

Theme::Theme(int screen_num):m_screen_num(screen_num) {
  ThemeManager::instance().registerTheme(*this);
} // Theme class init

Theme::~Theme() {
  ThemeManager::instance().unregisterTheme(*this);
} // Theme class destroy

ThemeManager &ThemeManager::instance() {
  static ThemeManager tm;
  return tm;
}

ThemeManager::ThemeManager():
  m_verbose(false),
  m_themelocation("") { }
// ThemeManager class init

bool ThemeManager::registerTheme(Theme &tm) {
  for (auto it : m_themes)
    if (it == &tm)
      return false;

  m_themes.push_back(&tm);
  return true;
}

bool ThemeManager::unregisterTheme(Theme &tm) {
  m_themes.remove(&tm);
  return true;
}

bool ThemeManager::load_file(const string &filename, thm_str_map &tmap) {
  ifstream tmp_file(filename.c_str() );
  string line;
  thm_str_map wildcards; // for "*font:

  if (!tmp_file) {
    cerr << "Error opening theme file!!! ("<< filename << ")\n";
    return false;
  }

  while (getline(tmp_file, line) ) {
    if (line.empty() || line[0] == '!' || line[0] == '#')
      continue; // skip comments

    string key;
    string val;
    size_t colon_pos = line.find(':');

    if (colon_pos != string::npos) {
      key = line.substr(0, colon_pos);
      key = StringUtil::toLower(key);

      val = line.substr(colon_pos+1);
      StringUtil::removeFirstWhitespace(val);
      StringUtil::removeTrailingWhitespace(val);
      //cerr << "          DEBUG: key : " << key << "\n";
      //cerr << "                 val : " << val << "\n";

      // handle wildcard values
      size_t star_pos = key.find('*');
      if (star_pos != string::npos) {
        wild_map[key] = val;
        continue; // skip main theme add, only add to wilds
      }
    } else {
      cerr << "Warning: Error loading theme line : ("<< line << ")\n";
      continue;
    }

    tmap[key] = val;
  } // while getline()

  return true;
}

bool ThemeManager::load(const string &filename,
                        const string &overlay_filename) {
  string location = StringUtil::expandFilename(filename);
  StringUtil::removeTrailingWhitespace(location);
  StringUtil::removeFirstWhitespace(location);
  string prefix = "";

  theme_map.clear(); // delete any lingering style items
  wild_map.clear();  // for running-reloads

  if (FileUtil::isDirectory(location.c_str() ) ) {
    prefix = location;

    location.append("/theme.cfg");
    if (!FileUtil::isRegularFile(location.c_str() ) ) {
      location = prefix;
      location.append("/style.cfg");
      if (!FileUtil::isRegularFile(location.c_str() ) ) {
        cerr << "Error loading theme file (" << location << ") not a regular file\n";
        return false;
      }
    }
  } else // dirname
    prefix = location.substr(0, location.find_last_of('/') );

  if (!load_file(location.c_str(), theme_map) ) {
    cerr << "Exiting because of theme/style error. Change your theme"
            " or check for permissions and drive issues.\n";
    return false;
  }

  thm_str_map overlay_map;

  if (!overlay_filename.empty() ) {
    string overlay_location = StringUtil::expandFilename(overlay_filename);
    if (FileUtil::isRegularFile(overlay_location.c_str() )
        && !load_file(overlay_location.c_str(), overlay_map) )
      cerr << "Warning: overlay file (" << overlay_location << ") not readable.\n";
  }

  // overwrite any theme setting with overlay's
  for (auto [k, v] : overlay_map)
    theme_map[k] = v;

  // relies on the fact that load_rc clears search paths each time
  if (m_themelocation != "") {
    Image::removeSearchPath(m_themelocation);
    m_themelocation.append("/pixmaps");
    Image::removeSearchPath(m_themelocation);
  }

  m_themelocation = prefix;

  location = prefix;
  Image::addSearchPath(location);
  location.append("/pixmaps");
  Image::addSearchPath(location);

  for (auto &it : m_themes)
    ThemeManager::instance().loadTheme(*it);

  return true;
} // ThemeManager load

void ThemeManager::loadTheme(Theme &tm) {
  // send reconfiguration signal to theme and listeners
  for (auto i : tm.itemList() ) {
    ThemeItem_base *resource = i;
    if (!loadItem(*resource) ) {
      // try fallback resource in theme
      if (!tm.fallback(*resource) ) {
        if (verbose() ) {
          _SB_USES_NLS;
          cerr << _TK_CONSOLETEXT(Error, ThemeItem, "Failed to read theme item",
               "When reading a style, couldn't read a specific item (following)")
               << ": " << resource->name() <<"\n";
        }
        resource->setDefaultValue();
      }
    } // if loadItem
  } // for itemList
} // ThemeManager loadTheme

bool ThemeManager::loadItem(ThemeItem_base &resource) {
  return loadItem(resource, resource.name() );
}

// for compatibility and because of how fallbacks work
// there are two search/wildcard scan stages.
// 1 - fallbacks - DONT init map value with "" (this)
//                 and keep searching fallbacks first
// 2 - final values - config value or wildcard match
//                 else ThemeItem sets defaults
//
// handles resource item loading with specific names
bool ThemeManager::loadItem(ThemeItem_base &resource, const string &name) {
  string lower_name = StringUtil::toLower(name);

  // check if proper 'toolbar.systemtray.color' exist
  // then 'toolbar.*.color'
  // then (from loadTheme loop) fallback 'toolbar.clock.color'
  string val = resourceValue(lower_name);

  if (val == "" && theme_map.count(lower_name) == 0)
    return false;

  resource.setFromString(val.c_str() );
  resource.load(&lower_name); // load additional stuff by the ThemeItem

  return true;
}

string ThemeManager::resourceValue(const string &name) {
  string lower_name = StringUtil::toLower(name);

  if (theme_map.count(lower_name) == 0) {
    string val = "";
    for (auto [k,v] : wild_map) {
      // split key string
      string s_wild, e_wild; // start/end for "*font" or "menu*font"
      size_t star_pos = k.find('*');
      s_wild = k.substr(0, star_pos); // menu*font -> menu
      s_wild = StringUtil::toLower(s_wild);

      e_wild = k.substr(star_pos+1); // menu*font -> font
      e_wild = StringUtil::toLower(e_wild);

      size_t s_len = s_wild.length(), e_len = e_wild.length();

      // c++20 has start/ends_with -.- rufkm?
      // there HAS to be an end match. toolbar* does nothing
      if (e_len > 0
          && name.find(e_wild.c_str(), name.length() - e_len - 1, e_len) != string::npos) {
        if (s_len == 0)
          val = v; // only end to match, maybe *font
        else if (name.find(s_wild.c_str(), (size_t)0, s_len) == 0) {
          val = v; // found*match
          break; // matched before*after so this is good
        }
      }
    } // for wildcards
    return val;
  } // if missing

  return theme_map[lower_name]; // init empty "" value
}

} // end namespace tk

// Copyright (c) 2023 Shynebox - zlice
//
// Copyright (c) 2002 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
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
