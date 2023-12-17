/*
	Copyright (c) 2023 Hendrik Vermuyten

	Licensed under the Apache License, Version 2.0 (the "License");
	you may not use this file except in compliance with the License.
	You may obtain a copy of the License at

	http://www.apache.org/licenses/LICENSE-2.0
*/



#include "data.h"
#include "tinyxml2.h"
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <random>
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
				std::string naamc;
				double lostijd;

				if (child->Attribute("naam") == nullptr)
					throw std::runtime_error("Error in function Instance::read_data(). Collectiepunt does not contain an attribute \"naam\"");
				naamc = child->Attribute("naam");

				_collection_points.push_back(naamc);
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
						std::string dag;
						int dagindex;
						int week;

						if (zonechild->Attribute("afvaltype") == nullptr)
							throw std::runtime_error("Error in function Instance::read_data(). HuidigeKalender does not contain an attribute \"afvaltype\"");
						afvaltype = zonechild->Attribute("afvaltype");

						if (zonechild->Attribute("dag") == nullptr)
							throw std::runtime_error("Error in function Instance::read_data(). HuidigeKalender does not contain an attribute \"dag\"");
						dag = zonechild->Attribute("dag");
						dagindex = _dag_naam_index.at(dag);

						if (zonechild->Attribute("week") == nullptr)
							throw std::runtime_error("Error in function Instance::read_data(). HuidigeKalender does not contain an attribute \"week\"");
						text = zonechild->Attribute("week");
						week = std::stoi(text) - 1; // index starts at 1 in xml, but at 0 in code

						_zones.back()._current_calendar_day[afvaltype] = dagindex;
						_zones.back()._current_calendar_week[afvaltype] = week;
					}
					else if (text == "Rijtijd")
					{
						std::string destination;
						double time;

						if (zonechild->Attribute("naar") == nullptr)
							throw std::runtime_error("Error in function Instance::read_data(). HuidigeKalender does not contain an attribute \"naar\"");
						destination = zonechild->Attribute("naar");

						if (zonechild->Attribute("tijd") == nullptr)
							throw std::runtime_error("Error in function Instance::read_data(). HuidigeKalender does not contain an attribute \"tijd\"");
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

	void Instance::clear_data()
	{
		_waste_types.clear();
		_waste_type_unloading_time.clear();
		_collection_points.clear();
		_trucks.clear();
		_zones.clear();

		_sol_alloc_x_tmdw.clear();
	}

	bool Instance::current_calendar(size_t zone, const std::string& waste_type, size_t day, size_t week) const
	{
		if (_zones[zone]._current_calendar_day.at(waste_type) == day
			&& _zones[zone]._current_calendar_week.at(waste_type) == week)
			return true;

		return false;
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

	double Instance::x_tmdw(size_t waste_type, size_t zone, size_t day, size_t week) const
	{ 
		assert(waste_type < _waste_types.size());
		assert(zone < _zones.size());
		assert(day < _nb_days);
		assert(week < _nb_weeks);

		return _sol_alloc_x_tmdw.at(waste_type * _zones.size() * _nb_days * _nb_weeks + zone * _nb_days * _nb_weeks + day * _nb_weeks + week);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////



	///////////////////////////////////////////
	///			  Instance Generator		///
	///////////////////////////////////////////

	void Instance_Generator::generate_xml()
	{
		std::random_device randdev;
		std::seed_seq seed{ randdev(),randdev(), randdev(), randdev(), randdev(), randdev(), randdev() };
		std::mt19937_64 engine(seed);

		std::uniform_int_distribution<> dist_demand_rest(5,25);
		std::uniform_int_distribution<> dist_demand_gft(2,10);
		std::uniform_int_distribution<> dist_drive_time(1,8); // echte waarde tussen 0.1 en 0.8 dus factor 10 kleiner
		std::uniform_int_distribution<> dist_collection_time(17, 27);	// echte waarde tussen 1.7 en 2.7 dus factor 10 kleiner
		std::uniform_int_distribution<> dist_costs(1, 10); // vermenigvuldig met 100 voor fixed costs en met 10 voor variable costs
		std::uniform_int_distribution<> dist_day(0, 4); 

		const std::vector<std::string> day_names{ "maandag","dinsdag","woensdag","donderdag","vrijdag" };
		
		const int max_visits = 1;
		const double max_time = 7.5;
		const double unloading_time = 0.25; // Jens

		const double fixedcosts = dist_costs(engine) * 100;
		const double operatingcosts = dist_costs(engine) * 10;
		const double capacity_truckgft_gft = 10;
		const double capacity_truckgft_rest = 12.5;
		const double capacity_truckrest_gft = 0;
		const double capacity_truckrest_rest = 25;
		

		std::ofstream file;
		file.open("random_instance.xml");
		if (!file.is_open())
		{
			std::cout << "\n\n\nError in Instance_Generator::generate_xml(). Couldn't open file";
			return;
		}

		file << "<?xml version=\"1.0\"?>\n<Instantie naam=\"Random\" aantal_dagen=\"" << _nb_days << "\" aantal_weken=\"" << _nb_weeks << "\" max_bezoeken=\"" << max_visits << "\">"
			<< "\n\t<Afvaltype naam=\"GFT\" lostijd=\"0.25\"/>\n\t<Afvaltype naam=\"restafval\" lostijd=\"0.25\"/>"
			<< "\n\t<Trucktype naam=\"truck_GFT\" max_uren=\"" << max_time << "\" vaste_kosten=\"" << fixedcosts << "\" variabele_kosten=\"" << operatingcosts << "\">"
			<< "\n\t\t<Capaciteit afvaltype=\"GFT\" cap=\"" << capacity_truckgft_gft << "\"/>"
			<< "\n\t\t<Capaciteit afvaltype=\"restafval\" cap=\"" << capacity_truckgft_rest << "\"/>"
			<< "\n\t</Trucktype>"
			<< "\n\t<Trucktype naam=\"truck_restafval\" max_uren=\"" << max_time << "\" vaste_kosten=\"" << fixedcosts << "\" variabele_kosten=\"" << operatingcosts << "\">"
			<< "\n\t\t<Capaciteit afvaltype=\"GFT\" cap=\"" << capacity_truckrest_gft << "\"/>"
			<< "\n\t\t<Capaciteit afvaltype=\"restafval\" cap=\"" << capacity_truckrest_rest << "\"/>"
			<< "\n\t</Trucktype>";

		// Collection Points
		for (int i = 0; i < _nb_collection_points; ++i)
		{
			file << "\n\t<Collectiepunt naam=\"CP" << i+1 << "\"/>";
		}

		// Customers (Zones)
		for (int i = 0; i < _nb_zones; ++i)
		{
			double demand_gft = static_cast<double>(dist_demand_gft(engine)) / 10.0;
			double demand_restafval = static_cast<double>(dist_demand_rest(engine)) / 10.0;
			double collectiontimerest = static_cast<double>(dist_collection_time(engine)) / 10.0;
			double collectiontimegft = static_cast<double>(dist_collection_time(engine)) / 10.0;
			int current_day = dist_day(engine);

			file << "\n\t<Zone naam=\"Z" << i + 1 << "\">"
				<< "\n\t\t<Afval afvaltype=\"GFT\" hoeveelheid=\"" << demand_gft << "\" collectietijd=\"" << collectiontimegft << "\"/>"
				<< "\n\t\t<Afval afvaltype=\"restafval\" hoeveelheid=\"" << demand_restafval << "\" collectietijd=\"" << collectiontimerest << "\"/>"
				<< "\n\t\t<HuidigeKalender afvaltype=\"restafval\" dag=\"" << day_names[current_day] << "\" week=\"1\"/>"
				<< "\n\t\t<HuidigeKalender afvaltype=\"GFT\" dag=\"" << day_names[current_day] << "\" week=\"2\"/>";

			file << "\n\t\t<Rijtijd naar=\"Depot" << "\" tijd=\"" << static_cast<double>(dist_drive_time(engine)) / 10.0 << "\"/>";

			for (int d = 0; d < _nb_collection_points; ++d)
				file << "\n\t\t<Rijtijd naar=\"CP" << d + 1 << "\" tijd=\"" << static_cast<double>(dist_drive_time(engine)) / 10.0 << "\"/>";


			file << "\n\t</Zone>";
		}

		file << "\n</Instantie>";
	}

	void Instance_Generator::change_parameters(size_t nb_zones, size_t nb_collection_points, size_t nb_days, size_t nb_weeks)
	{
		_nb_zones = nb_zones;
		_nb_collection_points = nb_collection_points;
		_nb_days = nb_days;
		_nb_weeks = nb_weeks;
	}
}