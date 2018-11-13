//  SuperTux
//  Copyright (C) 2006 Matthias Braun <matze@braunis.de>
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "video/video_system.hpp"

#include <assert.h>
#include <boost/optional.hpp>
#include <config.h>
#include <iomanip>
#include <physfs.h>
#include <sstream>

#include "util/log.hpp"
#include "video/sdl/sdl_video_system.hpp"
#include "video/sdl_surface.hpp"
#include "video/sdl_surface_ptr.hpp"

#ifdef HAVE_OPENGL
#  include "video/gl/gl_video_system.hpp"
#endif

std::unique_ptr<VideoSystem>
VideoSystem::create(VideoSystem::Enum video_system)
{
  switch (video_system)
  {
    case VIDEO_AUTO:
#ifdef HAVE_OPENGL
      try
      {
        return std::make_unique<GLVideoSystem>(true);
      }
      catch(std::exception& err)
      {
        try
        {
          log_warning << "Error creating GLVideoSystem-330core, using GLVideoSystem-20 fallback: "  << err.what() << std::endl;
          return std::make_unique<GLVideoSystem>(false);
        }
        catch(std::exception& err2)
        {
          log_warning << "Error creating GLVideoSystem-20, using SDL fallback: "  << err2.what() << std::endl;
          return std::make_unique<SDLVideoSystem>();
        }
      }
#else
      log_info << "new SDL renderer\n";
      return std::make_unique<SDLVideoSystem>();
#endif

#ifdef HAVE_OPENGL
    case VIDEO_OPENGL33CORE:
      return std::make_unique<GLVideoSystem>(true);

    case VIDEO_OPENGL20:
      return std::make_unique<GLVideoSystem>(false);
#else
    case VIDEO_OPENGL33CORE:
    case VIDEO_OPENGL20:
      log_warning << "OpenGL requested, but missing using SDL fallback" << std::endl;
      return std::make_unique<SDLVideoSystem>();
#endif

    case VIDEO_SDL:
      log_info << "new SDL renderer\n";
      return std::make_unique<SDLVideoSystem>();

    default:
      log_fatal << "invalid video system in config" << std::endl;
      assert(false);
      return {};
  }
}

VideoSystem::Enum
VideoSystem::get_video_system(const std::string &video)
{
  if (video == "auto")
  {
    return VIDEO_AUTO;
  }
#ifdef HAVE_OPENGL
  else if (video == "opengl" || video == "opengl33" || video == "opengl33core")
  {
    return VIDEO_OPENGL33CORE;
  }
  else if (video == "opengl20")
  {
    return VIDEO_OPENGL20;
  }
#endif
  else if (video == "sdl")
  {
    return VIDEO_SDL;
  }
  else
  {
#ifdef HAVE_OPENGL
    throw std::runtime_error("invalid VideoSystem::Enum, valid values are 'auto', 'sdl' and 'opengl'");
#else
    throw std::runtime_error("invalid VideoSystem::Enum, valid values are 'auto' and 'sdl'");
#endif
  }
}

std::string
VideoSystem::get_video_string(VideoSystem::Enum video)
{
  switch (video)
  {
    case VIDEO_AUTO:
      return "auto";
    case VIDEO_OPENGL33CORE:
      return "opengl";
    case VIDEO_OPENGL20:
      return "opengl20";
    case VIDEO_SDL:
      return "sdl";
    default:
      log_fatal << "invalid video system in config" << std::endl;
      assert(false);
      return "auto";
  }
}

void
VideoSystem::do_take_screenshot()
{
  SDLSurfacePtr surface = make_screenshot();
  if (!surface) {
    log_warning << "Creating the screenshot has failed" << std::endl;
    return;
  }

  auto find_filename = [&]() -> boost::optional<std::string>
    {
      const std::string writeDir = PHYSFS_getWriteDir();
      const std::string dirSep = PHYSFS_getDirSeparator();
      const std::string baseName = "screenshot";
      const std::string fileExt = ".png";

      for (int num = 0; num < 1000000; ++num)
      {
        std::ostringstream oss;
        oss << baseName << std::setw(6) << std::setfill('0') << num << fileExt;
        if (!PHYSFS_exists(oss.str().c_str()))
        {
          std::ostringstream fullpath;
          fullpath << writeDir << dirSep << oss.str();
          return fullpath.str();
        }
      }

      return boost::none;
    };

  auto filename = find_filename();
  if (!filename)
  {
    log_info << "Failed to find filename to save screenshot" << std::endl;
  }
  else
  {
    if (SDLSurface::save_png(*surface, *filename)) {
      log_info << "Wrote screenshot to \"" << *filename << "\"" << std::endl;
    }
  }
}

/* EOF */
