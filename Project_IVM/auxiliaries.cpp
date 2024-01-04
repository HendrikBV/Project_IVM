/*
	Copyright (c) 2023 Hendrik Vermuyten

	Licensed under the Apache License, Version 2.0 (the "License");
	you may not use this file except in compliance with the License.
	You may obtain a copy of the License at

	http://www.apache.org/licenses/LICENSE-2.0
*/



#include "auxiliaries.h"
#include <fstream>
#include <iostream>
#include <random>

namespace aux
{
	///////////////////////////////////////////
	///			   TransformerXML  			///
	///////////////////////////////////////////

	void TransformerXML::transform(const std::string& inputfile, const std::string& outputfile)
	{
		read_data(inputfile);
		generate_xml(outputfile);
	}

	void TransformerXML::read_data(const std::string& inputfile)
	{
		std::ifstream file;
		file.open(inputfile);
		if (!file.is_open())
		{
			std::cout << "\n\n\nCouldn't open input file";
			return;
		}

		std::string naam;
		int perceel;
		double gft;
		double restafval;
		int gft_week1;
		int gft_week2;
		int rest_week1;
		int rest_week2;
		int verboden_dag;
		double t_depot;
		double t_cp1;
		double t_cp2;
		double t_cp3;
		double t_cp4;

		while (file >> naam >> perceel >> gft >> restafval >> gft_week1 >> gft_week2 >> rest_week1 >> rest_week2 >> verboden_dag >> t_depot >> t_cp1 >> t_cp2 >> t_cp3 >> t_cp4)
		{
			_zones.push_back(Zone());
			_zones.back().naam = naam;
			_zones.back().gft = gft;
			_zones.back().restafval = restafval;
			_zones.back().gft_week1 = gft_week1;
			_zones.back().gft_week2 = gft_week2;
			_zones.back().rest_week1 = rest_week1;
			_zones.back().rest_week2 = rest_week2;
			_zones.back().verboden_dag = verboden_dag;
			_zones.back().t_depot = t_depot;
			_zones.back().t_cp1 = t_cp1;
			_zones.back().t_cp2 = t_cp2;
			_zones.back().t_cp3 = t_cp3;
			_zones.back().t_cp4 = t_cp4;
		}
	}

	void TransformerXML::generate_xml(const std::string& outputfile)
	{
		std::ofstream file;
		file.open(outputfile);
		if (!file.is_open())
		{
			std::cout << "\n\n\nCouldn't open xml file";
			return;
		}

		std::string name_instance = outputfile;
		name_instance.erase(std::find(name_instance.begin(), name_instance.end(), '.'), name_instance.end());

		// General information (vaste data al geimplementeerd)
		file << "<?xml version=\"1.0\"?>\n<Instantie naam=\"" << name_instance << "\" aantal_dagen=\"5\" aantal_weken=\"2\" max_bezoeken=\"1\">"
			<< "\n\t<Afvaltype naam=\"GFT\" lostijd=\"0.17\"/>"
			<< "\n\t<Afvaltype naam=\"restafval\" lostijd=\"0.17\"/>"
			<< "\n\t<Trucktype naam=\"truck_GFT\" max_uren=\"7.5\" vaste_kosten=\"100\" variabele_kosten=\"87\">"
			<< "\n\t\t<Capaciteit afvaltype=\"GFT\" cap=\"10.2\"/>"
			<< "\n\t\t<Capaciteit afvaltype=\"restafval\" cap=\"0\"/>"
			<< "\n\t</Trucktype>"
			<< "\n\t<Trucktype naam=\"truck_restafval\" max_uren=\"7.5\" vaste_kosten=\"100\" variabele_kosten=\"87\">"
			<< "\n\t\t<Capaciteit afvaltype=\"GFT\" cap=\"0\"/>"
			<< "\n\t\t<Capaciteit afvaltype=\"restafval\" cap=\"10.2\"/>"
			<< "\n\t</Trucktype>";

		// Collection Points
		file << "\n\t<Collectiepunt naam=\"IVM-restafval\">"
			<< "\n\t\t<ToegelatenAfval naam=\"restafval\"/>"
			<< "\n\t</Collectiepunt>"
			<< "\n\t<Collectiepunt naam=\"IVM-GFT\">"
			<< "\n\t\t<ToegelatenAfval naam=\"GFT\"/>"
			<< "\n\t</Collectiepunt>"
			<< "\n\t<Collectiepunt naam=\"Renewi\">"
			<< "\n\t\t<ToegelatenAfval naam=\"restafval\"/>"
			<< "\n\t\t<ToegelatenAfval naam=\"GFT\"/>"
			<< "\n\t</Collectiepunt>"
			<< "\n\t<Collectiepunt naam=\"CP-Deinze\">"
			<< "\n\t\t<ToegelatenAfval naam=\"restafval\"/>"
			<< "\n\t</Collectiepunt>";

		// Zones (customers)
		for (auto&& zone : _zones)
		{
			file << "\n\t<Zone naam=\"" << zone.naam << "\">"
				<< "\n\t\t<Afval afvaltype=\"GFT\" hoeveelheid=\"" << zone.gft << "\" collectietijd=\"2.2\"/>"
				 << "\n\t\t<Afval afvaltype=\"restafval\" hoeveelheid=\"" << zone.restafval << "\" collectietijd=\"2.2\"/>";

			if (zone.gft_week1 > 0)
				file << "\n\t\t<HuidigeKalender afvaltype=\"GFT\" dag=\"" << dag_naam.at(zone.gft_week1) << "\" week=\"1\"/>";
			if (zone.gft_week2 > 0)
				file << "\n\t\t<HuidigeKalender afvaltype=\"GFT\" dag=\"" << dag_naam.at(zone.gft_week2) << "\" week=\"2\"/>";
			if (zone.rest_week1 > 0)
				file << "\n\t\t<HuidigeKalender afvaltype=\"restafval\" dag=\"" << dag_naam.at(zone.rest_week1) << "\" week=\"1\"/>";
			if (zone.rest_week2 > 0)
				file << "\n\t\t<HuidigeKalender afvaltype=\"restafval\" dag=\"" << dag_naam.at(zone.rest_week2) << "\" week=\"2\"/>";
			if (zone.verboden_dag > 0)
				file << "\n\t\t<VerbodenDag dag=\"" << dag_naam.at(zone.verboden_dag) << "\"/>";


			file << "\n\t\t<Rijtijd naar=\"Depot" << "\" tijd=\"" << zone.t_depot << "\"/>";

			file << "\n\t\t<Rijtijd naar=\"IVM-restafval\" tijd=\"" << zone.t_cp1 << "\"/>";
			file << "\n\t\t<Rijtijd naar=\"IVM-GFT\" tijd=\"" << zone.t_cp2 << "\"/>";
			file << "\n\t\t<Rijtijd naar=\"Renewi\" tijd=\"" << zone.t_cp3 << "\"/>";
			file << "\n\t\t<Rijtijd naar=\"CP-Deinze\" tijd=\"" << zone.t_cp4 << "\"/>";

			file << "\n\t</Zone>";
		}

		file << "\n</Instantie>";
		file.flush();
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

		std::uniform_int_distribution<> dist_demand_rest(5, 25);
		std::uniform_int_distribution<> dist_demand_gft(2, 10);
		std::uniform_int_distribution<> dist_drive_time(1, 8); // echte waarde tussen 0.1 en 0.8 dus factor 10 kleiner
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
			file << "\n\t<Collectiepunt naam=\"CP" << i + 1 << "\">"
				<< "\n\t\t<ToegelatenAfval naam=\"restafval\"/>"
				<< "\n\t\t<ToegelatenAfval naam=\"GFT\"/>"
				<< "\n\t</Collectiepunt>";
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

	///////////////////////////////////////////////////////////////////////////////////////////////

}