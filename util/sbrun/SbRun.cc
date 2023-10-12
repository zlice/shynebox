// SbRun.cc
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

#include "SbRun.hh"

#include "tk/App.hh"
#include "tk/EventManager.hh"
#include "tk/Color.hh"
#include "tk/KeyUtil.hh"
#include "tk/FileUtil.hh"

#ifdef HAVE_XPM
#include <X11/xpm.h>
#include "sbrun.xpm"
#endif // HAVE_XPM

#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <unistd.h>

#include <iostream>
#include <iterator>
#include <fstream>
#include <algorithm>

using std::cerr;
using std::endl;
using std::string;
using std::fstream;
using std::ifstream;
using std::ofstream;
using std::ios;

SbRun::SbRun(int x, int y, size_t width):
    tk::TextBox(DefaultScreen(tk::App::instance()->display()),
                  m_font, ""),
    m_print(false),
    m_font("fixed"),
    m_display(tk::App::instance()->display()),
    m_bevel(4),
    m_padding(0),
    m_gc(*this),
    m_end(false),
    m_current_history_item(0),
    m_current_files_item(-1),
    m_last_completion_path(""),
    m_current_apps_item(-1),
    m_completion_pos(std::string::npos),
    m_cursor(XCreateFontCursor(tk::App::instance()->display(), XC_xterm)) {

    setGC(m_gc.gc());
    setCursor(m_cursor);
    // setting nomaximize in local resize
    resize(width, font().height() + m_bevel);

    // setup class name
    XClassHint ch;
    ch.res_name = const_cast<char *>("sbrun");
    ch.res_class = const_cast<char *>("SbRun");
    XSetClassHint(m_display, window(), &ch);

#ifdef HAVE_XPM
    Pixmap mask = 0;
    Pixmap pm;
    XpmCreatePixmapFromData(m_display,
                            window(),
                            const_cast<char **>(sbrun_xpm),
                            &pm,
                            &mask,
                            0); // attribs
    if (mask != 0)
        XFreePixmap(m_display, mask);

    m_pixmap = pm;
#endif // HAVE_XPM

    if (m_pixmap.drawable()) {
        XWMHints wmhints;
        wmhints.flags = IconPixmapHint;
        wmhints.icon_pixmap = m_pixmap.drawable();
        XSetWMHints(m_display, window(), &wmhints);
    }

}


SbRun::~SbRun() {
    hide();
}

void SbRun::run(const std::string &command) {
    tk::App::instance()->end(); // end application
    m_end = true; // mark end of processing

    hide(); // hide gui

    if (m_print) {
        std::cout << command;
        return;
    }

    if (command.empty()) {
        return;
    }

#ifdef HAVE_FORK
    // fork and execute program
    if (!fork()) {

        const char *shell = getenv("SHELL");
        if (!shell)
            shell = "/bin/sh";

        setsid();
        execl(shell, shell, "-c", command.c_str(), static_cast<void*>(NULL));
        exit(0); //exit child
    }
#else
#error "Can't build SbRun - don't know how to launch without fork on your platform"
#endif

    ofstream outfile(m_history_file.c_str());
    if (!outfile) {
        cerr<<"SbRun Warning: Can't write command history to file: "<<m_history_file<<endl;
        return;
    }

    int n = 1024;
    char *a = getenv("SBRUN_HISTORY_SIZE");
    if (a)
        n = atoi(a);
    int j = m_history.size();
    --n; // NOTICE: this should be "-=2", but a duplicate entry in the late
         // (good) section would wait "too" long
         // (we'd wait until 3 items are left and then still skip one for being a dupe)
         // IOW: the limit is either n or n+1, depending in the history structure
    for (unsigned int i = 0; i != m_history.size(); ++i) {
        // don't allow duplicates into the history file
        if (--j > n || m_history[i] == command)
            continue;
        outfile<<m_history[i]<<endl;
    }
    if (++n > 0) // n was decremented for the loop
        outfile << command << endl;
    outfile.close();
}


bool SbRun::loadHistory(const char *filename) {
    if (filename == 0)
        return false;
    ifstream infile(filename);
    if (!infile) {
        //even though we fail to load file, we should try save to it
        ofstream outfile(filename);
        if (outfile) {
            m_history_file = filename;
            return true;
        }
        return false;
    }

    m_history.clear();
    string line;
    while (getline(infile, line)) {
        if (!line.empty()) // don't add empty lines
            m_history.push_back(line);
    }
    // set no current histor to display
    m_current_history_item = m_history.size();
    // set history file
    m_history_file = filename;
    return true;
}

bool SbRun::loadCompletion(const char *filename) {
    if (!filename)
        return false;
    ifstream infile(filename);
    if (!infile)
        return false;

    m_apps.clear();
    string line;
    while (getline(infile, line)) {
        if (!line.empty()) // don't add empty lines
            m_apps.push_back(line);
    }
    return true;
}


bool SbRun::loadFont(const string &fontname) {
    if (!m_font.load(fontname.c_str()))
        return false;

    // resize to fit new font height
    resize(width(), font().height() + m_bevel);
    return true;
}

void SbRun::setForegroundColor(const tk::Color &color) {
    m_gc.setForeground(color);
}

void SbRun::setTitle(const string &title) {
    setName(title.c_str());
}

void SbRun::resize(unsigned int width, unsigned int height) {
    tk::TextBox::resize(width, height);
}

void SbRun::setPadding(int padding) {
    m_padding = padding;
    tk::TextBox::setPadding(padding);
}

void SbRun::redrawLabel() {
    clear();
}

void SbRun::keyPressEvent(XKeyEvent &ke) {
    // reset last completion prefix if we don't do a tab completion thing
    bool did_tab_complete = false;

    ke.state = tk::KeyUtil::instance().cleanMods(ke.state);

    tk::TextBox::keyPressEvent(ke);
    KeySym ks;
    char keychar[1];
    XLookupString(&ke, keychar, 1, &ks, 0);
    // a modifier key by itself doesn't do anything
    if (IsModifierKey(ks))
        return;

    if (m_autocomplete && isprint(keychar[0])) {
        did_tab_complete = true;
        if (m_completion_pos == std::string::npos) {
            m_completion_pos = cursorPosition();
        } else {
            ++m_completion_pos;
        }
        tabCompleteApps();
    } else if (tk::KeyUtil::instance().isolateModifierMask(ke.state)) {
        // a modifier key is down
        if ((ke.state & ControlMask) == ControlMask) {
            switch (ks) {
            case XK_p:
                did_tab_complete = true;
                prevHistoryItem();
                break;
            case XK_n:
                did_tab_complete = true;
                nextHistoryItem();
                break;
            case XK_Tab:
                did_tab_complete = true;
                tabComplete(m_history, m_current_history_item, true); // reverse
                break;
            }
        } else if ((ke.state & (Mod1Mask|ShiftMask)) == (Mod1Mask | ShiftMask)) {
            switch (ks) {
            case XK_less:
                did_tab_complete = true;
                firstHistoryItem();
                break;
            case XK_greater:
                did_tab_complete = true;
                lastHistoryItem();
                break;
            }
        }
    } else { // no modifier key
        switch (ks) {
        case XK_Escape:
            m_end = true;
            hide();
            tk::App::instance()->end(); // end program
            break;
        case XK_KP_Enter:
        case XK_Return:
            run(text());
            break;
        case XK_Up:
            prevHistoryItem();
            break;
        case XK_Down:
            nextHistoryItem();
            break;
        case XK_Tab:
            did_tab_complete = true;
            tabCompleteApps();
            break;
        }
    }
    if (!did_tab_complete)
        m_completion_pos = std::string::npos;
    clear();
}

void SbRun::lockPosition(bool size_too) {
    // we don't need to maximize this window
    XSizeHints sh;
    sh.flags = PMaxSize | PMinSize;
    sh.max_width = width();
    sh.max_height = height();
    sh.min_width = width();
    sh.min_height = height();
    if (size_too) {
        sh.flags |= USPosition;
        sh.x = x();
        sh.y = y();
    }
    XSetWMNormalHints(m_display, window(), &sh);
}

void SbRun::prevHistoryItem() {
    if (!(m_history.empty() || m_current_history_item == 0) ) {
        m_current_history_item--;
        setText(m_history[m_current_history_item]);
    }
}

void SbRun::nextHistoryItem() {
    if (m_current_history_item != m_history.size() ) {
        m_current_history_item++;
        tk::BiDiString text("");
        if (m_current_history_item == m_history.size()) {
            m_current_history_item = m_history.size();
        } else
            text.setLogical((m_history[m_current_history_item]));

        setText(text);
    }
}

void SbRun::firstHistoryItem() {
    if ( !(m_history.empty() || m_current_history_item == 0) ) {
        m_current_history_item = 0;
        setText(tk::BiDiString(m_history[m_current_history_item]));
    }
}

void SbRun::lastHistoryItem() {
    // actually one past the end
    if (!m_history.empty() ) {
        m_current_history_item = m_history.size();
        setText(tk::BiDiString(""));
    }
}

void SbRun::tabComplete(const std::vector<std::string> &list, int &currentItem, bool reverse) {
    if (list.empty())
        return;

    if (m_completion_pos == std::string::npos)
        m_completion_pos = textStartPos() + cursorPosition();
    size_t split = text().find_last_of(' ', m_completion_pos);
    if (split == std::string::npos)
        split = 0;
    else
        ++split; // skip space
    std::string prefix = text().substr(split, m_completion_pos - split);

    if (currentItem < 0)
        currentItem = 0;
    else if (currentItem >= list.size())
        currentItem = list.size() - 1;
    int item = currentItem;

    while (true) {
        if (reverse) {
            if (--item < 0)
                item = list.size() - 1;
        } else {
            if (++item >= list.size())
                item = 0;
        }
        if (list.at(item).find(prefix) == 0) {
            setText(tk::BiDiString(text().substr(0, split) + list.at(item)));
            if (item == currentItem) {
                cursorEnd();
                m_completion_pos = std::string::npos;
            } else {
                select(split + prefix.size(), text().size() - (prefix.size() + split));
            }
            currentItem = item;
            return;
        }
        if (item == currentItem) {
            cursorEnd();
            m_completion_pos = std::string::npos;
            return;
        }
    }
    // found nothing
}


void SbRun::tabCompleteApps() {
    if (m_completion_pos == std::string::npos)
        m_completion_pos = textStartPos() + cursorPosition();
    size_t split = text().find_last_of(' ', m_completion_pos);
    if (split == std::string::npos)
        split = 0;
    else
        ++split; // skip the space
    std::string prefix = text().substr(split, m_completion_pos - split);
    if (prefix.empty())
        return;

    if (prefix.at(0) == '/' || prefix.at(0) == '.' || prefix.at(0) == '~') {
        // we're completing a directory, find subdirs
        split = prefix.find_last_of('/');
        if (split == std::string::npos) {
            split = prefix.size();
            prefix.append("/");
        }
        prefix = prefix.substr(0, split+1);
        if (prefix != m_last_completion_path) {
            m_files.clear();
            m_current_files_item = -1;
            m_last_completion_path = prefix;

            tk::Directory dir;
            std::string path = prefix;
            if (path.at(0) == '~')
                path.replace(0,1,getenv("HOME"));
            dir.open(path.c_str());
            int n = dir.entries();
            while (--n > -1) {
                std::string entry = dir.readFilename();
                if (entry == "." || entry == "..")
                    continue;
                // escape special characters
                std::string needle(" !\"$&'()*,:;<=>?@[\\]^`{|}");
                std::size_t pos = 0;
                while ((pos = entry.find_first_of(needle, pos)) != std::string::npos) {
                    entry.insert(pos, "\\");
                    pos += 2;
                }
                if (tk::FileUtil::isDirectory(std::string(path + entry).c_str()))
                    m_files.push_back(prefix + entry + "/");
                else
                    m_files.push_back(prefix + entry);
            }
            dir.close();
            sort(m_files.begin(), m_files.end());
        }
        tabComplete(m_files, m_current_files_item);
    } else {
        static bool first_run = true;
        if (first_run && m_apps.empty()) {
            first_run = false;
            std::string path = getenv("PATH");
            tk::Directory dir;
            for (unsigned int l = 0, r = 0; r <= path.size(); ++r) {
                if ((r == path.size() || path.at(r) == ':') && r - l > 1) {
                    dir.open(path.substr(l, r - l).c_str());
                    prefix = dir.name() + (*dir.name().rbegin() == '/' ? "" : "/");
                    int n = dir.entries();
                    while (--n > -1) {
                        std::string entry = dir.readFilename();
                        std::string file = prefix + entry;
                        if (tk::FileUtil::isExecutable(file.c_str()) &&
                                     !tk::FileUtil::isDirectory(file.c_str())) {
                            m_apps.push_back(entry);
                        }
                    }
                    dir.close();
                    l = r + 1;
                }
            }
            sort(m_apps.begin(), m_apps.end());
            unique(m_apps.begin(), m_apps.end());
        }
        tabComplete(m_apps, m_current_apps_item);
    }
}

void SbRun::insertCharacter(char keychar) {
    char val[2] = {keychar, 0};
    insertText(val);
}
