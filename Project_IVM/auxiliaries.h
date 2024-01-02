/*
	Copyright (c) 2023 Hendrik Vermuyten

	Licensed under the Apache License, Version 2.0 (the "License");
	you may not use this file except in compliance with the License.
	You may obtain a copy of the License at

	http://www.apache.org/licenses/LICENSE-2.0
*/



/*!
 *  @file       Auxiliaries.h
 *  @brief      Defines auxiliary functions to transform inputs and outputs to other formats
 */

#pragma once
#ifndef AUXILIARIES_H
#define AUXILIARIES_H

#include <string>
#include <vector>
#include <unordered_map>

namespace aux
{
	/*!
	 *	@brief Class to transform input data from txt file to xml file
	 */
	class TransformerXML
	{
		/*! 
		 *	@brief Transfrom day index to day name
		 */
		const std::unordered_map<int, std::string> dag_naam{ {1,"maandag"},{2,"dinsdag"},{3,"woensdag"},{4,"donderdag"},{5,"vrijdag"} };

		/*!
		 *	@brief Stores information on the zones
		 */
		struct Zone
		{
			std::string naam;
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
		};

		/*!
		 *	@brief All zones
		 */
		std::vector<Zone> _zones;

		/*!
		 *	@brief Read data from txt file
		 *  @param inputfile	Name of the inputfile (txt)
		 */
		void read_data(const std::string& inputfile);

		/*!
		 *	@brief Generate xml file
		 *  @param outputfile	Name of the outputfile (xml)
		 */
		void generate_xml(const std::string& outputfile);

	public:
		/*!
		 *	@brief Transfrom the data from inputfile to outputfile
		 *  @param inputfile	Name of the inputfile (txt)
		 *  @param outputfile	Name of the outputfile (xml)
		 */
		void transform(const std::string& inputfile, const std::string& outputfile);
	};
}

#endif // !AUXILIARIES_H

