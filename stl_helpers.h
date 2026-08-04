/*
 * This source file is part of the Atlantis Little Helper program.
 * Copyright (C) 2001 Maxim Shariy.
 *
 * Atlantis Little Helper is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Atlantis Little Helper is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Atlantis Little Helper; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __AH_STL_HELPERS_H__
#define __AH_STL_HELPERS_H__

#include <string>

// Comparator for case-insensitive std::set<std::string>
struct CaseInsensitiveLess {
    bool operator()(const std::string& a, const std::string& b) const {
#ifdef _WIN32
        return _stricmp(a.c_str(), b.c_str()) < 0;
#else
        return strcasecmp(a.c_str(), b.c_str()) < 0;
#endif
    }
};

#endif // __AH_STL_HELPERS_H__
