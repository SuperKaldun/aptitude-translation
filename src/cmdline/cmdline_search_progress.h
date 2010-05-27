/** \file cmdline_search_progress.h */  // -*-c++-*-

// Copyright (C) 2010 Daniel Burrows
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; see the file COPYING.  If not, write to
// the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
// Boston, MA 02111-1307, USA.

#ifndef APTITUDE_CMDLINE_SEARCH_PROGRESS_H
#define APTITUDE_CMDLINE_SEARCH_PROGRESS_H

#include <string>

#include <boost/shared_ptr.hpp>

namespace aptitude
{
  namespace util
  {
    class progress_info;
  }

  namespace cmdline
  {
    class progress_display;
    class progress_throttle;

    void search_progress(const aptitude::util::progress_info &info,
                         const boost::shared_ptr<progress_display> &progress_msg,
                         const boost::shared_ptr<progress_throttle> &throttle,
                         const std::string &pattern);
  }
}

#endif // APTITUDE_CMDLINE_SEARCH_PROGRESS_H
