/*
	Copyright (c) 2024 KU Leuven
	Code author: Hendrik Vermuyten
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
			std::string naam;	///< Name of the zone
			double gft;			///< Amount of GFT to be picked up
			double restafval;	///< Amount of restafval to be picked up
			int gft_week1;		///< Day in week 1 on which GFT is currently picked up
			int gft_week2;		///< Day in week 2 on which GFT is currently picked up
			int rest_week1;		///< Day in week 1 on which restafval is currently picked up
			int rest_week2;		///< Day in week 2 on which restafval is currently picked up
			int verboden_dag;	///< Day on which no pickups are allowed
			double t_depot;		///< Driving time from this zone to the depot
			double t_cp1;		///< Driving time from this zone collection point 1 (IVM-restafval)
			double t_cp2;		///< Driving time from this zone collection point 2 (IVM-GFT)
			double t_cp3;		///< Driving time from this zone collection point 3	(Renewi)
			double t_cp4;		///< Driving time from this zone collection point 4	(Deinze)
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

	///////////////////////////////////////////////////////////////////////////////////////////////

	/*!
	 *	@brief Class to generate test instances
	 */
	class Instance_Generator
	{
		/*!
		 *	@brief The number of zones in the instance
		 */
		size_t _nb_zones = 41;

		/*!
		 *	@brief The number of collection points
		 */
		size_t _nb_collection_points = 3;

		/*!
		 *	@brief The number of days per week in the instance
		 */
		size_t _nb_days = 5;

		/*!
		 *	@brief The number of weeks in the instance
		 */
		size_t _nb_weeks = 2;

	public:
		/*!
		 *	@brief Change the size of the instance to be generated
		 *  @param nb_zones	The number of zones in the instance
		 *  @param nb_collection_points	The number of collection points in the instance
		 *  @param nb_days	The number of days in the instance
		 *  @param nb_weeks	The number of weeks in the instance
		 */
		void change_parameters(size_t nb_zones, size_t nb_collection_points, size_t nb_days, size_t nb_weeks);

		/*!
		 *	@brief Generate an instance and write it to an xml-file
		 */
		void generate_xml();
	};
}

#endif // !AUXILIARIES_H

