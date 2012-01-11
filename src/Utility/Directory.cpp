/*
    Copyright © 2007, 2008, 2009, 2010, 2011, 2012
              Vladimír Vondruš <mosra@centrum.cz>

    This file is part of Corrade.

    Corrade is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License version 3
    only, as published by the Free Software Foundation.

    Corrade is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU Lesser General Public License version 3 for more details.
*/

#include "Directory.h"

#include <dirent.h>
#include <sys/stat.h>
#include <cstdio>
#include <algorithm>

#ifdef _WIN32
#include <shlobj.h>
#endif

#include "utilities.h"

using namespace std;

namespace Corrade { namespace Utility {

string Directory::path(const std::string& filename) {
    /* If filename is already a path, return it */
    if(!filename.empty() && filename[filename.size()-1] == '/')
        return filename.substr(0, filename.size()-1);

    size_t pos = filename.find_last_of('/');

    /* Filename doesn't contain any slash (no path), return empty string */
    if(pos == string::npos) return "";

    /* Return everything to last slash */
    return filename.substr(0, pos);
}

string Directory::filename(const std::string& filename) {
    size_t pos = filename.find_last_of('/');

    /* Return whole filename if it doesn't contain slash */
    if(pos == string::npos) return filename;

    /* Return everything after last slash */
    return filename.substr(pos+1);
}

string Directory::join(const std::string& path, const std::string& filename) {
    /* Absolute filename or empty path, return filename */
    if(path.empty() || (!filename.empty() && filename[0] == '/'))
        return filename;

    /* Add leading slash to path, if not present */
    if(path[path.size()-1] != '/')
        return path + '/' + filename;

    return path + filename;
}

bool Directory::mkpath(const std::string& _path) {
    if(_path.empty()) return false;

    /* If path contains trailing slash, strip it */
    if(_path[_path.size()-1] == '/')
        return mkpath(_path.substr(0, _path.size()-1));

    /* If parent directory doesn't exist, create it */
    string parentPath = path(_path);
    if(!parentPath.empty()) {
        DIR* directory = opendir(parentPath.c_str());
        if(directory == 0 && !mkpath(parentPath)) return false;
        closedir(directory);
    }

    /* Create directory */
    #ifndef _WIN32
    int ret = mkdir(_path.c_str(), 0777);
    #else
    int ret = mkdir(_path.c_str());
    #endif

    /* Directory is successfully created or already exists */
    if(ret == 0 || ret == -1) return true;

    return false;
}

bool Directory::rm(const string& path) {
    if(remove(path.c_str()) == 0) return true;
    return false;
}

bool Directory::fileExists(const std::string& filename) {
  struct stat fileInfo;

  if(stat(filename.c_str(), &fileInfo) == 0) return true;
  return false;
}

string Directory::home() {
    #ifndef _WIN32
    char* h = getenv("HOME");
    if(!h) return "";
    #else
    /** @bug Doesn't work at all */
    TCHAR h[MAX_PATH];
    if(!SUCCEEDED(SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, 0, h)))
        return "";
    #endif

    return h;
}

string Directory::configurationDir(const std::string& applicationName, bool createIfNotExists) {
    #ifndef _WIN32
    string h = home();
    if(h.empty()) return "";
    string dir = join(h, '.' + lowercase(applicationName));
    #else
    TCHAR path[MAX_PATH];
    if(!SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, path)))
        return "";
    string appdata = path;
    if(appdata.empty()) return "";
    string dir = join(appdata, applicationName);
    #endif

    if(createIfNotExists) mkpath(dir);
    return dir;
}

Directory::Directory(const string& path, int flags): _isLoaded(false) {
    DIR* directory;
    dirent* entry;

    directory = opendir(path.c_str());
    if(directory == 0) return;

    while((entry = readdir(directory)) != 0) {
        #ifndef _WIN32
        if((flags & SkipDotAndDotDot) && (string(entry->d_name) == "." || string(entry->d_name) == ".."))
            continue;
        if((flags & SkipDirectories) && entry->d_type == DT_DIR)
            continue;
        if((flags & SkipFiles) && entry->d_type == DT_REG)
            continue;
        if((flags & SkipSpecial) && entry->d_type != DT_DIR && entry->d_type != DT_REG)
            continue;
        #endif

        /** @todo On some systems dirent returns DT_UNKNOWN for everything */
        push_back(entry->d_name);
    }

    closedir(directory);

    if(flags & SortAscending) sort(begin(), end());
    if(flags & SortDescending) sort(rbegin(), rend());

    _isLoaded = true;
}

}}
