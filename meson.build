# -*- mode: meson -*-

# Style objective: be consistent with what mesonbuild.com documents/uses, and/or
# the meson book: https://meson-manual.com/

project(
  'synebox', 'cpp',
  version: '2023-1.0',
  default_options: [
    'cpp_std=c++17',
    'warning_level=0',
    'optimization=s',
    'prefix=/usr',
    'buildtype=custom', # build type overrides the other settings...
  ],
  meson_version: '>=0.47.0',
)

# need version.h.in

# CONFIG.H

cfg_data = configuration_data()
cfg_data.set('__shynebox_version', meson.project_version() )

# COMPILER

cc = meson.get_compiler('cpp')
#add_project_arguments(cc.get_supported_arguments(['-Wunused-value']), language: 'cpp')

compiler_options = [
  '-include',
  'cstdint',
  '-include',
  'config.h',
#  '-Os',
  '-Wno-reorder', # there are some items in Screen and Toolbar that "aren't in order"
#  '-Wno-unused-variable', # see strnum __attribute__((unused))
]

# LIBRARY DEPENDENCIES

## https://mesonbuild.com/howtox.html#add-math-library-lm-portably
#nls_dep = cc.find_library('nls', required: false)
x11_dep = dependency('x11', method: 'pkg-config')
fontconfig_dep = dependency('fontconfig', method: 'pkg-config')
randr_dep = [dependency('xrandr', version: '>=1.2', method: 'pkg-config')]

#  dep_list += [dependency('threads')]

dep_list = [
  x11_dep,
  fontconfig_dep,
  randr_dep,
]

buildtype = get_option('buildtype')
dodebug = get_option('enabledebug')
dofribidi = get_option('fribidi')
dofreetype2 = get_option('freetype2')
doimlib2 = get_option('imlib2')
doregexp = get_option('regexp')
dotoolbar = get_option('toolbar')
doshape = get_option('shape')
doxft = get_option('xft')
doxmb = get_option('xmb')
doxpm = get_option('xpm')

if buildtype == 'debug' or dodebug
  cfg_data.set('DEBUG', 1)
endif

if dofribidi
  cfg_data.set('HAVE_FRIBIDI', 1)
endif

if dofreetype2
  dep_list += [dependency('freetype2', method: 'pkg-config')]
endif

if doimlib2
  cfg_data.set('HAVE_IMLIB2', 1)
  dep_list += [dependency('imlib2', method: 'pkg-config')]
endif

if doregexp
  cfg_data.set('USE_REGEXP', 1)
endif

if dotoolbar
  cfg_data.set('USE_TOOLBAR', 1)
endif

if doshape
  cfg_data.set('SHAPE', 1)
  dep_list += [dependency('xext', method: 'pkg-config')]
endif

if doxft
  cfg_data.set('USE_XFT', 1)
  cfg_data.set('HAVE_XFT_UTF8_STRING', 1)
  # assume this function exist^
  dep_list += [dependency('xft', method: 'pkg-config')]
endif

if doxmb
  cfg_data.set('USE_XMB', 1)
endif

if doxpm
  cfg_data.set('HAVE_XPM', 1)
  dep_list += [dependency('xpm', method: 'pkg-config')]
endif


# FIX - should determine if it's there, but wtf...
#https://github.com/ImageMagick/glib/blob/main/meson.build
  cfg_data.set('HAVE_CASSERT', cc.has_header('cassert') )
  cfg_data.set('HAVE_CATCLOSE', cc.has_function('catclose') )
  cfg_data.set('HAVE_CATGETS', cc.has_function('catgets') )
  cfg_data.set('HAVE_CATOPEN', cc.has_function('catopen') )
  cfg_data.set('HAVE_CERRNO', cc.has_header('cerrno') )
  cfg_data.set('HAVE_CLOCK_GETTIME', cc.has_function('clock_gettime') )
  cfg_data.set('HAVE_CMATH', cc.has_header('cmath') )
  cfg_data.set('HAVE_CSTDIO', cc.has_header('cstdio') )
  cfg_data.set('HAVE_CSTDLIB', cc.has_header('cstdlib') )
  cfg_data.set('HAVE_CSTDINT', cc.has_header('cstdint') )
  cfg_data.set('HAVE_CSTRING', cc.has_header('cstring') )
  cfg_data.set('HAVE_CTIME', cc.has_header('ctime') )
  cfg_data.set('HAVE_CCTYPE', cc.has_header('cctype') ) # ?????
  cfg_data.set('HAVE_CTIME', cc.has_header('ctime') )
  cfg_data.set('HAVE_CTIME', cc.has_header('ctime') )
  cfg_data.set('HAVE_FORK', cc.has_function('fork') )
  cfg_data.set('HAVE_GETPID', cc.has_function('getpid') )
  cfg_data.set('HAVE_ICONV', cc.has_header('iconv.h') )
  cfg_data.set('HAVE_INTTYPES_H', cc.has_header('inttypes.h') )
  cfg_data.set('HAVE_LOCALE_H', cc.has_header('locale.h') )
  cfg_data.set('HAVE_NL_TYPES_H', cc.has_header('nl_types.h') )
  cfg_data.set('HAVE_PROCESS_H', cc.has_header('process.h') )
  cfg_data.set('HAVE_SET_LOCALE', cc.has_function('setlocale') )
  cfg_data.set('HAVE_SIGNAL_H', cc.has_header('signal.h') )
  cfg_data.set('HAVE_STRFTIME', cc.has_function('strftime') )
  cfg_data.set('HAVE_SYNC', cc.has_function('sync') ) # idk wtf this is, only in cli_cfiles ?
  cfg_data.set('HAVE_SYS_PARAM_H', cc.has_header('sys/param.h') )
  cfg_data.set('HAVE_SYS_SELECT_H', cc.has_header('sys/select.h') )
  cfg_data.set('HAVE_SYS_STAT_H', cc.has_header('sys/stat.h') )
  cfg_data.set('HAVE_SYS_TYPES_H', cc.has_header('sys/types.h') )
  cfg_data.set('HAVE_SYS_WAIT_H', cc.has_header('sys/wait.h') )
  cfg_data.set('HAVE_TIME_H', cc.has_header('time.h') )
  cfg_data.set('HAVE_UNISTD_H', cc.has_header('unistd.h') )
  # STDC_HEADERS
  #/* Define to 1 if all of the C90 standard headers exist (not just the ones
  # required in a freestanding environment). This macro is provided for
  # backward compatibility; new code need not use it. */
  # #define STDC_HEADERS 1
  # don't need?
  #
  # a bunch more per aix/interix, macos, solaris, net/open bsd ...



# TODO ???
# NLS
# LOCALEPATH "/usr/share/fluxbox/nls"


config_h = configure_file(
  output: 'config.h',
  configuration: cfg_data,
)

#config_h_in = configure_file(
#  output: 'config.h.in',
#  configuration: cfg_data,
#)

#config_h = declare_dependency(
#  input: cfg_data,
#  output: 'config.h',
#)

#if meson.version().version_compare('>=0.53')
#  summary('build manpages (-Dmans)', get_option('mans'))
#endif

# Required for e.g. struct ucred to be defined as per unix(7).
#add_project_arguments('-D_GNU_SOURCE', language: 'cpp')

libtksrcs = [
  'src/tk/App.cc',
  'src/tk/AutoReloadHelper.cc',
  'src/tk/BorderTheme.cc',
  'src/tk/Button.cc',
  'src/tk/ButtonTrain.cc',
  'src/tk/CachedPixmap.cc',
  'src/tk/Color.cc',
  'src/tk/ColorLUT.cc',
  'src/tk/Config.cc',
  'src/tk/EventManager.cc',
  'src/tk/SbDrawable.cc',
  'src/tk/SbPixmap.cc',
  'src/tk/SbString.cc',
  'src/tk/SbTime.cc',
  'src/tk/SbWindow.cc',
  'src/tk/FileUtil.cc',
  'src/tk/Font.cc',
  'src/tk/GContext.cc',
  'src/tk/I18n.cc',
  'src/tk/Image.cc',
  'src/tk/ImageControl.cc',
  'src/tk/ImageImlib2.cc',
  'src/tk/ImageXPM.cc',
  'src/tk/KeyUtil.cc',
  'src/tk/Layer.cc',
  'src/tk/LayerItem.cc',
  'src/tk/LayerManager.cc',
  'src/tk/LogicCommands.cc',
  'src/tk/MacroCommand.cc',
  'src/tk/Menu.cc',
  'src/tk/MenuItem.cc',
  'src/tk/MenuSearch.cc',
  'src/tk/MenuSeparator.cc',
  'src/tk/MenuTheme.cc',
  'src/tk/RegExp.cc',
  'src/tk/RelCalcHelper.cc',
  'src/tk/Shape.cc',
  'src/tk/StringUtil.cc',
  'src/tk/TextBox.cc',
  'src/tk/TextButton.cc',
  'src/tk/TextTheme.cc',
  'src/tk/TextUtils.cc',
  'src/tk/Texture.cc',
  'src/tk/TextureRender.cc',
  'src/tk/Theme.cc',
  'src/tk/ThemeItems.cc',
  'src/tk/Timer.cc',
  'src/tk/XFontImp.cc',
  'src/tk/XftFontImp.cc',
  'src/tk/XmbFontImp.cc',
]

inc = include_directories('src', 'src/tk')

libtk = static_library(
  'tk',
  libtksrcs,
  include_directories: inc,
  dependencies: [ dep_list, ],
  cpp_args : compiler_options,
)

sbsrcs = [
  'src/ButtonTheme.cc',
  'src/ButtonTool.cc',
  'src/CascadePlacement.cc',
  'src/CenterPlacement.cc',
  'src/ClientMenu.cc',
  'src/ClientPattern.cc',
  'src/ClockTool.cc',
  'src/ConfigMenu.cc',
  'src/CurrentWindowCmd.cc',
  'src/Ewmh.cc',
  'src/SbAtoms.cc',
  'src/SbCommands.cc',
  'src/SbMenu.cc',
  'src/SbMenuParser.cc',
  'src/SbRootWindow.cc',
  'src/SbWinFrame.cc',
  'src/SbWinFrameTheme.cc',
  'src/FocusControl.cc',
  'src/FocusableList.cc',
  'src/HeadArea.cc',
  'src/IconButton.cc',
  'src/IconbarTheme.cc',
  'src/IconbarTool.cc',
  'src/Keys.cc',
  'src/LayerMenu.cc',
  'src/MenuCreator.cc',
  'src/MinOverlapPlacement.cc',
  'src/OSDWindow.cc',
  'src/Remember.cc',
  'src/RootCmdMenuItem.cc',
  'src/RootTheme.cc',
  'src/FocusSmartPlacement.cc',
  'src/Screen.cc',
  'src/ScreenPlacement.cc',
  'src/SendToMenu.cc',
  'src/SpacerTool.cc',
  'src/StyleMenuItem.cc',
  'src/SystemTray.cc',
  'src/TextDialog.cc',
  'src/ToolFactory.cc',
  'src/ToolTheme.cc',
  'src/Toolbar.cc',
  'src/ToolbarItem.cc',
  'src/ToolbarTheme.cc',
  'src/TooltipWindow.cc',
  'src/UnderMousePlacement.cc',
  'src/WinButton.cc',
  'src/WinButtonTheme.cc',
  'src/WinClient.cc',
  'src/Window.cc',
  'src/WindowCmd.cc',
  'src/WindowState.cc',
  'src/Workspace.cc',
  'src/WorkspaceCmd.cc',
  'src/WorkspaceMenu.cc',
  'src/WorkspaceNameTool.cc',
  'src/cli_cfiles.cc',
  'src/cli_info.cc',
  'src/cli_options.cc',
  'src/defaults.cc',
  'src/shynebox.cc',
  'src/main.cc',
]

executable(
  'shynebox',
  sbsrcs,
  install: true,
  include_directories: inc,
  dependencies: dep_list,
  link_with: libtk,
  cpp_args : compiler_options,
)

sbrunsrcs = [
  'util/sbrun/SbRun.cc',
  'util/sbrun/main.cc',
]

executable(
  'sbrun',
  sbrunsrcs,
  install: true,
  include_directories: inc,
  dependencies: dep_list,
  link_with: libtk,
  cpp_args : compiler_options,
)

warning('!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!')
warning('THIS IS A UNFINISHED PROTOTYPE BUILD - IF YOU WOULD LIKE TO FINISH IT, THANKS!')
warning('¯\_(ツ)_/¯ ¯\_(ツ)_/¯ IT IS A BIT SLOWER SO I SAID F IT ¯\_(ツ)_/¯ ¯\_(ツ)_/¯')
warning('!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!')
