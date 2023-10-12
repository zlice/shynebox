// RootTheme.cc for Shynebox Window Manager

#include "RootTheme.hh"

#include "defaults.hh"
#include "SbRootWindow.hh"
#include "SbCommands.hh"
#include "Screen.hh"

#include "tk/App.hh"
#include "tk/Font.hh"
#include "tk/Image.hh"
#include "tk/ImageControl.hh"
#include "tk/Config.hh"
#include "tk/FileUtil.hh"
#include "tk/StringUtil.hh"
#include "tk/TextureRender.hh"
#include "tk/I18n.hh"

#include <X11/Xatom.h>

#include <sys/types.h>
#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif

#ifdef HAVE_CSTRING
  #include <cstring>
#else
  #include <string.h>
#endif

using std::string;

class BackgroundItem: public tk::ThemeItem<tk::Texture> {
public:
  BackgroundItem(tk::Theme &tm, const string &name):
      tk::ThemeItem<tk::Texture>(tm, name),
      m_changed(false), m_loaded(false)
      { }

  void load(const string *o_name = 0) {
    const string &m_name = (o_name == 0) ? name() : *o_name;

    // if we got this far, then the background was loaded
    m_loaded = true;

    // create subnames
    string color_name(tk::ThemeManager::instance().
                      resourceValue(m_name + ".color") );
    string colorto_name(tk::ThemeManager::instance().
                        resourceValue(m_name + ".colorTo") );
    string pixmap_name(tk::ThemeManager::instance().
                       resourceValue(m_name + ".pixmap") );
    string mod_x(tk::ThemeManager::instance().
                 resourceValue(m_name + ".modX") );
    string mod_y(tk::ThemeManager::instance().
                 resourceValue(m_name + ".modY") );

    // validate mod_x and mod_y
    if (mod_x.length() > 2)
      mod_x.erase(2,mod_x.length() ); // shouldn't be longer than 2 digits
    if (mod_y.length() > 2)
      mod_y.erase(2,mod_y.length() ); // ditto
    // should be integers
    if (!mod_x.length() || mod_x[0] < '0' || mod_x[0] > '9'
        || (mod_x.length() == 2 && (mod_x[1] < '0' || mod_x[1] > '9') ) )
      mod_x = "1";
    if (!mod_y.length() || mod_y[0] < '0' || mod_y[0] > '9'
        || (mod_y.length() == 2 && (mod_y[1] < '0' || mod_y[1] > '9') ) )
      mod_y = "1";

    // remove whitespace from filename
    tk::StringUtil::removeFirstWhitespace(pixmap_name);
    tk::StringUtil::removeTrailingWhitespace(pixmap_name);

    // check if the background has been changed
    if (mod_x != m_mod_x || mod_y != m_mod_y || pixmap_name != m_filename
        || color_name != m_color || colorto_name != m_color_to) {
      m_changed = true;
      m_mod_x = mod_x;
      m_mod_y = mod_y;
      m_filename = pixmap_name;

      // these aren't quite right because of defaults set below
      m_color = color_name;
      m_color_to = colorto_name;
    }

    // set default value if we failed to load colors
    if (!(*this)->color().setFromString(color_name.c_str(),
                                        theme().screenNum() ) )
      (*this)->color().setFromString("darkgray", theme().screenNum() );

    if (!(*this)->colorTo().setFromString(colorto_name.c_str(),
                                          theme().screenNum() ) )
      (*this)->colorTo().setFromString("white", theme().screenNum() );


    if (((*this)->type() & tk::Texture::SOLID) != 0
        && ((*this)->type() & tk::Texture::FLAT) == 0)
      (*this)->calcHiLoColors(theme().screenNum() );

    // we dont load any pixmap, using external command to set background pixmap
    (*this)->pixmap() = 0;
  } // load

  void setFromString(const char *str) {
    m_options = str; // save option string
    tk::ThemeItem<tk::Texture>::setFromString(str);
  }
  const string &filename() const { return m_filename; }
  const string &options() const { return m_options; }
  const string &colorString() const { return m_color; }
  const string &colorToString() const { return m_color_to; }
  const string &modX() const { return m_mod_x; }
  const string &modY() const { return m_mod_y; }
  bool changed() const { return m_changed; }
  bool loaded() const { return m_loaded; }
  void setApplied() { m_changed = false; }
  void unsetLoaded() { m_loaded = false; }
private:
  string m_filename, m_options;
  string m_color, m_color_to;
  string m_mod_x, m_mod_y;
  bool m_changed, m_loaded;
}; // class BackgroundItem

RootTheme::RootTheme(tk::ImageControl &image_control):
    tk::Theme(image_control.screenNumber() ),
    m_background(new BackgroundItem(*this, "background") ),
    m_opgc(RootWindow(tk::App::instance()->display(), image_control.screenNumber() ) ),
    m_first(true) {
  Display *disp = tk::App::instance()->display();
  m_opgc.setForeground(WhitePixel(disp, screenNum() )^BlackPixel(disp, screenNum() ) );
  m_opgc.setFunction(GXxor);
  m_opgc.setSubwindowMode(IncludeInferiors);
  tk::ThemeManager::instance().loadTheme(*this);
} // RootTheme class init

RootTheme::~RootTheme() {
  delete m_background;
} // RootTheme class destroy

bool RootTheme::fallback(tk::ThemeItem_base &item) {
  // if background theme item was not found in the
  // style then mark background as not loaded so
  // we can deal with it in reconfigureTheme()
  if (item.name() == "background") {
    m_background->unsetLoaded();
    return true;
    // mark no background loaded
  }
  return false;
}

void RootTheme::reconfigTheme() {
  if (!m_background->loaded()
      || (!m_first && !m_background->changed() ) )
    return;

  m_background->setApplied();

  // handle background option in style
  string filename = m_background->filename();
  tk::StringUtil::removeTrailingWhitespace(filename);
  tk::StringUtil::removeFirstWhitespace(filename);

  // if background argument is a file then
  // parse image options and call image setting
  // command specified in the resources
  string img_path = tk::Image::locateFile(filename);
  filename = tk::StringUtil::expandFilename(filename);
  string cmd = realProgramName("sbsetbg") + (m_first ? " -z " : " -Z ");

  // user explicitly requests NO background be set at all
  if (strstr(m_background->options().c_str(), "unset") != 0)
    return;
  // style doesn't wish to change the background
  if (strstr(m_background->options().c_str(), "none") != 0) {
    if (!m_first)
      return;
  } else if (!img_path.empty() ) {
    // parse options
    if (strstr(m_background->options().c_str(), "tiled") != 0)
      cmd += "-t ";
    else if (strstr(m_background->options().c_str(), "centered") != 0)
      cmd += "-c ";
    else if (strstr(m_background->options().c_str(), "aspect") != 0)
      cmd += "-a ";
    else
      cmd += "-f ";
    cmd += img_path;
  } else if (tk::FileUtil::isDirectory(filename.c_str() )
             && strstr(m_background->options().c_str(), "random") != 0) {
    cmd += "-r " + filename;
  } else {
    cmd += "-b "; // render normal texture with sbsetroot

    // Make sure the color strings are valid,
    // so we dont pass any `commands` that can be executed
    bool color_valid =
        tk::Color::validColorString(m_background->colorString().c_str(),
                                      screenNum() );
    bool color_to_valid =
        tk::Color::validColorString(m_background->colorToString().c_str(),
                                      screenNum() );

    string options;
    if (color_valid)
      cmd += "-foreground '" + m_background->colorString() + "' ";
    if (color_to_valid)
      cmd += "-background '" + m_background->colorToString() + "' ";

    if (strstr(m_background->options().c_str(), "mod") != 0)
      cmd += "-mod " + m_background->modX() + " " + m_background->modY();
    else if ((*m_background)->type() & tk::Texture::SOLID && color_valid)
      cmd += "-solid '" + m_background->colorString() + "' ";
    else if ((*m_background)->type() & tk::Texture::GRADIENT) {
      // remove whitespace from the options, since sbsetroot doesn't care
      // and dealing with sh and sbsetbg is impossible if we don't
      string options = m_background->options();
      options = tk::StringUtil::replaceString(options, " ", "");
      options = tk::StringUtil::replaceString(options, "\t", "");
      cmd += "-gradient " + options;
    }
  } // if-chain background options str

  // call command with options
  SbCommands::ExecuteCmd exec(cmd, screenNum() );
  m_first = false;
  exec.execute();
} // reconfigTheme

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
