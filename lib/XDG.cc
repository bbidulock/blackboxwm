// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// XDG.cc for Blackbox - an X11 Window manager
// Copyright (c) 2001 - 2005 Sean 'Shaleh' Perry <shaleh@debian.org>
// Copyright (c) 1997 - 2000, 2002 - 2005
//         Bradley T Hughes <bhughes at trolltech.com>
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

#include "Util.hh"
#include "XDG.hh"

#include <algorithm>

#include <stdlib.h>
#include <algorithm>

// make sure directory names end with a slash
static std::string terminateDir(const std::string &string)
{
  std::string returnValue = string;
  std::string::const_iterator it = returnValue.end() - 1;
  if (*it != '/')
    returnValue += '/';
  return returnValue;
}

static std::string readEnvDir(const char *name, const char *defaultValue)
{
  const char * const env = getenv(name);
  std::string returnValue = std::string(env ? env : defaultValue);
  returnValue = bt::expandTilde(returnValue);
  return terminateDir(returnValue);
}

static std::list<std::string> readEnvDirList(const char *name,
                                             const char *defaultValue)
{
  const char * const env = getenv(name);

  std::string str = env ? env : defaultValue;
  // if the environment variable ends with a ':', append the
  // defaultValue
  std::string::const_iterator last = str.end() - 1;
  if (*last == ':')
    str += defaultValue;

  std::list<std::string> returnValue;
  const std::string::const_iterator end = str.end();
  std::string::const_iterator begin = str.begin();
  do {
    std::string::const_iterator it = std::find(begin, end, ':');
    std::string dir = std::string(begin, it);
    dir = bt::expandTilde(dir);
    dir = terminateDir(dir);
    returnValue.push_back(dir);
    begin = it;
    if (begin != end)
      ++begin;
  } while (begin != end);

  return returnValue;
}


std::string bt::XDG::BaseDir::dataHome()
{
  static std::string XDG_DATA_HOME =
    readEnvDir("XDG_DATA_HOME", "~/.local/share/");
  return XDG_DATA_HOME;
}

std::string bt::XDG::BaseDir::configHome()
{
  static std::string XDG_CONFIG_HOME =
    readEnvDir("XDG_CONFIG_HOME", "~/.config/");
  return XDG_CONFIG_HOME;
}

std::list<std::string> bt::XDG::BaseDir::dataDirs()
{
  static std::list<std::string> XDG_DATA_DIRS =
    readEnvDirList("XDG_DATA_DIRS", "/usr/local/share/:/usr/share/");
  return XDG_DATA_DIRS;
}

std::list<std::string> bt::XDG::BaseDir::configDirs()
{
  static std::list<std::string> XDG_CONFIG_DIRS =
    readEnvDirList("XDG_CONFIG_DIRS", "/etc/xdg/");
  return XDG_CONFIG_DIRS;
}

std::string bt::XDG::BaseDir::cacheHome()
{
  static std::string XDG_CACHE_HOME =
    readEnvDir("XDG_CACHE_HOME", "~/.cache/");
  return XDG_CACHE_HOME;
}

std::string bt::XDG::BaseDir::writeDataFile(const std::string &filename)
{
  std::string path = dataHome() + filename;
  std::string directoryName = dirname(path);
  if (!mkdirhier(directoryName, 0700))
    return std::string();
  return path;
}

std::string bt::XDG::BaseDir::writeConfigFile(const std::string &filename)
{
  std::string path = configHome() + filename;
  std::string directoryName = dirname(path);
  if (!mkdirhier(directoryName, 0700))
    return std::string();
  return path;
}

std::string bt::XDG::BaseDir::writeCacheFile(const std::string &filename)
{
  std::string path = cacheHome() + filename;
  std::string directoryName = dirname(path);
  if (!mkdirhier(directoryName, 0700))
    return std::string();
  return path;
}
