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

#include "supertux/level.hpp"

#include "badguy/goldbomb.hpp"
#include "object/bonus_block.hpp"
#include "object/coin.hpp"
#include "supertux/sector.hpp"
#include "supertux/tile_manager.hpp"
#include "supertux/tile_set.hpp"
#include "trigger/secretarea_trigger.hpp"
#include "util/reader.hpp"
#include "util/reader_collection.hpp"
#include "util/reader_document.hpp"
#include "util/reader_mapping.hpp"

#include <sstream>
#include <stdexcept>

using namespace std;

std::unique_ptr<Level>
Level::from_file(const std::string& filename)
{
  std::unique_ptr<Level> level(new Level);
  level->load(filename);
  return level;
}

Level::Level() :
  name("noname"),
  author("Mr. X"),
  contact(),
  license(),
  filename(),
  on_menukey_script(),
  sectors(),
  stats(),
  target_time(),
  tileset("images/tiles.strf")
{
}

Level::~Level()
{
  sectors.clear();
}

void
Level::load(const std::string& filepath)
{
  try {
    filename = filepath;
    register_translation_directory(filepath);
    auto doc = ReaderDocument::parse(filepath);
    auto root = doc.get_root();

    if(root.get_name() != "supertux-level")
      throw std::runtime_error("file is not a supertux-level file.");

    auto level = root.get_mapping();

    int version = 1;
    level.get("version", version);
    if(version == 1) {
      log_info << "[" <<  filepath << "] level uses old format: version 1" << std::endl;
      load_old_format(level);
    } else if (version == 2) {
      level.get("tileset", tileset);

      level.get("name", name);
      level.get("author", author);
      level.get("contact", contact);
      level.get("license", license);
      level.get("on-menukey-script", on_menukey_script);
      level.get("target-time", target_time);

      auto iter = level.get_iter();
      while(iter.next()) {
        if (iter.get_key() == "sector") {
          std::unique_ptr<Sector> sector(new Sector(this));
          sector->parse(iter.as_mapping());
          add_sector(std::move(sector));
        }
      }

      if (license.empty()) {
        log_warning << "[" <<  filepath << "] The level author \"" << author
                    << "\" did not specify a license for this level \""
                    << name << "\". You might not be allowed to share it."
                    << std::endl;
      }
    } else {
      log_warning << "[" <<  filepath << "] level format version " << version << " is not supported" << std::endl;
    }
  } catch(std::exception& e) {
    std::stringstream msg;
    msg << "Problem when reading level '" << filepath << "': " << e.what();
    throw std::runtime_error(msg.str());
  }
}

void
Level::load_old_format(const ReaderMapping& reader)
{
  reader.get("name", name);
  reader.get("author", author);

  std::unique_ptr<Sector> sector(new Sector(this));
  sector->parse_old_format(reader);
  add_sector(std::move(sector));
}

void
Level::add_sector(std::unique_ptr<Sector> sector)
{
  Sector* test = get_sector(sector->get_name());
  if (test != nullptr) {
    throw std::runtime_error("Trying to add 2 sectors with same name");
  } else {
    sectors.push_back(std::move(sector));
  }
}

Sector*
Level::get_sector(const std::string& name_) const
{
  for(auto const& sector : sectors) {
    if(sector->get_name() == name_) {
      return sector.get();
    }
  }
  return nullptr;
}

size_t
Level::get_sector_count() const
{
  return sectors.size();
}

Sector*
Level::get_sector(size_t num) const
{
  return sectors.at(num).get();
}

int
Level::get_total_coins() const
{
  int total_coins = 0;
  for(auto const& sector : sectors) {
    for(auto o = sector->gameobjects.begin(); o != sector->gameobjects.end(); ++o) {
      Coin* coin = dynamic_cast<Coin*>(o->get());
      if(coin)
      {
        total_coins++;
        continue;
      }
      BonusBlock *block = dynamic_cast<BonusBlock*>(o->get());
      if(block)
      {
        if (block->contents == BonusBlock::CONTENT_COIN)
        {
          total_coins += block->hit_counter;
          continue;
        } else if (block->contents == BonusBlock::CONTENT_RAIN) {
          total_coins += 10;
          continue;
        } else if (block->contents == BonusBlock::CONTENT_EXPLODE) {
          total_coins += 10;
          continue;
        }
      }
      GoldBomb *goldbomb = dynamic_cast<GoldBomb*>(o->get());
      if(goldbomb)
        total_coins += 10;
    }
  }
  return total_coins;
}

int
Level::get_total_badguys() const
{
  int total_badguys = 0;
  for(auto const& sector : sectors) {
    total_badguys += sector->get_total_badguys();
  }
  return total_badguys;
}

int
Level::get_total_secrets() const
{
  int total_secrets = 0;
  for(auto const& sector : sectors) {
    total_secrets += sector->get_total_count<SecretAreaTrigger>();
  }
  return total_secrets;
}

/* EOF */
