/*
	Copyright (c) 2024 KU Leuven
	Code author: Hendrik Vermuyten
*/



#include "data.h"
#include "tinyxml2.h"
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <chrono>
#include <cassert>

namespace IVM
{
	///////////////////////////////////////////
	///			  Problem data  			///
	///////////////////////////////////////////

	void Instance::read_data_xml(const std::string& filename)
	{
		std::string text;
		int status = 0;

		// XML Document
		tinyxml2::XMLDocument doc;
		status = doc.LoadFile(filename.c_str());
		if (status != tinyxml2::XML_SUCCESS)
			throw std::runtime_error("Error in function Instance::read_data(). Coulnd't load xml document \"" + filename + "\"");

		// Root node
		tinyxml2::XMLElement* rootnode = doc.RootElement();
		if(rootnode == nullptr)
			throw std::runtime_error("Error in function Instance::read_data(). XML does not contain root node");
		text = rootnode->Value();
		if(text != "Instantie")
			throw std::runtime_error("Error in function Instance::read_data(). XML root node is not named \"Instantie\"");

		// Attributes root node
		if(rootnode->Attribute("naam") == nullptr)
			throw std::runtime_error("Error in function Instance::read_data(). Instantie does not contain an attribute \"naam\"");
		_name = rootnode->Attribute("naam");

		if(rootnode->Attribute("aantal_dagen") == nullptr)
			throw std::runtime_error("Error in function Instance::read_data(). Instantie does not contain an attribute \"aantal_dagen\"");
		text = rootnode->Attribute("aantal_dagen");
		_nb_days = std::stoull(text);

		if(rootnode->Attribute("aantal_weken") == nullptr)
			throw std::runtime_error("Error in function Instance::read_data(). Instantie does not contain an attribute \"aantal_weken\"");
		text = rootnode->Attribute("aantal_weken");
		_nb_weeks = std::stoull(text);

		if(rootnode->Attribute("max_bezoeken") == nullptr)
			throw std::runtime_error("Error in function Instance::read_data(). Instantie does not contain an attribute \"max_bezoeken\"");
		text = rootnode->Attribute("max_bezoeken");
		_max_visits = std::stoull(text);


		// Child nodes
		tinyxml2::XMLElement* child;
		for (child = rootnode->FirstChildElement(); child; child = child->NextSiblingElement())
		{
			text = child->Value();
			if (text == "Afvaltype")
			{
				std::string waste_type;
				double lostijd;

				if(child->Attribute("naam") == nullptr)
					throw std::runtime_error("Error in function Instance::read_data(). Afvaltype does not contain an attribute \"naam\"");
				waste_type = child->Attribute("naam");

				if(child->Attribute("lostijd") == nullptr)
					throw std::runtime_error("Error in function Instance::read_data(). Afvaltype does not contain an attribute \"lostijd\"");
				text = child->Attribute("lostijd");
				lostijd = std::stod(text);

				_waste_types.push_back(waste_type);
				_waste_type_unloading_time[waste_type] = lostijd;
			}
			else if (text == "Trucktype")
			{
				_trucks.push_back(Truck());

				if(child->Attribute("naam") == nullptr)
					throw std::runtime_error("Error in function Instance::read_data(). Trucktype does not contain an attribute \"naam\"");
				_trucks.back()._name = child->Attribute("naam");

				if(child->Attribute("max_uren") == nullptr)
					throw std::runtime_error("Error in function Instance::read_data(). Trucktype does not contain an attribute \"max_uren\"");
				text = child->Attribute("max_uren");
				_trucks.back()._max_hours = std::stod(text);

				if(child->Attribute("vaste_kosten") == nullptr)
					throw std::runtime_error("Error in function Instance::read_data(). Trucktype does not contain an attribute \"vaste_kosten\"");
				text = child->Attribute("vaste_kosten");
				_trucks.back()._fixed_costs = std::stod(text);

				if (child->Attribute("variabele_kosten") == nullptr)
					throw std::runtime_error("Error in function Instance::read_data(). Trucktype does not contain an attribute \"variabele_kosten\"");
				text = child->Attribute("variabele_kosten");
				_trucks.back()._operating_costs = std::stod(text);

				tinyxml2::XMLElement* truckchild;
				for (truckchild = child->FirstChildElement(); truckchild; truckchild = truckchild->NextSiblingElement())
				{
					text = truckchild->Value();
					if (text == "Capaciteit")
					{
						std::string afvaltype; 
						double cap;

						if (truckchild->Attribute("afvaltype") == nullptr)
							throw std::runtime_error("Error in function Instance::read_data(). Capaciteit does not contain an attribute \"afvaltype\"");
						afvaltype = truckchild->Attribute("afvaltype");

						if(truckchild->Attribute("cap") == nullptr)
							throw std::runtime_error("Error in function Instance::read_data(). Capaciteit does not contain an attribute \"cap\"");
						text = truckchild->Attribute("cap");
						cap = std::stod(text);

						_trucks.back()._capacities[afvaltype] = cap;
					}
				}
			}
			else if (text == "Collectiepunt")
			{
				_collection_points.push_back(Collection_Point());

				std::string naamc;

				if (child->Attribute("naam") == nullptr)
					throw std::runtime_error("Error in function Instance::read_data(). Collectiepunt does not contain an attribute \"naam\"");
				naamc = child->Attribute("naam");
				_collection_points.back()._name = naamc;

				tinyxml2::XMLElement* collchild;
				for (collchild = child->FirstChildElement(); collchild; collchild = collchild->NextSiblingElement())
				{
					text = collchild->Value();
					if (text == "ToegelatenAfval")
					{
						std::string afvaltype;

						if (collchild->Attribute("naam") == nullptr)
							throw std::runtime_error("Error in function Instance::read_data(). ToegelatenAfval does not contain an attribute \"naam\"");
						afvaltype = collchild->Attribute("naam");

						_collection_points.back()._allowed_waste_types.push_back(afvaltype);
					}
					else if (text == "Rijtijd")
					{
						std::string bestemming;
						if(collchild->Attribute("naar") == nullptr)
							throw std::runtime_error("Error in function Instance::read_data(). Rijtijd does not contain an attribute \"naar\"");
						bestemming = collchild->Attribute("naar");
						if(bestemming != "Depot")
							throw std::runtime_error("Error in function Instance::read_data(). Attribute \"naar\" should have value \"Depot\"");
						
						if (collchild->Attribute("tijd") == nullptr)
							throw std::runtime_error("Error in function Instance::read_data(). Rijtijd does not contain an attribute \"tijd\"");
						text = collchild->Attribute("tijd");
						double time = std::stod(text);

						_collection_points.back()._driving_time_depot = time;
					}
				}
			}
			else if (text == "Zone")
			{
				_zones.push_back(Zone());

				if (child->Attribute("naam") == nullptr)
					throw std::runtime_error("Error in function Instance::read_data(). Zone does not contain an attribute \"naam\"");
				_zones.back()._name = child->Attribute("naam");

				tinyxml2::XMLElement* zonechild;
				for (zonechild = child->FirstChildElement(); zonechild; zonechild = zonechild->NextSiblingElement())
				{
					text = zonechild->Value();
					if (text == "Afval")
					{
						std::string afvaltype; 
						double hoeveelheid;
						double collectietijd;

						if(zonechild->Attribute("afvaltype") == nullptr)
							throw std::runtime_error("Error in function Instance::read_data(). Afval does not contain an attribute \"afvaltype\"");
						afvaltype = zonechild->Attribute("afvaltype");

						if(zonechild->Attribute("hoeveelheid") == nullptr)
							throw std::runtime_error("Error in function Instance::read_data(). Afval does not contain an attribute \"hoeveelheid\"");
						text = zonechild->Attribute("hoeveelheid");
						hoeveelheid = std::stod(text);

						if (zonechild->Attribute("collectietijd") == nullptr)
							throw std::runtime_error("Error in function Instance::read_data(). Afval does not contain an attribute \"collectietijd\"");
						text = zonechild->Attribute("collectietijd");
						collectietijd = std::stod(text);

						_zones.back()._demands[afvaltype] = hoeveelheid;
						_zones.back()._collection_times[afvaltype] = collectietijd;
					}
					else if (text == "HuidigeKalender")
					{
						std::string afvaltype;
						int dagindex;
						int week;

						if (zonechild->Attribute("afvaltype") == nullptr)
							throw std::runtime_error("Error in function Instance::read_data(). HuidigeKalender does not contain an attribute \"afvaltype\"");
						afvaltype = zonechild->Attribute("afvaltype");

						if (zonechild->Attribute("dag") == nullptr)
							throw std::runtime_error("Error in function Instance::read_data(). HuidigeKalender does not contain an attribute \"dag\"");
						text = zonechild->Attribute("dag");
						dagindex = _dag_naam_index.at(text);

						if (zonechild->Attribute("week") == nullptr)
							throw std::runtime_error("Error in function Instance::read_data(). HuidigeKalender does not contain an attribute \"week\"");
						text = zonechild->Attribute("week");
						week = std::stoi(text) - 1; // index starts at 1 in xml, but at 0 in code

						_zones.back()._current_calendar_day.insert(std::pair<std::string, int>(afvaltype, dagindex));
						_zones.back()._current_calendar_week.insert(std::pair<std::string, int>(afvaltype, week));
					}
					else if (text == "Rijtijd")
					{
						std::string destination;
						double time;

						if (zonechild->Attribute("naar") == nullptr)
							throw std::runtime_error("Error in function Instance::read_data(). Rijtijd does not contain an attribute \"naar\"");
						destination = zonechild->Attribute("naar");

						if (zonechild->Attribute("tijd") == nullptr)
							throw std::runtime_error("Error in function Instance::read_data(). Rijtijd does not contain an attribute \"tijd\"");
						text = zonechild->Attribute("tijd");
						time = std::stod(text);

						_zones.back()._driving_time[destination] = time;
					}
					else if (text == "VerbodenDag")
					{
						std::string dag;
						int dagindex;

						if (zonechild->Attribute("dag") == nullptr)
							throw std::runtime_error("Error in function Instance::read_data(). VerbodenDag does not contain an attribute \"dag\"");
						dag = zonechild->Attribute("dag");
						dagindex = _dag_naam_index.at(dag);

						_zones.back()._forbidden_days.push_back(dagindex);
					}
				}
			}
		}
	}

	void Instance::read_allocation_xml(const std::string& filename)
	{
		// Initialize vector with 0's 
		_sol_alloc_x_tmdw.clear();
		_sol_alloc_x_tmdw.reserve(nb_waste_types() * nb_zones() * nb_days() * nb_weeks());
		for (int t = 0; t < nb_waste_types(); ++t) {
			for (int m = 0; m < nb_zones(); ++m) {
				for (int d = 0; d < nb_days(); ++d) {
					for (int w = 0; w < nb_weeks(); ++w) {
						_sol_alloc_x_tmdw.push_back(0.0);
					}
				}
			}
		}


		std::string text;
		int status = 0;

		// XML Document
		tinyxml2::XMLDocument doc;
		status = doc.LoadFile(filename.c_str());
		if (status != tinyxml2::XML_SUCCESS)
			throw std::runtime_error("Error in function Instance::read_allocation_xml(). Coulnd't load xml document \"" + filename + "\"");

		// Root node
		tinyxml2::XMLElement* rootnode = doc.RootElement();
		if (rootnode == nullptr)
			throw std::runtime_error("Error in function Instance::read_allocation_xml(). XML does not contain root node");
		text = rootnode->Value();
		if (text != "Allocatie")
			throw std::runtime_error("Error in function Instance::read_allocation_xml(). XML root node is not named \"Allocatie\"");

		// Attributes root node
		if (rootnode->Attribute("instantie") == nullptr)
			throw std::runtime_error("Error in function Instance::read_allocation_xml(). Instantie does not contain an attribute \"instantie\"");
		text = rootnode->Attribute("instantie");
		if (_name != text)
			throw std::runtime_error("Error in function Instance::read_allocation_xml(). Instantie name for Allocatie is not equal to Instantie name for other data");
		// Other attributes (scenario, max_pct_veranderingen, max_rekentijd) not relevant here


	// Child nodes
		tinyxml2::XMLElement* child;
		for (child = rootnode->FirstChildElement(); child; child = child->NextSiblingElement())
		{
			text = child->Value();
			if (text != "Ophaling")
				throw std::runtime_error("Error in function Instance::read_allocation_xml(). Child of \"Allocatie\" should be \"Ophaling\"");

			if (child->Attribute("afval_type") == nullptr)
				throw std::runtime_error("Error in function Instance::read_allocation_xml(). Ophaling does not contain an attribute \"afval_type\"");
			std::string afvaltype = child->Attribute("afval_type");

			if (child->Attribute("zone") == nullptr)
				throw std::runtime_error("Error in function Instance::read_allocation_xml(). Ophaling does not contain an attribute \"zone\"");
			std::string zone = child->Attribute("zone");

			if (child->Attribute("dag") == nullptr)
				throw std::runtime_error("Error in function Instance::read_allocation_xml(). Ophaling does not contain an attribute \"dag\"");
			std::string daystr = child->Attribute("dag");
			int day = _dag_naam_index.at(daystr);

			if (child->Attribute("week") == nullptr)
				throw std::runtime_error("Error in function Instance::read_allocation_xml(). Ophaling does not contain an attribute \"week\"");
			std::string weekstr = child->Attribute("week");
			int week = std::stoi(weekstr) - 1;

			if (child->Attribute("hoeveelheid") == nullptr)
				throw std::runtime_error("Error in function Instance::read_allocation_xml(). Ophaling does not contain an attribute \"hoeveelheid\"");
			std::string hoeveelheidstr = child->Attribute("hoeveelheid");
			double hoeveelheid = std::stod(hoeveelheidstr);


			// zet om naar indices
			int waste_type_index, zone_index;
			for (int t = 0; t < nb_waste_types(); ++t) {
				if (waste_type(t) == afvaltype) {
					waste_type_index = t;
					break;
				}
			}
			for (int m = 0; m < nb_zones(); ++m) {
				if (zone_name(m) == zone) {
					zone_index = m;
					break;
				}
			}

			// sla op in vector
			_sol_alloc_x_tmdw[waste_type_index * nb_zones() * nb_days() * nb_weeks() + zone_index * nb_days() * nb_weeks() + day * nb_weeks() + week] = hoeveelheid;
		}
	}

	void Instance::read_routes_xml(const std::string& filename)
	{
		std::string text;
		int status = 0;

		// XML Document
		tinyxml2::XMLDocument doc;
		status = doc.LoadFile(filename.c_str());
		if (status != tinyxml2::XML_SUCCESS)
			throw std::runtime_error("Error in function Instance::read_routes_xml(). Coulnd't load xml document \"" + filename + "\"");

		// Root node
		tinyxml2::XMLElement* rootnode = doc.RootElement();
		if (rootnode == nullptr)
			throw std::runtime_error("Error in function Instance::read_routes_xml(). XML does not contain root node");
		text = rootnode->Value();
		if (text != "Routes")
			throw std::runtime_error("Error in function Instance::read_routes_xml(). XML root node is not named \"Routes\"");

		// Attributes root node
		if (rootnode->Attribute("instantie") == nullptr)
			throw std::runtime_error("Error in function Instance::read_routes_xml(). Instantie does not contain an attribute \"instantie\"");
		text = rootnode->Attribute("instantie");
		if (_name != text)
			throw std::runtime_error("Error in function Instance::read_routes_xml(). Instantie name for Routes is not equal to Instantie name for other data");
		// Other attributes (vaste_kosten, variabele_kosten, max_rekentijd, max_trucks_per_type, max_nb_segmenten) not relevant here


		// Child nodes
		tinyxml2::XMLElement* child;
		for (child = rootnode->FirstChildElement(); child; child = child->NextSiblingElement())
		{
			_routes.push_back(Route());

			text = child->Value();
			if (text != "Route")
				throw std::runtime_error("Error in function Instance::read_routes_xml(). Child of \"Routes\" should be \"Route\"");

			if (child->Attribute("afval_type") == nullptr)
				throw std::runtime_error("Error in function Instance::read_routes_xml(). Route does not contain an attribute \"afval_type\"");
			std::string afvaltype = child->Attribute("afval_type");
			_routes.back()._waste_type = afvaltype;

			if (child->Attribute("dag") == nullptr)
				throw std::runtime_error("Error in function Instance::read_routes_xml(). Route does not contain an attribute \"dag\"");
			std::string dagstr = child->Attribute("dag");
			int dag = _dag_naam_index.at(dagstr);
			_routes.back()._day = dag;

			if (child->Attribute("week") == nullptr)
				throw std::runtime_error("Error in function Instance::read_routes_xml(). Route does not contain an attribute \"week\"");
			std::string weekstr = child->Attribute("week");
			int week = std::stoi(weekstr) - 1;
			_routes.back()._week = week;

			// Pickups
			tinyxml2::XMLElement* pickup;
			for (pickup = child->FirstChildElement(); pickup; pickup = pickup->NextSiblingElement())
			{
				_routes.back()._pickups.push_back(std::pair<std::string, double>());

				text = child->Value();
				if (text != "Ophaling")
					throw std::runtime_error("Error in function Instance::read_routes_xml(). Child of \"Route\" should be \"Ophaling\"");

				if (child->Attribute("zone") == nullptr)
					throw std::runtime_error("Error in function Instance::read_routes_xml(). Ophaling does not contain an attribute \"zone\"");
				std::string zone = child->Attribute("zone");
				_routes.back()._pickups.back().first = zone;

				if (child->Attribute("hoeveelheid") == nullptr)
					throw std::runtime_error("Error in function Instance::read_routes_xml(). Ophaling does not contain an attribute \"hoeveelheid\"");
				std::string hoeveelheidstr = child->Attribute("hoeveelheid"); // in kg
				double hoeveelheid = std::stod(hoeveelheidstr);
				_routes.back()._pickups.back().second = hoeveelheid;
			}
		}
	}

	void Instance::clear_data()
	{
		_waste_types.clear();
		_waste_type_unloading_time.clear();
		_collection_points.clear();
		_trucks.clear();
		_zones.clear();

		_sol_alloc_x_tmdw.clear();
		_routes.clear();
	}

	bool Instance::current_calendar(size_t zone, const std::string& waste_type, size_t day, size_t week) const
	{
		bool dayfound = false;
		auto range_day = _zones[zone]._current_calendar_day.equal_range(waste_type);
		for (auto&& it = range_day.first; it != range_day.second; ++it) {
			if (it->second == day) {
				dayfound = true;
				break;
			}
		}

		bool weekfound = false;
		auto range_week = _zones[zone]._current_calendar_week.equal_range(waste_type);
		for (auto&& it = range_week.first; it != range_week.second; ++it) {
			if (it->second == week) {
				weekfound = true;
				break;
			}
		}

		return (dayfound && weekfound);
	}

	size_t Instance::nb_pickups_current_calendar() const
	{
		int result = 0;

		for (int t = 0; t < _waste_types.size(); ++t)
		{
			for (int m = 0; m < _zones.size(); ++m)
			{
				for (int d = 0; d < _nb_days; ++d)
				{
					for (int w = 0; w < _nb_weeks; ++w)
					{
						const std::string& waste_type = _waste_types[t];
						if (current_calendar(m, waste_type, d, w))
							++result;
					}
				}
			}
		}

		return result;
	}

	bool Instance::collection_point_waste_type_allowed(size_t index, const std::string& waste_type) const
	{
		auto& vec = _collection_points[index]._allowed_waste_types;
		if (std::find(vec.begin(), vec.end(), waste_type) != vec.end())
			return true;
		return false;
	}

	bool Instance::route_visits_zone(size_t index_route, size_t index_zone) const
	{
		for (auto&& v : _routes[index_route]._pickups)
		{
			if (v.first == _zones[index_zone]._name)
				return true;
		}
		return false;
	}

	double Instance::x_tmdw(size_t waste_type, size_t zone, size_t day, size_t week) const
	{ 
		assert(waste_type < _waste_types.size());
		assert(zone < _zones.size());
		assert(day < _nb_days);
		assert(week < _nb_weeks);

		return _sol_alloc_x_tmdw.at(waste_type * _zones.size() * _nb_days * _nb_weeks + zone * _nb_days * _nb_weeks + day * _nb_weeks + week);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////

}