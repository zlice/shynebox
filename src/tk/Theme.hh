// Theme.hh for Shynebox Window Manager

/*
  Holds ThemeManager, Theme and ThemeItem<T> which is the base for any theme.

  ThemeItem<T> is a single item for various lines in a theme.cfg
  Theme is a list of ThemeItem<T>
  The ThemeManager is a single instance that loads in the actual file and
  creates a map to assign and create the Theme.

  For compatibility, theme code is mostly intact from Fluxbox. This was
  previously a Xrdb (X Resource DataBase) so there is an extra 'wildcards'
  map to preserved that compatibility.
*/

#ifndef TK_THEME_HH
#define TK_THEME_HH

#include <string>
#include <list>
#include <unordered_map>

namespace tk {

class Theme;

// Base class for ThemeItem, holds name
class ThemeItem_base {
public:
  ThemeItem_base(const std::string &name):
    m_name(name) { }
  virtual ~ThemeItem_base() { }
  virtual void setFromString(const char *str) = 0;
  virtual void setDefaultValue() = 0;
  virtual void load(const std::string *name = 0) = 0; // if it needs to load additional stuff
  const std::string &name() const { return m_name; }
private:
  std::string m_name;
};

// template ThemeItem class for basic theme items
// to use this you need to specialize setDefaultValue, setFromString and load
template <typename T>
class ThemeItem: public ThemeItem_base {
public:
  ThemeItem(tk::Theme &tm, const std::string &name);
  virtual ~ThemeItem();
  void setDefaultValue();
  virtual void setFromString(const char *strval);
  virtual void load(const std::string *name = 0);

  T& operator*() { return m_value; }
  const T& operator*() const { return m_value; }
  T *operator->() { return &m_value; }
  const T *operator->() const { return &m_value; }

  tk::Theme &theme() { return m_tm; }
private:
  T m_value;
  tk::Theme &m_tm;
};

// Hold ThemeItems. Use this to create a Theme set
class Theme {
public:
  typedef std::list<ThemeItem_base *> ItemList;

  explicit Theme(int screen_num); // create a theme for a specific screen
  virtual ~Theme();
  virtual void reconfigTheme() = 0;
  int screenNum() const { return m_screen_num; }
  ItemList &itemList() { return m_themeitems; }
  const ItemList &itemList() const { return m_themeitems; }
  // add ThemeItem
  template <typename T>
  void add(ThemeItem<T> &item);
  // remove ThemeItem
  template <typename T>
  void remove(ThemeItem<T> &item);
  virtual bool fallback(ThemeItem_base &) { return false; }
private:
  const int m_screen_num; // for X internals
  ItemList m_themeitems;
};

// Proxy interface for themes, so they can be substituted dynamically
template <class BaseTheme>
class ThemeProxy {
public:
  virtual ~ThemeProxy() { }

  virtual BaseTheme &operator *() = 0;
  virtual const BaseTheme &operator *() const = 0;
  virtual BaseTheme *operator ->() { return &(**this); }
  virtual const BaseTheme *operator ->() const { return &(**this); }
};

// Singleton theme manager
// Use this to load all the registred themes
class ThemeManager {
public:
  typedef std::list<tk::Theme *> ThemeList;
  typedef std::unordered_map<std::string, std::string> thm_str_map;

  static ThemeManager &instance();
  // load style file "filename" to screen
  bool load_file(const std::string &filename, thm_str_map &tmap);
  bool load(const std::string &filename, const std::string &overlay_filename);
  std::string resourceValue(const std::string &name); // DO init map empty vals
  void loadTheme(Theme &tm);
  bool loadItem(ThemeItem_base &resource);
  bool loadItem(ThemeItem_base &resource, const std::string &name);

  bool verbose() const { return m_verbose; }
  void setVerbose(bool value) { m_verbose = value; }

private:
  ThemeManager();
  ~ThemeManager() { }

  friend class tk::Theme; // so only theme can register itself in constructor
  bool registerTheme(tk::Theme &tm);
  bool unregisterTheme(tk::Theme &tm);

  ThemeList m_themes;
  thm_str_map theme_map;
  thm_str_map wild_map; // for "*.font" or "toolbar.*.color"
  bool m_verbose;

  std::string m_themelocation;
};

template <typename T>
ThemeItem<T>::ThemeItem(tk::Theme &tm,
                        const std::string &name):
  ThemeItem_base(name),
  m_tm(tm) {
  tm.add(*this);
  setDefaultValue();
}

template <typename T>
ThemeItem<T>::~ThemeItem() {
  m_tm.remove(*this);
}

template <typename T>
void Theme::add(ThemeItem<T> &item) {
  m_themeitems.push_back(&item);
  m_themeitems.unique();
}

template <typename T>
void Theme::remove(ThemeItem<T> &item)  {
  m_themeitems.remove(&item);
}

} // end namespace tk

#endif // TK_THEME_HH

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
