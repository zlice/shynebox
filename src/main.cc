// main.cc for Shynebox Window Manager

#include "shynebox.hh"
#include "version.h"
#include "defaults.hh"
#include "cli.hh"

#include "tk/I18n.hh"
#include "tk/StringUtil.hh"

//use GNU extensions
#ifndef	 _GNU_SOURCE
#define	 _GNU_SOURCE
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif

#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif


#include <cstdlib>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <typeinfo>

using std::cout;
using std::cerr;
using std::string;
using std::ostream;
using std::ofstream;
using std::streambuf;
using std::out_of_range;
using std::runtime_error;
using std::bad_cast;
using std::bad_alloc;
using std::exception;

namespace {

Shynebox *shynebox = 0;

void handleSignal(int signum) {
  _SB_USES_NLS;
  static int re_enter = 0;

  switch (signum) {
  case SIGCHLD: // we don't want the child process to kill us
      // more than one process may have terminated
      while (waitpid(-1, 0, WNOHANG | WUNTRACED) > 0);
      break;
  case SIGHUP:
      // xinit sends HUP when it wants to go down. there is no point in
      // restoring anything in the screens / workspaces, the connection
      // to the xserver might drop any moment
      if (shynebox)  shynebox->shutdown(1);
      break;
  case SIGUSR1:
      if (shynebox)  shynebox->restart();
      break;
  case SIGUSR2:
      if (shynebox)  shynebox->reconfigure();
      break;
  case SIGSEGV:
      abort();
      break;
  case SIGALRM:
      // last resort for shutting down shynebox. the alarm() is set in
      // Shynebox::shutdown()
      if (shynebox && shynebox->isShuttingDown() ) {
        cerr << "shynebox took longer than expected to shutdown\n";
        exit(13);
      }
      break;
  case SIGFPE:
  case SIGINT:
  case SIGPIPE:
  case SIGTERM:
      if (shynebox) { shynebox->shutdown(); }
      break;
  default:
      cerr << "ERROR: caught signal - " << signum << "\n";
      // NLS
      //fprintf(stderr,
      //        _SB_CONSOLETEXT(BaseDisplay, SignalCaught,
      //            "%s:      signal %d caught\n",
      //            "signal catch debug message. Include %s for Command<void> and %d for signal number").c_str(),
      //        "TODO: m_arg[0]", signum);

      if (! shynebox->isStartup() && ! re_enter) {
          re_enter = 1;
          cerr<< _SB_CONSOLETEXT(BaseDisplay, ShuttingDown,
                 "Shutting Down\n",
                 "Quitting because of signal, end with newline");
          if (shynebox)
            shynebox->shutdown();
      }

      cerr << _SB_CONSOLETEXT(BaseDisplay, Aborting,
              "Aborting... dumping core\n",
              "Aboring and dumping core, end with newline");
      abort();
      break;
  } // switch(signum)
} // handleSignal

void setupSignalHandling() {
  signal(SIGSEGV, handleSignal);
  signal(SIGSEGV, handleSignal);
  signal(SIGFPE, handleSignal);
  signal(SIGTERM, handleSignal);
  signal(SIGINT, handleSignal);
#ifdef HAVE_ALARM
  signal(SIGALRM, handleSignal);
#endif
  signal(SIGPIPE, handleSignal); // e.g. output sent to grep
  signal(SIGCHLD, handleSignal);
  signal(SIGHUP, handleSignal);
  signal(SIGUSR1, handleSignal);
  signal(SIGUSR2, handleSignal);
}

} // anonymouse namespace

int main(int argc, char **argv) {
  tk::I18n::init(0);

  ShyneboxCli::Options opts;
  int exitcode = opts.parse(argc, argv);

  if (exitcode != -1)
    exit(exitcode);

  exitcode = EXIT_FAILURE;

#ifdef __EMX__
  _chdir2(getenv("X11ROOT") );
#endif // __EMX__

  streambuf *outbuf = 0;
  streambuf *errbuf = 0;

  ofstream log_file(opts.log_filename.c_str() );

  _SB_USES_NLS;

  // setup log file
  if (log_file.is_open() ) {
    cerr << _SB_CONSOLETEXT(main, LoggingTo, "Logging to", "Logging to a file")
         << ": "
         << opts.log_filename << "\n";

    log_file <<"------------------------------------------\n";
    log_file << _SB_CONSOLETEXT(main, LogFile, "Log File", "")
             << ": " << opts.log_filename <<"\n";

    ShyneboxCli::showInfo(log_file);
    log_file << "------------------------------------------\n";
    // setup log to use cout and cerr stream
    outbuf = cout.rdbuf(log_file.rdbuf() );
    errbuf = cerr.rdbuf(log_file.rdbuf() );
  }

  ShyneboxCli::setupConfigFiles(opts.rc_path, opts.rc_file);

  try {
      if (shynebox)
        delete shynebox;
      shynebox = new Shynebox(argc, argv,
                  opts.session_display,
                  opts.rc_path,
                  opts.rc_file,
                  opts.xsync);
      setupSignalHandling();
      shynebox->eventLoop();
      cerr << "Shynebox:  ... exiting event loop\n";
      exitcode = EXIT_SUCCESS;
  } catch (out_of_range &oor) {
      cerr <<"Shynebox: "
          << _SB_CONSOLETEXT(main, ErrorOutOfRange, "Out of range", "Error message")
          << ": " << oor.what() << "\n";
  } catch (runtime_error &re) {
      cerr << "Shynebox: "
          << _SB_CONSOLETEXT(main, ErrorRuntime, "Runtime error", "Error message")
          << ": " << re.what() << "\n";
  } catch (bad_cast &bc) {
      cerr << "Shynebox: "
          << _SB_CONSOLETEXT(main, ErrorBadCast, "Bad cast", "Error message")
          << ": " << bc.what() << "\n";
  } catch (bad_alloc &ba) {
      cerr << "Shynebox: "
          << _SB_CONSOLETEXT(main, ErrorBadAlloc, "Bad Alloc", "Error message")
          << ": " << ba.what() << "\n";
  } catch (exception &e) {
      cerr << "Shynebox: "
          << _SB_CONSOLETEXT(main, ErrorStandardException, "Standard Exception", "Error message")
          << ": " << e.what() << "\n";
  } catch (string & error_str) {
      cerr << _SB_CONSOLETEXT(Common, Error, "Error", "Error message header")
          << ": " << error_str << "\n";
  } catch (...) {
      cerr << "Shynebox: "
          << _SB_CONSOLETEXT(main, ErrorUnknown, "Unknown error", "Error message")
          << "." << "\n";
      abort();
  } // try/catch exit code errors

  bool restarting = false;
  string restart_argument;

  if (shynebox) {
    restarting = shynebox->isRestarting();
    restart_argument = shynebox->getRestartArgument();
  }

  // destroy shynebox
  delete shynebox;
  shynebox = 0;

  // restore cout and cin streams
  if (outbuf != 0)
    cout.rdbuf(outbuf);
  if (errbuf != 0)
    cerr.rdbuf(errbuf);

  tk::SbStringUtil::shutdown();

  if (restarting) {
    if (!restart_argument.empty() ) {
      const char *shell = getenv("SHELL");
      if (!shell)
        shell = "/bin/sh";

      execlp(shell, shell, "-c", restart_argument.c_str(), (const char *) NULL);
      perror(restart_argument.c_str() );
    }

    // fall back in case the above execlp doesn't work
    execvp(argv[0], argv);
    perror(argv[0]);

    const std::string basename = tk::StringUtil::basename(argv[0]);
    execvp(basename.c_str(), argv);
    perror(basename.c_str() );
  }

  return exitcode;
} //main

// Copyright (c) 2023 Shynebox - zlice
//
// Copyright (c) 2023 zlice
//
// Copyright (c) 2001 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
//               and 2003-2005 Simon Bowden (rathnor at users.sourceforge.net)
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
