// Config.cc for Shynebox Window Manager

#include "Config.hh"
#include "I18n.hh"
#include "StringUtil.hh"

//#include "Debug.hh"

#include <iostream>
#include <fstream>
#include <cassert>

using std::ifstream;
using std::ofstream;
using std::string;

namespace tk {

// these are the defaults for the config file 'init'
// NOTE: some of these values are signed instead of unsigned
//       because of misc logic elsewhere
const strstr dflts[] = {
  { 's', "appsFile",                         "" }, // becomes $cfgdir/apps
  { 'u', "autoRaiseDelay",                   "0" },
  { 'b', "autoRaiseWindows",                 "true" },
  { 'u', "cacheLife",                        "50" },
  { 'u', "cacheMax",                         "2000" },
  { 'b', "clickRaisesWindows",               "true" },
  { 'b', "clientMenu.usePixmap",             "true" },
  { T_COLPLACE, "colPlacementDirection",     "TOPBOTTOM" },
  { 'i', "colorsPerChannel",                 "4" },
  { 's', "defaultDeco",                      "NORMAL" },
  { 'i', "doubleClickInterval",              "250" },
  { 'i', "edgeResizeSnapThreshold",          "0" },
  { 'i', "edgeSnapThreshold",                "0" },
  { T_FOCMODEL, "focusModel",                "CLICKFOCUS" },
  { 'b', "focusNewWindows",                  "true" },
  { 'b', "focusSameHead",                    "false" },
  { 'b', "fullMaximization",                 "false" },
  { T_IBARALGN, "iconbar.alignment",         "RELATIVE" },
  { 'i', "iconbar.iconTextPadding",          "0" },
  { 'i', "iconbar.iconWidth",                "128" },
  { 's', "iconbar.iconifiedPattern",         "%t" },
  { 's', "iconbar.mode",                     "{static groups} (workspace)" },
  { 'b', "iconbar.usePixmap",                "true" },
  { 's', "keyFile",                          "" }, // becomes $cfgdir/keys
  { 'b', "maxDisableMove",                   "false" },
  { 'b', "maxDisableResize",                 "false" },
  { 'b', "maxIgnoreIncrement",               "true" },
  { 'i', "menuDelay",                        "200" },
  { 's', "menuFile",                         "" }, // becomes $cfgdir/menu
  { T_MENUSRCH, "menuSearch",                "SOMEWHERE" },
  { 'u', "noFocusWhileTypingDelay",          "0" },
  { 'b', "obeyHeads",                        "false" },
  { 'b', "opaqueMove",                       "true" },
  { 'b', "opaqueResize",                     "false" },
  { 'u', "opaqueResizeDelay",                "0" },
  { T_ROWPLACE, "rowPlacementDirection",     "LEFTRIGHT" },
  { 'b', "showWindowPosition",               "false" },
  { 's', "strftimeFormat",                   "%a %d %b %k:%M" },
  { 's', "styleFile",                        "" }, // becomes bloe theme, set in Shynebox()
  { 's', "styleOverlay",                     "" }, // becomes $cfgdir/overlay
  { 's', "systray.pinLeft",                  "" },
  { 's', "systray.pinRight",                 "" },
  { T_TABFMODL, "tabs.focusModel",           "CLICKTABFOCUS" },
  { 'b', "tabs.inTitlebar",                  "true" },
  { 'b', "tabs.maxOver",                     "false" },
  { 'i', "tabs.textPadding",                 "0" },
  { T_TABPLACE, "tabs.placement",            "BOTTOMCENTER" },
  { 'b', "tabs.usePixmap",                   "true" },
  { 'i', "tabs.width",                       "64" },
  { 's', "titlebar.left",                    "Stick" },
  { 's', "titlebar.right",                   "Minimize Maximize Close" },
  { 'b', "toolbar.autoHide",                 "false" },
  { 'b', "toolbar.autoRaise",                "false" },
  { 'b', "toolbar.claimSpace",               "true" },
  { 'i', "toolbar.height",                   "0" },
  { T_TBRLAYER, "toolbar.layer",             "DOCK" },
  { 'i', "toolbar.onHead",                   "1" },
  { T_TBRPLACE, "toolbar.placement",         "BOTTOMCENTER" },
  { 's', "toolbar.tools",                    "workspacename, iconbar, systemtray, clock" },
  { 'b', "toolbar.visible",                  "true" },
  { 'i', "toolbar.widthPercent",             "100" },
  { 'u', "tooltipDelay",                     "500" },
  { 's', "windowMenuFile",                   "" }, // becomes $cfgdir/overlay
  { T_WINPLACE, "windowPlacement",           "ROWMINOVERLAPPLACEMENT" },
  { 's', "workspaceNames",                   "1,2,3,4" },
  { 'i', "workspaces",                       "4" },
  { 'b', "workspaceWarpingHorizontal",       "true" },
  { 'i', "workspaceWarpingHorizontalOffset", "1" },
  { 'b', "workspaceWarpingVertical",         "true" },
  { 'i', "workspaceWarpingVerticalOffset",   "1" },
};

// defaults list
////////////////////////////////////////////////////
// CfgItm

// for if/when you access cfgmap["notthere"]
CfgItm::CfgItm() {
  type = 's';
  setFromString("");
}

CfgItm::CfgItm(char t) {
  type = t;     // actual value starts UNINIALIZED
  // setFromString MUST be called after class init
} // CfgItm class init

CfgItm::~CfgItm() {
  chk_del_str();
} // CfgItm class destroy

// this is not 100% sufficient / safe
// but should only fail if types change mid run
void CfgItm::chk_del_str() {
  if (type == 's' && m_val.s != 0)
    delete m_val.s; // prevent double delete because of map/class creation
}

// used after init() when reading config
void CfgItm::setFromString(const char *strval) {
  if (type != 's')
    chk_del_str(); // shouldn't be swapping types mid run anyway
  switch (type) {
  case 'b':         m_val.b = StringUtil::strcasestr(strval, "true"); break;
  case 'i':         m_val.i = std::stoi(strval);               break;
  case 'u':         m_val.u = std::stoul(strval);              break;

  case T_COLPLACE:  m_val.e = &ColDirection_strnum;            goto set_strnum;
  case T_FOCMODEL:  m_val.e = &MainFocusModel_strnum;          goto set_strnum;
  case T_IBARALGN:  m_val.e = &ButtonTrainAlignment_strnum;    goto set_strnum;
  case T_MENUSRCH:  m_val.e = &MenuMode_strnum;                goto set_strnum;
  case T_ROWPLACE:  m_val.e = &RowDirection_strnum;            goto set_strnum;
  case T_TABFMODL:  m_val.e = &TabFocusModel_strnum;           goto set_strnum;
  case T_TABPLACE:  m_val.e = &TabPlacement_strnum;            goto set_strnum;
  case T_TBRLAYER:  m_val.e = &ResLayers_strnum;               goto set_strnum;
  case T_TBRPLACE:  m_val.e = &ToolbarPlacement_strnum;        goto set_strnum;
  case T_WINPLACE:  m_val.e = &ScreenPlacementPolicy_strnum;   // goto set_strnum;
set_strnum:
    *m_val.e = strval;                                         break;

  default:
  case 's':           m_val.s = new string(strval);            break;
  }
} // setFromString
void CfgItm::setFromString(string strval) {
  setFromString(strval.c_str() );
} // setFromString

// what ends up in the config file
string CfgItm::getString() const {
  switch (type) {
  case 'b': return string(m_val.b == true ? "true" : "false");
  case 'i': return StringUtil::number2String(m_val.i);
  case 'u': return StringUtil::number2String(m_val.u);
  case T_COLPLACE:
  case T_FOCMODEL:
  case T_IBARALGN:
  case T_MENUSRCH:
  case T_ROWPLACE:
  case T_TABFMODL:
  case T_TABPLACE:
  case T_TBRLAYER:
  case T_TBRPLACE:
  case T_WINPLACE:   // return ResLayers_strnum.estr[*m_val.e]; // direct works for each
    return *m_val.e; // but single operator call returns const string inside estr[]
  default:
  case 's':
    break;
  }
  return *m_val.s;
} // getString


// CfgItm
////////////////////////////////////////////////////
// ConfigManager

ConfigManager::ConfigManager(const char *filename) :
    m_filename(filename ? filename : "") {
  load(filename);
  // if the file doesn't exist, don't write it.
  // first time runs won't have one, but there's
  // no reason to write a file. also worried
  // there could be a weird blip where it isn't
  // seen but then write works and you nuke
  // a valid config
} // ConfigManager class init

ConfigManager::~ConfigManager() {
  for (auto [k,v] : cfgmap)
    delete v;
  cfgmap.clear();
}
// ConfigManager class destroy

bool ConfigManager::load(const char *filename) {
  m_filename = StringUtil::expandFilename(filename).c_str();

  string line;
  ifstream cfg_file(m_filename, ifstream::in);

  if (cfg_file.is_open() ) {
  while (!cfg_file.eof() && getline(cfg_file, line) ) {
    string key;
    string val;
    // key: val (note the leading whitespace)
    size_t colon_pos = string::npos;

    // comment lines
    if (line.c_str()[0] != '!' && line.c_str()[0] != '#')
      colon_pos= line.find(':');
    if (colon_pos != string::npos) {
      key = line.substr(0, colon_pos);
      val = line.substr(colon_pos+1);
      StringUtil::removeFirstWhitespace(val);
      StringUtil::removeTrailingWhitespace(val);
    } else {
      key = line;
      val = "";
    }

    char type = 's'; // default type if not found
    if (val != "") { // if not comment or no colon
      for (const auto [t, k, v] : dflts) {
        if (StringUtil::strcasestr(k, key.c_str() ) ) {
          type = t;
          key = k; // want 'someThing' in config, not 'sOmEtHiNg'
          break;
        }
      } // for
    } // if valid 'key: val'

    if (!cfgmap.count(key) ) // don't create dupes
      cfgmap[key] = new CfgItm(type);
    cfgmap[key]->setFromString(val);
  } // while getline()
  } // if file opened

  // make sure at least defaults are loaded
  for (const auto [t, k, v] : dflts)
    if (cfgmap[k] == 0 // map[] auto-initializer, and a couple defaults are ""
       || (cfgmap[k]->getString() == "" && StringUtil::strcasestr(v, "") ) ) {
      cfgmap[k] = new CfgItm(t);
      cfgmap[k]->setFromString(v);
    }

// DUMP
//  for (const auto [k, v] : cfgmap)
//    sbdbg << "final init value --- " << k << " = " << v->getString() << "\n";

  return !cfg_file.fail(); // if !file error
}

bool ConfigManager::save(const char *filename) {
  assert(filename);
  // these must be local variables; otherwise, the memory gets released by
  // std::string, causing weird issues
  string file_str = StringUtil::expandFilename(filename);
  filename = file_str.c_str();

  string file_buf("");
  const string colon = ": ";
  const string newline = "\n";

  ofstream cfg_file(filename, ofstream::trunc); // overwrite each time

  if (cfg_file) { // opened fine, no fail/bad flag bits
    for (auto [name, val] : cfgmap) {
      if (val->getString() == "")   // commented out lines
        file_buf += name + newline; // will always be at top
      else
        file_buf += name + colon + val->getString() + newline;
    }

    // could check file_buf size
    // buf if there's 0 config items you have bigger problems
    file_buf.pop_back(); // remove last newline

    cfg_file << file_buf;
    } // if file opened

    m_filename = filename;

  return !cfg_file.fail(); // if !file error
}

} // end namespace tk

// Copyright (c) 2023 Shynebox - zlice
//
// Copyright (c) 2023 zlice
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
