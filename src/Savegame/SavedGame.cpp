/*
 * Copyright 2010 OpenXcom Developers.
 *
 * This file is part of OpenXcom.
 *
 * OpenXcom is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * OpenXcom is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenXcom.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "SavedGame.h"
#include <fstream>
#include "yaml.h"
#include "../Ruleset/Ruleset.h"
#include "../Engine/RNG.h"
#include "GameTime.h"
#include "Country.h"
#include "Base.h"
#include "Region.h"
#include "Ufo.h"
#include "Waypoint.h"

namespace OpenXcom
{

/**
 * Initializes a brand new saved game according to the specified difficulty.
 * @param difficulty Game difficulty.
 */
SavedGame::SavedGame(GameDifficulty difficulty) : _difficulty(difficulty), _funds(0), _countries(), _regions(), _bases(), _ufos(), _craftId(), _waypoints(), _ufoId(1), _waypointId(1), _battleGame(0)
{
	_time = new GameTime(6, 1, 1, 1999, 12, 0, 0);
	RNG::init();
}

/** 
 * Deletes the game content from memory.
 */
SavedGame::~SavedGame()
{
	for (std::vector<Country*>::iterator i = _countries.begin(); i != _countries.end(); i++)
	{
		delete *i;
	}
	for (std::vector<Region*>::iterator i = _regions.begin(); i != _regions.end(); i++)
	{
		delete *i;
	}
	for (std::vector<Base*>::iterator i = _bases.begin(); i != _bases.end(); i++)
	{
		delete *i;
	}
	for (std::vector<Ufo*>::iterator i = _ufos.begin(); i != _ufos.end(); i++)
	{
		delete *i;
	}
	for (std::vector<Waypoint*>::iterator i = _waypoints.begin(); i != _waypoints.end(); i++)
	{
		delete *i;
	}
}

/**
 * Loads a saved game's contents from a YAML file.
 * @note Assumes the saved game is blank.
 * @param filename YAML filename.
 * @param rule Ruleset for the saved game.
 */
void SavedGame::load(const std::string &filename, Ruleset *rule)
{
	unsigned int size = 0;

	std::string s = "./USER/" + filename + ".sav";
	std::ifstream fin(s.c_str());
	if (!fin)
	{
		throw "Failed to load savegame";
	}
    YAML::Parser parser(fin);
	YAML::Node doc;

	// Get brief save info
    parser.GetNextDocument(doc);
	_time->load(doc["time"]);

	// Get full save data
	parser.GetNextDocument(doc);
	int a;
	doc["difficulty"] >> a;
	_difficulty = (GameDifficulty)a;
	doc["funds"] >> _funds;

	size = doc["countries"].size();
	for (unsigned i = 0; i < size; i++)
	{
		std::string type;
		doc["countries"][i]["type"] >> type;
		Country *c = new Country(rule->getCountry(type));
		c->load(doc["countries"][i]);
		_countries.push_back(c);
	}

	size = doc["regions"].size();
	for (unsigned i = 0; i < size; i++)
	{
		std::string type;
		doc["regions"][i]["type"] >> type;
		Region *r = new Region(rule->getRegion(type));
		r->load(doc["regions"][i]);
		_regions.push_back(r);
	}
	
	size = doc["ufos"].size();
	for (unsigned i = 0; i < size; i++)
	{
		std::string type;
		doc["ufos"][i]["type"] >> type;
		Ufo *u = new Ufo(rule->getUfo(type));
		u->load(doc["ufos"][i]);
		_ufos.push_back(u);
	}

	doc["craftId"] >> _craftId;

	size = doc["waypoints"].size();
	for (unsigned i = 0; i < size; i++)
	{
		Waypoint *w = new Waypoint();
		w->load(doc["waypoints"][i]);
		_waypoints.push_back(w);
	}

	doc["ufoId"] >> _ufoId;
	doc["waypointId"] >> _waypointId;

	size = doc["bases"].size();
	for (unsigned i = 0; i < size; i++)
	{
		Base *b = new Base();
		b->load(doc["bases"][i], rule, this);
		_bases.push_back(b);
	}

	if (const YAML::Node *pName = doc.FindValue("battleGame"))
	{
		_battleGame = new SavedBattleGame();
		_battleGame->load(*pName);
	}
	
	fin.close();
}

/**
 * Saves a saved game's contents to a YAML file.
 * @param filename YAML filename.
 */
void SavedGame::save(const std::string &filename) const
{
	std::string s = "./USER/" + filename + ".sav";
	std::ofstream sav(s.c_str());
	if (!sav)
	{
		throw "Failed to save savegame";
	}

	YAML::Emitter outBrief, outSave;

	// Saves the brief game info used in the saves list
	outBrief << YAML::BeginMap;
	outBrief << YAML::Key << "version" << YAML::Value << "0.2";
	outBrief << YAML::Key << "time" << YAML::Value;
	_time->save(outBrief);
	outBrief << YAML::EndMap;

	// Saves the full game data to the save
	outSave << YAML::BeginMap;
	outSave << YAML::Key << "difficulty" << YAML::Value << _difficulty;
	outSave << YAML::Key << "funds" << YAML::Value << _funds;
	outSave << YAML::Key << "countries" << YAML::Value;
	outSave << YAML::BeginSeq;
	for (std::vector<Country*>::const_iterator i = _countries.begin(); i != _countries.end(); i++)
	{
		(*i)->save(outSave);
	}
	outSave << YAML::EndSeq;
	outSave << YAML::Key << "regions" << YAML::Value;
	outSave << YAML::BeginSeq;
	for (std::vector<Region*>::const_iterator i = _regions.begin(); i != _regions.end(); i++)
	{
		(*i)->save(outSave);
	}
	outSave << YAML::EndSeq;
	outSave << YAML::Key << "bases" << YAML::Value;
	outSave << YAML::BeginSeq;
	for (std::vector<Base*>::const_iterator i = _bases.begin(); i != _bases.end(); i++)
	{
		(*i)->save(outSave);
	}
	outSave << YAML::EndSeq;
	outSave << YAML::Key << "ufos" << YAML::Value;
	outSave << YAML::BeginSeq;
	for (std::vector<Ufo*>::const_iterator i = _ufos.begin(); i != _ufos.end(); i++)
	{
		(*i)->save(outSave);
	}
	outSave << YAML::EndSeq;
	outSave << YAML::Key << "craftId" << YAML::Value << _craftId;
	outSave << YAML::Key << "waypoints" << YAML::Value;
	outSave << YAML::BeginSeq;
	for (std::vector<Waypoint*>::const_iterator i = _waypoints.begin(); i != _waypoints.end(); i++)
	{
		(*i)->save(outSave);
	}
	outSave << YAML::EndSeq;
	outSave << YAML::Key << "ufoId" << YAML::Value << _ufoId;
	outSave << YAML::Key << "waypointId" << YAML::Value << _waypointId;
	if (_battleGame != 0)
	{
		outSave << YAML::Key << "battleGame" << YAML::Value;
		_battleGame->save(outSave);
	}
	outSave << YAML::EndMap;

	sav << outBrief.c_str() << std::endl << outSave.c_str();
	sav.close();
}

/**
 * Returns the player's current funds.
 * @return Current funds.
 */
int SavedGame::getFunds() const
{
	return _funds;
}

/**
 * Changes the player's funds to a new value.
 * @param funds New funds.
 */
void SavedGame::setFunds(int funds)
{
	_funds = funds;
}

/**
 * Gives the player his monthly funds, taking in account
 * all maintenance and profit costs.
 */
void SavedGame::monthlyFunding()
{
	_funds += getCountryFunding() - getBaseMaintenance();
}

/**
 * Returns the current time of the game.
 * @return Pointer to the game time.
 */
GameTime *const SavedGame::getTime() const
{
	return _time;
}

/**
 * Returns the list of countries in the game world.
 * @return Pointer to country list.
 */
std::vector<Country*> *const SavedGame::getCountries()
{
	return &_countries;
}

/**
 * Adds up the monthly funding of all the countries.
 * @return Total funding.
 */
int SavedGame::getCountryFunding() const
{
	int total = 0;
	for (std::vector<Country*>::const_iterator i = _countries.begin(); i != _countries.end(); i++)
	{
		total += (*i)->getFunding();
	}
	return total;
}

/**
 * Returns the list of world regions.
 * @return Pointer to region list.
 */
std::vector<Region*> *const SavedGame::getRegions()
{
	return &_regions;
}

/**
 * Returns the list of player bases.
 * @return Pointer to base list.
 */
std::vector<Base*> *const SavedGame::getBases()
{
	return &_bases;
}

/**
 * Adds up the monthly maintenance of all the bases.
 * @return Total maintenance.
 */
int SavedGame::getBaseMaintenance() const
{
	int total = 0;
	for (std::vector<Base*>::const_iterator i = _bases.begin(); i != _bases.end(); i++)
	{
		total += (*i)->getMonthlyMaintenace();
	}
	return total;
}

/**
 * Returns the latest craft IDs for each type.
 * @return Pointer to ID list.
 */
std::map<std::string, int> *const SavedGame::getCraftIds()
{
	return &_craftId;
}

/**
 * Returns the list of alien UFOs.
 * @return Pointer to UFO list.
 */
std::vector<Ufo*> *const SavedGame::getUfos()
{
	return &_ufos;
}

/**
 * Returns the latest ufo ID.
 * @return Pointer to ID value.
 */
int *const SavedGame::getUfoId()
{
	return &_ufoId;
}

/**
 * Returns the latest waypoint ID.
 * @return Pointer to ID value.
 */
int *const SavedGame::getWaypointId()
{
	return &_waypointId;
}

/**
 * Returns the list of craft waypoints.
 * @return Pointer to waypoint list.
 */
std::vector<Waypoint*> *const SavedGame::getWaypoints()
{
	return &_waypoints;
}

/**
 * Set battleGame object.
 * @param battleGame Pointer to the battleGame object.
 */
void SavedGame::setBattleGame(SavedBattleGame *battleGame)
{
	_battleGame = battleGame;
}

/**
 * Get pointer to the battleGame object.
 * @return Pointer to the battleGame object.
 */
SavedBattleGame *SavedGame::getBattleGame()
{
	return _battleGame;
}

}
