// Remember.cc for Shynebox Window Manager

#include "Remember.hh"
#include "ClientPattern.hh"
#include "Screen.hh"
#include "Window.hh"
#include "WinClient.hh"
#include "SbMenu.hh"
#include "MenuCreator.hh"
#include "SbCommands.hh"
#include "shynebox.hh"
#include "Debug.hh"

#include "tk/I18n.hh"
#include "tk/SbString.hh"
#include "tk/StringUtil.hh"
#include "tk/FileUtil.hh"
#include "tk/MenuItem.hh"
#include "tk/AutoReloadHelper.hh"
#include "tk/Config.hh"

#include <cstring>
#include <set>

using std::cerr;
using std::string;
using std::list;
using std::set;
using std::make_pair;
using std::ifstream;
using std::ofstream;
using std::hex;
using std::dec;

using tk::StringUtil::getStringBetween;
using tk::StringUtil::removeFirstWhitespace;
using tk::StringUtil::removeTrailingWhitespace;
using tk::StringUtil::toLower;
using tk::StringUtil::toLower;
using tk::StringUtil::extractNumber;
using tk::StringUtil::expandFilename;

#define WP tk::WinProperty_e

namespace {

inline bool isComment(std::string& line) {
  removeFirstWhitespace(line);
  removeTrailingWhitespace(line);
  if (line.size() == 0 || line[0] == '#')
    return true;
  return false;
}

} // anonymous namespace

class Application {
public:
  Application(bool transient, bool grouped, ClientPattern *pat = 0);
  void reset();
  void forgetWorkspace() { workspace_remember = false; }
  void forgetHead() { head_remember = false; }
  void forgetDimensions() { dimensions_remember = false; }
  void forgetPosition() { position_remember = false; }
  void forgetShadedstate() { shadedstate_remember = false; }
  void forgetDecostate() { decostate_remember = false; }
  void forgetFocusHiddenstate() { focushiddenstate_remember= false; }
  void forgetIconHiddenstate() { iconhiddenstate_remember= false; }
  void forgetStuckstate() { stuckstate_remember = false; }
  void forgetFocusProtection() { focusprotection_remember = false; }
  void forgetJumpworkspace() { jumpworkspace_remember = false; }
  void forgetLayer() { layer_remember = false; }
  void forgetSaveOnClose() { save_on_close_remember = false; }
  void forgetMinimizedstate() { minimizedstate_remember = false; }
  void forgetMaximizedstate() { maximizedstate_remember = false; }
  void forgetFullscreenstate() { fullscreenstate_remember = false; }

  void rememberWorkspace(int ws)
      { workspace = ws; workspace_remember = true; }
  void rememberHead(int h)
      { head = h; head_remember = true; }
  void rememberDimensions(int width, int height, bool is_w_relative, bool is_h_relative) {
        dimension_is_w_relative = is_w_relative;
        dimension_is_h_relative = is_h_relative;
        w = width; h = height;
        dimensions_remember = true;
      }
  void rememberFocusHiddenstate(bool state)
      { focushiddenstate= state; focushiddenstate_remember= true; }
  void rememberIconHiddenstate(bool state)
      { iconhiddenstate= state; iconhiddenstate_remember= true; }
  void rememberPosition(int posx, int posy, bool is_x_relative, bool is_y_relative,
               ShyneboxWindow::ReferenceCorner rfc = ShyneboxWindow::LEFTTOP) {
        position_is_x_relative = is_x_relative;
        position_is_y_relative = is_y_relative;
        x = posx; y = posy;
        refc = rfc;
        position_remember = true;
      }
  void rememberShadedstate(bool state)
      { shadedstate = state; shadedstate_remember = true; }
  void rememberDecostate(unsigned int state)
      { decostate = state; decostate_remember = true; }
  void rememberStuckstate(bool state)
      { stuckstate = state; stuckstate_remember = true; }
  void rememberFocusProtection(unsigned int protect)
      { focusprotection = protect; focusprotection_remember = true; }
  void rememberJumpworkspace(bool state)
      { jumpworkspace = state; jumpworkspace_remember = true; }
  void rememberLayer(int layernum)
      { layer = layernum; layer_remember = true; }
  void rememberSaveOnClose(bool state)
      { save_on_close = state; save_on_close_remember = true; }
  void rememberMinimizedstate(bool state)
      { minimizedstate = state; minimizedstate_remember = true; }
  void rememberMaximizedstate(int state)
      { maximizedstate = state; maximizedstate_remember = true; }
  void rememberFullscreenstate(bool state)
      { fullscreenstate = state; fullscreenstate_remember = true; }

/*      if to remember               values remembered       */
  bool workspace_remember;         unsigned int workspace;
  bool decostate_remember;         unsigned int decostate;
  bool focusprotection_remember;   unsigned int focusprotection;
  bool layer_remember;             int layer;
  bool maximizedstate_remember;    int maximizedstate;
  bool head_remember;              int head;

  bool dimensions_remember,       dimension_is_w_relative,
                                  dimension_is_h_relative;
                                  int w,h; // width, height

  bool position_remember,          position_is_x_relative,
                                   position_is_y_relative;
                                   int x,y;
                      ShyneboxWindow::ReferenceCorner refc;

  bool shadedstate_remember,       shadedstate,
       stuckstate_remember,        stuckstate,
       focushiddenstate_remember,  focushiddenstate,
       iconhiddenstate_remember,   iconhiddenstate,
       jumpworkspace_remember,     jumpworkspace,
       save_on_close_remember,     save_on_close,
       minimizedstate_remember,    minimizedstate,
       fullscreenstate_remember,   fullscreenstate;

  bool is_transient, is_grouped, ignoreSizeHints_remember;

  ClientPattern *group_pattern = 0;
}; // Application class


Application::Application(bool transient, bool grouped, ClientPattern *pat):
                         is_transient(transient), is_grouped(grouped) {
  (void) pat;
  if (group_pattern)
    delete group_pattern;
  reset();
} // Application class init

void Application::reset() {
  decostate_remember =
    dimensions_remember =
    focushiddenstate_remember =
    iconhiddenstate_remember =
    jumpworkspace_remember =
    layer_remember  =
    position_remember =
    shadedstate_remember =
    stuckstate_remember =
    focusprotection_remember =
    workspace_remember =
    head_remember =
    minimizedstate_remember =
    maximizedstate_remember =
    fullscreenstate_remember =
    save_on_close_remember = false;
}

namespace {

// replace special chars like ( ) and [ ] with \( \) and \[ \]
string escapeRememberChars(const string& str) {
  if (str.empty() )
    return str;

  string escaped_str;
  escaped_str.reserve(str.capacity() );

  string::const_iterator i;
  for (i = str.begin(); i != str.end(); ++i) {
    switch (*i) {
      case '(': case ')': case '[': case ']':
        escaped_str += '\\';
      default:
        escaped_str += *i;
        break;
    }
  } // for str
  return escaped_str;
}

class RememberMenuItem : public tk::MenuItem {
public:
  RememberMenuItem(const tk::BiDiString &label,
                   const Remember::Attribute attrib) :
      tk::MenuItem(label),
      m_attrib(attrib) {
      setToggleItem(true);
      setCloseOnClick(false);
  }

  bool isSelected() const {
    if (SbMenu::window() == 0)
      return false;

    if (SbMenu::window()->numClients() ) // ensure it HAS clients
      return Remember::instance().isRemembered(SbMenu::window()->winClient(), m_attrib);
    return false;
  }

  bool isEnabled() const {
    if (SbMenu::window() == 0)
      return false;

    if (m_attrib != Remember::REM_JUMPWORKSPACE)
      return true;
    else if (SbMenu::window()->numClients() )
      return (Remember::instance().isRemembered(SbMenu::window()->winClient(), Remember::REM_WORKSPACE) );
    return false;
  }

  void click(int button, int time, unsigned int mods) {
    // reconfigure only does stuff if the apps file has changed
    Remember& r = Remember::instance();
    r.checkReload();
    if (SbMenu::window() != 0) {
      WinClient& wc = SbMenu::window()->winClient();
      if (isSelected() )
          r.forgetAttrib(wc, m_attrib);
      else
          r.rememberAttrib(wc, m_attrib);
    }
    r.save();
    tk::MenuItem::click(button, time, mods);
  }
private:
    Remember::Attribute m_attrib;
}; // RememberMenuItem

tk::Menu *createRememberMenu(BScreen &screen) {
    _SB_USES_NLS;

    static const struct { const tk::BiDiString label; Remember::Attribute attr; } _entries[] = {
        { _SB_XTEXT(Remember, Workspace, "Workspace", "Remember Workspace"), Remember::REM_WORKSPACE },
        { _SB_XTEXT(Remember, Head, "Head", "Remember Head"), Remember::REM_HEAD},
        { _SB_XTEXT(Remember, JumpToWorkspace, "Jump to workspace", "Change active workspace to remembered one on open"), Remember::REM_JUMPWORKSPACE },
        { _SB_XTEXT(Remember, Dimensions, "Dimensions", "Remember Dimensions - with width and height"), Remember::REM_DIMENSIONS},
        { _SB_XTEXT(Remember, Position, "Position", "Remember position - window co-ordinates"), Remember::REM_POSITION},
        { _SB_XTEXT(Remember, Sticky, "Sticky", "Remember Sticky"), Remember::REM_STUCKSTATE},
        { _SB_XTEXT(Remember, Decorations, "Decorations", "Remember window decorations"), Remember::REM_DECOSTATE},
        { _SB_XTEXT(Remember, Shaded, "Shaded", "Remember shaded"), Remember::REM_SHADEDSTATE},
        { _SB_XTEXT(Remember, Minimized, "Minimized", "Remember minimized"), Remember::REM_MINIMIZEDSTATE},
        { _SB_XTEXT(Remember, Maximized, "Maximized", "Remember maximized"), Remember::REM_MAXIMIZEDSTATE},
        { _SB_XTEXT(Remember, Fullscreen, "Fullscreen", "Remember fullscreen"), Remember::REM_FULLSCREENSTATE},
        { _SB_XTEXT(Remember, Layer, "Layer", "Remember Layer"), Remember::REM_LAYER},
        { _SB_XTEXT(Remember, SaveOnClose, "Save on close", "Save remembered attributes on close"), Remember::REM_SAVEONCLOSE}
    };

    // each shyneboxwindow has its own windowmenu
    // so we also create a remember menu just for it...
    tk::Menu *menu = MenuCreator::createMenu("Remember", screen);
    size_t i;
    for (i = 0; i < sizeof(_entries)/sizeof(_entries[0]); i++)
      menu->insertItem(new RememberMenuItem(_entries[i].label, _entries[i].attr) );

    menu->updateMenu();
    return menu;
} // Menu createRememberMenu

// offset is the offset in the string that we start looking from
// return true if all ok, false on error
bool handleStartupItem(const string &line, int offset) {
  Shynebox* sb = Shynebox::instance();
  unsigned int screen = sb->keyScreen()->screenNumber();
  int next = 0;
  string str;

  // accept some options, for now only "screen=NN"
  // these option are given in parentheses before the command
  next = getStringBetween(str, line.c_str() + offset, '(', ')');
  if (next > 0) {
    // there are some options
    string option;
    int pos = str.find('=');
    bool error = false;
    if (pos > 0) {
      option = str.substr(0, pos);
      if (strcasecmp(option.c_str(), "screen") == 0)
        error = !extractNumber(str.c_str() + pos + 1, screen);
      else
        error = true;
    } else
      error = true;
    if (error) {
      cerr<<"Error parsing startup options.\n";
      return false;
    }
  } else
      next = 0;

  next = getStringBetween(str, line.c_str() + offset + next, '{', '}');

  if (next <= 0) {
    cerr<<"Error parsing [startup] at column "<<offset<<" - expecting {command}.\n";
    return false;
  }

  // don't run command if shynebox is restarting
  if (sb->findScreen(screen)->isRestart() )
    return true;
    // the line was successfully read; we just didn't use it

  SbCommands::ExecuteCmd *tmp_exec_cmd = new SbCommands::ExecuteCmd(str, screen);

  sbdbg<<"Executing startup Command<void> '"<<str<<"' on screen "<<screen<<"\n";

  tmp_exec_cmd->execute();
  delete tmp_exec_cmd;
  return true;
} // handleStartupItem


// returns number of lines read
// optionally can give a line to read before the first (lookahead line)
int parseApp(ifstream &file, Application &app, string *first_line = 0) {
  string line;
  _SB_USES_NLS;
  Focus::Protection protect = Focus::NoProtection;
  bool remember_protect = false;
  int row = 0;
  while (! file.eof() ) {
    if (!(first_line || getline(file, line) ) )
      continue;

    if (first_line) {
      line = *first_line;
      first_line = 0;
    }

    row++;
    if (isComment(line) )
      continue;

    string str_key, str_option, str_label;
    int parse_pos = 0;
    int err = getStringBetween(str_key, line.c_str(), '[', ']');
    if (err > 0) {
      int tmp = getStringBetween(str_option, line.c_str() + err, '(', ')');
      if (tmp > 0)
        err += tmp;
    }
    if (err > 0 ) {
      parse_pos += err;
      getStringBetween(str_label, line.c_str() + parse_pos, '{', '}');
    } else
        continue; //read next line

    bool had_error = false;

    if (str_key.empty() )
      continue; //read next line

    str_key = toLower(str_key);
    str_label = toLower(str_label);

    if (str_key == "workspace") {
      unsigned int w;
      if (extractNumber(str_label, w) )
        app.rememberWorkspace(w);
      else
        had_error = true;
    } else if (str_key == "head") {
      unsigned int h;
      if (extractNumber(str_label, h) )
        app.rememberHead(h);
      else
        had_error = true;
    } else if (str_key == "layer") {
        std::string v = tk::StringUtil::toUpper(str_label);
        int l = tk::ResLayers_strnum.sz - 1;
        for (; l > 0 ; l--)
          if (tk::ResLayers_strnum.estr[l] == v)
            break;

        app.rememberLayer(l);
    } else if (str_key == "dimensions") {
        std::vector<string> tokens;
        tk::StringUtil::stringtok<std::vector<string> >(tokens, str_label);
        if (tokens.size() == 2) {
          unsigned int h, w;
          bool h_relative, w_relative, ignore;
          w = tk::StringUtil::parseSizeToken(tokens[0], w_relative, ignore);
          h = tk::StringUtil::parseSizeToken(tokens[1], h_relative, ignore);
          app.rememberDimensions(w, h, w_relative, h_relative);
        } else
          had_error = true;
    } else if (str_key == "ignoresizehints") {
        app.ignoreSizeHints_remember = str_label == "yes";
    } else if (str_key == "position") {
        ShyneboxWindow::ReferenceCorner r = ShyneboxWindow::LEFTTOP;
        // more info about the parameter
        // in ::rememberPosition

        if (str_option.length() ) {
          r = ShyneboxWindow::getCorner(str_option);
          std::vector<string> tokens;
          tk::StringUtil::stringtok<std::vector<string> >(tokens, str_label);
          if (tokens.size() == 2) {
            int x, y;
            bool x_relative, y_relative, ignore;
            x = tk::StringUtil::parseSizeToken(tokens[0], x_relative, ignore);
            y = tk::StringUtil::parseSizeToken(tokens[1], y_relative, ignore);
            app.rememberPosition(x, y, x_relative, y_relative, r);
          }
        }
    } else if (str_key == "shaded") {
        app.rememberShadedstate(str_label == "yes");
    } else if (str_key == "focushidden") {
        app.rememberFocusHiddenstate(str_label == "yes");
    } else if (str_key == "iconhidden") {
        app.rememberIconHiddenstate(str_label == "yes");
    } else if (str_key == "hidden") {
        app.rememberIconHiddenstate(str_label == "yes");
        app.rememberFocusHiddenstate(str_label == "yes");
    } else if (str_key == "deco") {
        int deco = WindowState::getDecoMaskFromString(str_label);
        if (deco == -1)
            had_error = 1;
        else
            app.rememberDecostate((unsigned int)deco);
    } else if (str_key == "sticky") {
        app.rememberStuckstate(str_label == "yes");
    } else if (str_key == "focusnewwindow") {
        remember_protect = true;
        if (!(protect & (Focus::Gain|Focus::Refuse) ) ) { // cut back on contradiction
            if (str_label == "yes")
                protect |= Focus::Gain;
            else
                protect |= Focus::Refuse;
        }
    } else if (str_key == "focusprotection") {
        remember_protect = true;
        std::list<std::string> labels;
        tk::StringUtil::stringtok(labels, str_label, ", ");
        for (auto it : labels) {
            if (it == "lock")
                protect = (protect & ~Focus::Deny) | Focus::Lock;
            else if (it == "deny")
                protect = (protect & ~Focus::Lock) | Focus::Deny;
            else if (it == "gain")
                protect = (protect & ~Focus::Refuse) | Focus::Gain;
            else if (it == "refuse")
                protect = (protect & ~Focus::Gain) | Focus::Refuse;
            else if (it == "none")
                protect = Focus::NoProtection;
            else
                had_error = 1;
        }
    } else if (str_key == "minimized") {
        app.rememberMinimizedstate(str_label == "yes");
    } else if (str_key == "maximized") {
        WindowState::MaximizeMode m = WindowState::MAX_NONE;
        if (str_label == "yes")
            m = WindowState::MAX_FULL;
        else if (str_label == "horz")
            m = WindowState::MAX_HORZ;
        else if (str_label == "vert")
            m = WindowState::MAX_VERT;
        app.rememberMaximizedstate(m);
    } else if (str_key == "fullscreen") {
        app.rememberFullscreenstate(str_label == "yes");
    } else if (str_key == "jump") {
        app.rememberJumpworkspace(str_label == "yes");
    } else if (str_key == "close") {
        app.rememberSaveOnClose(str_label == "yes");
    } else if (str_key == "end") {
        break;
    } else {
        cerr << _SB_CONSOLETEXT(Remember, Unknown, "Unknown apps key",
               "apps entry type not known")<<" = " << str_key << "\n";
    } // if-chain line parse
    if (had_error)
        cerr<<"Error parsing apps entry: ("<<line<<")\n";
  } // while ! eof
  if (remember_protect)
      app.rememberFocusProtection(protect);
  return row;
} // parseApp


/*
  This function is used to search for old instances of the same pattern
  (when reloading apps file). More than one pattern might match, but only
  if the application is the same (also note that they'll be adjacent).
  We REMOVE and delete any matching patterns from the old list, as they're
  effectively moved into the new
*/

Application* findMatchingPatterns(ClientPattern *pat, Remember::Patterns *patlist,
                    bool transient, bool is_group, ClientPattern *match_pat = 0) {
  Remember::Patterns::iterator it = patlist->begin();
  Remember::Patterns::iterator it_end = patlist->end();

  for (; it != it_end; ++it) {
    if (*it->first == *pat && is_group == it->second->is_grouped
        && transient == it->second->is_transient
        && ((match_pat == 0 && it->second->group_pattern == 0)
          || (match_pat && *match_pat == *it->second->group_pattern) ) ) {
      Application *ret = it->second;

      if (!is_group)
        return ret;
      // find the rest of the group and remove it from the list

      // rewind
      Remember::Patterns::iterator tmpit = it;
      while (tmpit != patlist->begin() ) {
        --tmpit;
        if (tmpit->second == ret)
          it = tmpit;
        else
          break;
      }

      // forward
      for (; it != it_end && it->second == ret; ++it)
        delete it->first;
      patlist->erase(patlist->begin(), it);

      return ret;
    } // if pat match
  } // for patlist
  return 0;
} // Application findMatchinPatterns

} // end anonymous namespace

Remember *Remember::s_instance = 0;

Remember::Remember():
    m_pats(new Patterns() ),
    m_reloader(new tk::AutoReloadHelper() ) {
  if (s_instance != 0)
    throw string("Can not create more than one instance of Remember");

  s_instance = this;

  tk::Command<void> *reload_cmd = new tk::SimpleCommand<Remember>(*this, &Remember::reload);
  m_reloader->setReloadCmd(*reload_cmd);
  reconfigure();
} // Remember class init

Remember::~Remember() {
  // the patterns free the "Application"s
  // the client mapping shouldn't need cleaning
  Patterns::iterator it;
  set<Application *> all_apps; // no duplicates
  while (!m_pats->empty() ) {
    it = m_pats->begin();
    delete it->first; // ClientPattern
    all_apps.insert(it->second); // Application, not necessarily unique
    m_pats->erase(it);
  }
  delete m_pats;

  for (auto ait : all_apps)
    delete ait;

  delete(m_reloader);

  s_instance = 0;
} // Remember class destroy

Application* Remember::find(WinClient &winclient) {
  // if it is already associated with a application, return that one
  // otherwise, check it against every pattern that we've got
  Clients::iterator wc_it = m_clients.find(&winclient);
  if (wc_it != m_clients.end() )
    return wc_it->second;
  else {
    for (auto it : *m_pats) {
      //if (it.first->match(winclient)
      //    && it.second->is_transient == winclient.isTransient() ) {
      if (it.first->match(winclient) ) {
        it.first->addMatch();
        m_clients[&winclient] = it.second;
        return it.second;
      }
    } // for m_pats
  } // if client exist
  // oh well, no matches
  return 0;
}

Application * Remember::add(WinClient &winclient) {
  ClientPattern *p = new ClientPattern();
  Application *app = new Application(winclient.isTransient(), false);

  // by default, we match against the WMClass of a window (instance and class strings)
  string win_name  = ::escapeRememberChars(p->getProperty(WP::NAME,  winclient) );
  string win_class = ::escapeRememberChars(p->getProperty(WP::CLASS, winclient) );
  string win_role  = ::escapeRememberChars(p->getProperty(WP::ROLE,  winclient) );

  p->addTerm(win_name, WP::NAME);
  p->addTerm(win_class, WP::CLASS);
  if (!win_role.empty() )
    p->addTerm(win_role, WP::ROLE);
  m_clients[&winclient] = app;
  p->addMatch();
  m_pats->push_back(make_pair(p, app) );
  return app;
}

void Remember::reconfigure() {
  m_reloader->setMainFile(Shynebox::instance()->getAppsFilename() );
}

void Remember::checkReload() {
  m_reloader->checkReload();
}

void Remember::reload() {
  Shynebox& sb = *Shynebox::instance();
  string apps_string = expandFilename(sb.getAppsFilename() );
  bool ok = true;

  sbdbg<<"("<<__FUNCTION__<<"): Loading apps file ["<<apps_string<<"]\n";

  ifstream apps_file(apps_string.c_str() );

  // we merge the old patterns with new ones
  Patterns *old_pats = m_pats; // delete ahead
  set<Application *> reused_apps;
  m_pats = new Patterns();
  m_startups.clear();

  if (apps_file.fail() ) {
    ok = false;
    cerr << "failed to open apps file " << apps_string << "\n";
  }

  if (ok && apps_file.eof() ) {
    ok = false;
    sbdbg<<"("<<__FUNCTION__<< ") Empty apps file\n";
  }

  if (ok) {
    string line;
    int row = 0;
    bool in_group = false;
    ClientPattern *pat = 0;
    list<ClientPattern *> grouped_pats;
    while (getline(apps_file, line) && ! apps_file.eof() ) {
      row++;

      if (isComment(line) )
        continue;

      string key;
      int err=0;
      int pos = getStringBetween(key, line.c_str(), '[', ']');
      string lc_key = toLower(key);

    if (pos > 0 && (lc_key == "app" || lc_key == "transient") ) {
      ClientPattern *pat = new ClientPattern(line.c_str() + pos);
      if (!in_group) {
          if ((err = pat->error() ) == 0) {
            bool transient = (lc_key == "transient");
            Application *app = findMatchingPatterns(pat,
                                   old_pats, transient, false);
            if (app) {
              app->reset();
              reused_apps.insert(app);
            } else
              app = new Application(transient, false);

            m_pats->push_back(make_pair(pat, app) );
            row += parseApp(apps_file, *app);
          } else {
            cerr<<"Error reading apps file at line "<<row<<", column "<<(err+pos)<<".\n";
            delete pat; // since it didn't work
          }
      } else
        grouped_pats.push_back(pat);
    } else if (pos > 0 && (lc_key == "startup" && sb.isStartup() ) ) {
      if (!handleStartupItem(line, pos) )
        cerr<<"Error reading apps file at line "<<row<<".\n";
      // save the item even if it was bad (aren't we nice)
      m_startups.push_back(line.substr(pos) );
    } else if (pos > 0 && (lc_key == "group") ) {
      in_group = true;
      if (line.find('(') != string::npos)
        pat = new ClientPattern(line.c_str() + pos);
    } else if (in_group) {
      // otherwise assume that it is the start of the attributes
      Application *app = 0;
      // search for a matching app
      for (auto it : grouped_pats) {
        app = findMatchingPatterns(it, old_pats, false, in_group, pat);
        if (app)
          break;
      }

      if (!app)
        app = new Application(false, in_group, pat);
      else
        reused_apps.insert(app);

      while (!grouped_pats.empty() ) {
        // associate all the patterns with this app
        m_pats->push_back(make_pair(grouped_pats.front(), app) );
        grouped_pats.pop_front();
      }

      // we hit end... probably don't have attribs for the group
      // so finish it off with an empty application
      // otherwise parse the app
      if (!(pos > 0 && lc_key == "end") )
        row += parseApp(apps_file, *app, &line);
      in_group = false;
    } else
      cerr<<"Error in apps file on line "<<row<<".\n";
    // if pos / else if in_group
    }
  }

  // Clean up old state
  // can't just delete old patterns list. Need to delete the
  // patterns themselves, plus the applications!

  Patterns::iterator it;
  set<Application *> old_apps; // no duplicates
  while (!old_pats->empty() ) {
    it = old_pats->begin();
    delete it->first; // ClientPattern
    if (reused_apps.find(it->second) == reused_apps.end() )
      old_apps.insert(it->second); // Application, not necessarily unique
    old_pats->erase(it);
  }

  // now remove any client entries for the old apps
  for (auto cit : m_clients) {
    if (old_apps.find(cit.second) != old_apps.end() ) {
      WinClient *tmpit = cit.first;
      m_clients.erase(tmpit);
    }
  } // while/for clients

  for (auto ait : old_apps)
    delete ait;

  delete old_pats;
} // reload

void Remember::save() {
  string apps_string = tk::StringUtil::expandFilename(Shynebox::instance()->getAppsFilename() );

  sbdbg<<"("<<__FUNCTION__<<"): Saving apps file ["<<apps_string<<"]\n";

  ofstream apps_file(apps_string.c_str() );

  // first of all we output all the startup commands
  for (auto sit : m_startups)
    apps_file<<"[startup] "<<sit<<"\n";

  set<Application *> grouped_apps; // no duplicates

  for (auto it : *m_pats) {
    Application &a = *it.second;
    if (a.is_grouped) {
      // if already processed
      if (grouped_apps.find(&a) != grouped_apps.end() )
        continue;
      grouped_apps.insert(&a);
      // otherwise output this whole group
      apps_file << "[group]";
      if (a.group_pattern)
        apps_file << " " << a.group_pattern->toString();
      apps_file << "\n";

      for (auto git : *m_pats)
        if (git.second == &a)
          apps_file << (a.is_transient ? " [transient]" : " [app]") <<
                                         git.first->toString()<<"\n";
    } else
      apps_file << (a.is_transient ? "[transient]" : "[app]") <<
                   it.first->toString()<<"\n";
    // ^ if grouped
    if (a.workspace_remember)
      apps_file << "  [Workspace]\t{" << a.workspace << "}\n";
    if (a.head_remember)
      apps_file << "  [Head]\t{" << a.head << "}\n";
    if (a.dimensions_remember)
      apps_file << "  [Dimensions]\t{" <<
          a.w << (a.dimension_is_w_relative ? "% " : " ") <<
          a.h << (a.dimension_is_h_relative ? "%}" : "}") << "\n";
    if (a.position_remember) {
        apps_file << "  [Position]\t(";
        switch(a.refc) {
        case ShyneboxWindow::CENTER:
          apps_file << "CENTER";
          break;
        case ShyneboxWindow::LEFTBOTTOM:
          apps_file << "LOWERLEFT";
          break;
        case ShyneboxWindow::RIGHTBOTTOM:
          apps_file << "LOWERRIGHT";
          break;
        case ShyneboxWindow::RIGHTTOP:
          apps_file << "UPPERRIGHT";
          break;
        case ShyneboxWindow::LEFT:
          apps_file << "LEFT";
          break;
        case ShyneboxWindow::RIGHT:
          apps_file << "RIGHT";
          break;
        case ShyneboxWindow::TOP:
          apps_file << "TOP";
          break;
        case ShyneboxWindow::BOTTOM:
          apps_file << "BOTTOM";
          break;
        default:
          apps_file << "UPPERLEFT";
        }
        apps_file << ")\t{" <<
            a.x << (a.position_is_x_relative ? "% " : " ") <<
            a.y << (a.position_is_y_relative ? "%}" : "}") << "\n";
    } // if position remember
    if (a.shadedstate_remember)
      apps_file << "  [Shaded]\t{" << ((a.shadedstate)?"yes":"no") << "}\n";
    if (a.decostate_remember) {
      switch (a.decostate) {
      case (0) :
        apps_file << "  [Deco]\t{NONE}\n";
        break;
      case (0xffffffff):
      case (WindowState::DECOR_NORMAL):
        apps_file << "  [Deco]\t{NORMAL}\n";
        break;
      case (WindowState::DECOR_TOOL):
        apps_file << "  [Deco]\t{TOOL}\n";
        break;
      case (WindowState::DECOR_TINY):
        apps_file << "  [Deco]\t{TINY}\n";
        break;
      case (WindowState::DECOR_BORDER):
        apps_file << "  [Deco]\t{BORDER}\n";
        break;
      case (WindowState::DECOR_TAB):
        apps_file << "  [Deco]\t{TAB}\n";
        break;
      default:
        apps_file << "  [Deco]\t{0x"<<hex<<a.decostate<<dec<<"}\n";
        break;
      }
    } // if decostate remember
    if (a.focushiddenstate_remember || a.iconhiddenstate_remember) {
      if (a.focushiddenstate_remember && a.iconhiddenstate_remember
          && a.focushiddenstate == a.iconhiddenstate)
        apps_file << "  [Hidden]\t{" << ((a.focushiddenstate)?"yes":"no") << "}\n";
      else if (a.focushiddenstate_remember)
        apps_file << "  [FocusHidden]\t{" << ((a.focushiddenstate)?"yes":"no") << "}\n";
      else if (a.iconhiddenstate_remember)
        apps_file << "  [IconHidden]\t{" << ((a.iconhiddenstate)?"yes":"no") << "}\n";
    }
    if (a.stuckstate_remember)
      apps_file << "  [Sticky]\t{" << ((a.stuckstate)?"yes":"no") << "}\n";
    if (a.focusprotection_remember) {
      apps_file << "  [FocusProtection]\t{";
      if (a.focusprotection == Focus::NoProtection) {
        apps_file << "none";
      } else {
        bool b = false;
        if (a.focusprotection & Focus::Gain) {
          apps_file << (b?",":"") << "gain";
          b = true;
        }
        if (a.focusprotection & Focus::Refuse) {
          apps_file << (b?",":"") << "refuse";
          b = true;
        }
        if (a.focusprotection & Focus::Lock) {
          apps_file << (b?",":"") << "lock";
          b = true;
        }
        if (a.focusprotection & Focus::Deny) {
          apps_file << (b?",":"") << "deny";
          b = true;
        }
      }
      apps_file << "}\n";
    } // if focusprotect remember
    if (a.minimizedstate_remember)
      apps_file << "  [Minimized]\t{" << ((a.minimizedstate)?"yes":"no") << "}\n";
    if (a.maximizedstate_remember) {
      apps_file << "  [Maximized]\t{";
      switch (a.maximizedstate) {
      case WindowState::MAX_FULL:
        apps_file << "yes" << "}\n";
        break;
      case WindowState::MAX_HORZ:
        apps_file << "horz" << "}\n";
        break;
      case WindowState::MAX_VERT:
        apps_file << "vert" << "}\n";
        break;
      case WindowState::MAX_NONE:
      default:
        apps_file << "no" << "}\n";
        break;
      }
    } // if max remember
    if (a.fullscreenstate_remember)
      apps_file << "  [Fullscreen]\t{" << ((a.fullscreenstate)?"yes":"no") << "}\n";
    if (a.jumpworkspace_remember)
      apps_file << "  [Jump]\t{" << ((a.jumpworkspace)?"yes":"no") << "}\n";
    if (a.layer_remember)
      apps_file << "  [Layer]\t{" << a.layer << "}\n";
    if (a.save_on_close_remember)
      apps_file << "  [Close]\t{" << ((a.save_on_close)?"yes":"no") << "}\n";
    apps_file << "[end]\n";
  } // for m_pats
  apps_file.close();
  // update timestamp to avoid unnecessary reload
  m_reloader->addFile(Shynebox::instance()->getAppsFilename() );
} // save

bool Remember::isRemembered(WinClient &winclient, Attribute attrib) {
  Application *app = find(winclient);
  if (!app) return false;
  switch (attrib) {
  case REM_WORKSPACE:
      return app->workspace_remember;
      break;
  case REM_HEAD:
      return app->head_remember;
      break;
  case REM_DIMENSIONS:
      return app->dimensions_remember;
      break;
  case REM_IGNORE_SIZEHINTS:
      return app->ignoreSizeHints_remember;
      break;
  case REM_POSITION:
      return app->position_remember;
      break;
  case REM_FOCUSHIDDENSTATE:
      return app->focushiddenstate_remember;
      break;
  case REM_ICONHIDDENSTATE:
      return app->iconhiddenstate_remember;
      break;
  case REM_STUCKSTATE:
      return app->stuckstate_remember;
      break;
  case REM_FOCUSPROTECTION:
      return app->focusprotection_remember;
      break;
  case REM_MINIMIZEDSTATE:
      return app->minimizedstate_remember;
      break;
  case REM_MAXIMIZEDSTATE:
      return app->maximizedstate_remember;
      break;
  case REM_FULLSCREENSTATE:
      return app->fullscreenstate_remember;
      break;
  case REM_DECOSTATE:
      return app->decostate_remember;
      break;
  case REM_SHADEDSTATE:
      return app->shadedstate_remember;
      break;
  case REM_JUMPWORKSPACE:
      return app->jumpworkspace_remember;
      break;
  case REM_LAYER:
      return app->layer_remember;
      break;
  case REM_SAVEONCLOSE:
      return app->save_on_close_remember;
      break;
  case REM_LASTATTRIB:
  default:
      return false; // should never get here
  }
} // isRemembered

void Remember::rememberAttrib(WinClient &winclient, Attribute attrib) {
  ShyneboxWindow *win = winclient.sbwindow();
  if (!win)
    return;
  Application *app = find(winclient);
  if (!app) {
    app = add(winclient);
    if (!app)
      return;
  }
  int head, percx, percy;
  switch (attrib) {
  case REM_WORKSPACE:
      app->rememberWorkspace(win->workspaceNumber() );
      break;
  case REM_HEAD:
      app->rememberHead(win->screen().getHead(win->sbWindow() ) );
      break;
  case REM_DIMENSIONS: {
      head = win->screen().getHead(win->sbWindow() );
      percx = win->screen().calRelativeDimensionWidth(head, win->normalWidth() );
      percy = win->screen().calRelativeDimensionHeight(head, win->normalHeight() );
      app->rememberDimensions(percx, percy, true, true);
      break;
  }
  case REM_POSITION: {
      head = win->screen().getHead(win->sbWindow() );
      percx = win->screen().calRelativePositionWidth(head, win->normalX() );
      percy = win->screen().calRelativePositionHeight(head, win->normalY() );
      app->rememberPosition(percx, percy, true, true);
      break;
  }
  case REM_FOCUSHIDDENSTATE:
      app->rememberFocusHiddenstate(win->isFocusHidden() );
      break;
  case REM_ICONHIDDENSTATE:
      app->rememberIconHiddenstate(win->isIconHidden() );
      break;
  case REM_SHADEDSTATE:
      app->rememberShadedstate(win->isShaded() );
      break;
  case REM_DECOSTATE:
      app->rememberDecostate(win->decorationMask() );
      break;
  case REM_STUCKSTATE:
      app->rememberStuckstate(win->isStuck() );
      break;
  case REM_FOCUSPROTECTION:
      app->rememberFocusProtection(win->focusProtection() );
      break;
  case REM_MINIMIZEDSTATE:
      app->rememberMinimizedstate(win->isIconic() );
      break;
  case REM_MAXIMIZEDSTATE:
      app->rememberMaximizedstate(win->maximizedState() );
      break;
  case REM_FULLSCREENSTATE:
      app->rememberFullscreenstate(win->isFullscreen() );
      break;
  case REM_JUMPWORKSPACE:
      app->rememberJumpworkspace(true);
      break;
  case REM_LAYER:
      app->rememberLayer(win->layerNum() );
      break;
  case REM_SAVEONCLOSE:
      app->rememberSaveOnClose(true);
      break;
  case REM_LASTATTRIB:
  default:
      // nothing
      break;
  }
} // rememberAttrib

void Remember::forgetAttrib(WinClient &winclient, Attribute attrib) {
  ShyneboxWindow *win = winclient.sbwindow();
  if (!win)
    return;
  Application *app = find(winclient);
  if (!app) {
    app = add(winclient);
    if (!app)
      return;
  }
  switch (attrib) {
  case REM_WORKSPACE:
      app->forgetWorkspace();
      break;
  case REM_HEAD:
      app->forgetHead();
      break;
  case REM_DIMENSIONS:
      app->forgetDimensions();
      break;
  case REM_IGNORE_SIZEHINTS:
      app->ignoreSizeHints_remember = false;
      break;
  case REM_POSITION:
      app->forgetPosition();
      break;
  case REM_FOCUSHIDDENSTATE:
      app->forgetFocusHiddenstate();
      break;
  case REM_ICONHIDDENSTATE:
      app->forgetIconHiddenstate();
      break;
  case REM_STUCKSTATE:
      app->forgetStuckstate();
      break;
  case REM_FOCUSPROTECTION:
      app->forgetFocusProtection();
      break;
  case REM_MINIMIZEDSTATE:
      app->forgetMinimizedstate();
      break;
  case REM_MAXIMIZEDSTATE:
      app->forgetMaximizedstate();
      break;
  case REM_FULLSCREENSTATE:
      app->forgetFullscreenstate();
      break;
  case REM_DECOSTATE:
      app->forgetDecostate();
      break;
  case REM_SHADEDSTATE:
      app->forgetShadedstate();
      break;
  case REM_JUMPWORKSPACE:
      app->forgetJumpworkspace();
      break;
  case REM_LAYER:
      app->forgetLayer();
      break;
  case REM_SAVEONCLOSE:
      app->forgetSaveOnClose();
      break;
  case REM_LASTATTRIB:
  default:
      break; // nothing
  }
} // forgetAttrib

void Remember::setupFrame(ShyneboxWindow &win) {
  WinClient &winclient = win.winClient();
  Application *app = find(winclient);
  if (app == 0)
    return; // nothing to do

  // first, set the options that aren't preserved as window properties on
  // restart, then return if shynebox is restarting -- we want restart to
  // disturb the current window state as little as possible

  if (app->focushiddenstate_remember)
    win.setFocusHidden(app->focushiddenstate);
  if (app->iconhiddenstate_remember)
    win.setIconHidden(app->iconhiddenstate);
  if (app->layer_remember)
    win.moveToLayer(app->layer);
  if (app->decostate_remember)
    win.setDecorationMask(app->decostate);

  BScreen &screen = winclient.screen();

  // now check if shynebox is restarting
  if (screen.isRestart() )
    return;

  if (app->workspace_remember) {
    // we use setWorkspace and not reassoc because we're still initialising
    win.setWorkspace(app->workspace);
    if (app->jumpworkspace_remember && app->jumpworkspace)
      screen.changeWorkspaceID(app->workspace);
  }

  if (app->head_remember)
    win.setOnHead(app->head);

  if (app->dimensions_remember) {
    int win_w = app->w;
    int win_h = app->h;
    int head = screen.getHead(win.sbWindow() );
    int border_w = win.frame().window().borderWidth();

    if (app->dimension_is_w_relative)
      win_w = screen.calRelativeWidth(head, win_w);
    if (app->dimension_is_h_relative)
      win_h = screen.calRelativeHeight(head, win_h);

    win.resize(win_w - 2 * border_w, win_h - 2 * border_w);
  }

  if (app->position_remember) {
    int newx = app->x;
    int newy = app->y;
    int head = screen.getHead(win.sbWindow() );

    if (app->position_is_x_relative)
      newx = screen.calRelativeWidth(head, newx);
    if (app->position_is_y_relative)
      newy = screen.calRelativeHeight(head, newy);

    win.translateCoords(newx, newy, app->refc);
    win.move(newx, newy);
  }

  if (app->shadedstate_remember)
    if ((win.isShaded() && !app->shadedstate) ||
           (!win.isShaded() && app->shadedstate) )
      win.shade(); // toggles

  if (app->stuckstate_remember)
    if ((win.isStuck() && !app->stuckstate) ||
           (!win.isStuck() && app->stuckstate) )
      win.stick(); // toggles

  if (app->focusprotection_remember)
    win.setFocusProtection(app->focusprotection);

  if (app->minimizedstate_remember) {
    // if inconsistent...
    // this one doesn't actually work, but I can't imagine needing it
    if (win.isIconic() && !app->minimizedstate)
      win.deiconify();
    else if (!win.isIconic() && app->minimizedstate)
      win.iconify();
  }

  // I can't really test the "no" case of this
  if (app->maximizedstate_remember)
    win.setMaximizedState(app->maximizedstate);

  // I can't really test the "no" case of this
  if (app->fullscreenstate_remember)
    win.setFullscreen(app->fullscreenstate);
} // setupFrame

void Remember::setupClient(WinClient &winclient) {
  // leave windows alone on restart
  if (winclient.screen().isRestart() )
    return;

  // check if apps file has changed
  checkReload();

  Application *app = find(winclient);
  if (app == 0)
    return; // nothing to do

  ShyneboxWindow *group;
  if (winclient.sbwindow() == 0 && app->is_grouped
      && (group = findGroup(app, winclient.screen() ) ) ) {

    group->attachClient(winclient);
    if (app->jumpworkspace_remember && app->jumpworkspace)
      winclient.screen().changeWorkspaceID(group->workspaceNumber() );
      // jump to window, not saved workspace
  }
} // setupClient

ShyneboxWindow *Remember::findGroup(Application *app, BScreen &screen) {
  if (!app || !app->is_grouped)
    return 0;

  // find the first client associated with the app and return its sbwindow
  for (auto it : m_clients)
    if (it.second == app && it.first->sbwindow()
        && &screen == &it.first->screen()
        && (!app->group_pattern || app->group_pattern->match(*it.first) ) )
      return it.first->sbwindow();

  // there weren't any open, but that's ok
  return 0;
}

void Remember::updateDecoStateFromClient(WinClient& winclient) {
  Application* app= find(winclient);

  if (app && isRemembered(winclient, REM_DECOSTATE) )
    winclient.sbwindow()->setDecorationMask(app->decostate);
}

void Remember::updateClientClose(WinClient &winclient) {
  checkReload(); // reload if it's changed
  Application *app = find(winclient);

  if (app) {
    for (auto it : *m_pats) {
      if (it.second == app) {
        it.first->removeMatch();
        break;
      }
    }
  }

  if (app && (app->save_on_close_remember && app->save_on_close) ) {
    for (int attrib = 0; attrib < REM_LASTATTRIB; attrib++) {
      if (isRemembered(winclient, (Attribute) attrib) )
        rememberAttrib(winclient, (Attribute) attrib);
      save();
    }
  }

  // we need to get rid of references to this client
  Clients::iterator wc_it = m_clients.find(&winclient);
  if (wc_it != m_clients.end() )
    m_clients.erase(wc_it);
}

tk::Menu* Remember::createMenu(BScreen& screen) {
  return createRememberMenu(screen);
}

// Copyright (c) 2023 Shynebox - zlice
//
// Copyright (c) 2003 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
//                     and Simon Bowden    (rathnor at users.sourceforge.net)
// Copyright (c) 2002 Xavier Brouckaert
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
