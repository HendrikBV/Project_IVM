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

		while (file >> naam >> perceel >> gft >> restafval >> gft_week1 >> gft_week2 >> rest_week1 >> rest_week2 >> verboden_dag >> t_depot >> t_cp1 >> t_cp2 >> t_cp3)
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

		// General information (vaste data al geimplementeerd)
		file << "<?xml version=\"1.0\"?>\n<Instantie naam=\"X\" aantal_dagen=\"5\" aantal_weken=\"2\" max_bezoeken=\"1\">"
			<< "\n\t<Afvaltype naam=\"GFT\" lostijd=\"0.17\"/>"
			<< "\n\t<Afvaltype naam = \"restafval\" lostijd=\"0.17\"/>"
			<< "\n\t<Trucktype naam=\"truck_GFT\" max_uren=\"7.5\" vaste_kosten=\"100\" variabele_kosten=\"87\">"
			<< "\n\t\t<Capaciteit afvaltype=\"GFT\" cap=\"10.2\"/>"
			<< "\n\t\t<Capaciteit afvaltype=\"restafval\" cap=\"10.2\"/>"
			<< "\n\t</Trucktype>";

		// Collection Points
		file << "\n\t<Collectiepunt naam=\"CP Eeklo\"/>"
			<< "\n\t<Collectiepunt naam=\"CP Evergem\"/>"
			<< "\n\t<Collectiepunt naam=\"CP Deinze\"/>";

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

			file << "\n\t\t<Rijtijd naar=\"CP Eeklo\" tijd=\"" << zone.t_cp1 << "\"/>";
			file << "\n\t\t<Rijtijd naar=\"CP Evergem\" tijd=\"" << zone.t_cp2 << "\"/>";
			file << "\n\t\t<Rijtijd naar=\"CP Deinze\" tijd=\"" << zone.t_cp3 << "\"/>";

			file << "\n\t</Zone>";
		}

		file << "\n</Instantie>";
		file.flush();
	}

	///////////////////////////////////////////////////////////////////////////////////////////////

	
}