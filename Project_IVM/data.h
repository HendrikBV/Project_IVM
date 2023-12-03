#pragma once
#ifndef DATA_H
#define DATA_H

#include <string>
#include <vector>
#include <unordered_map>
#include <exception>

namespace IVM
{
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

	///////////////////////////////////////////////////////////////////////////

	/*!
	 *	@brief Class to store input/output all data
	 */
	class Instance
	{
		/*!
		 *	@brief To switch between indices and names for the days
		 */
		const std::unordered_map<std::string, int> _dag_naam_index{ {"maandag", 0}, {"dinsdag", 1}, {"woensdag", 2}, {"donderdag", 3}, {"vrijdag", 4} };

		/*!
		 *	@brief The name of the instance
		 */
		std::string _name;

		/*!
		 *	@brief The number of days per week in the instance
		 */
		size_t _nb_days;

		/*!
		 *	@brief The number of weeks in the instance
		 */
		size_t _nb_weeks;

		/*!
		 *	@brief The maximum number of visits to every zone in the planning horizon
		 */
		size_t _max_visits;

		/*!
		 *	@brief The names of all waste types. Used to access by name instead of index
		 */
		std::vector<std::string> _waste_types;

		/*!
		 *	@brief Maps the name of a collection point and the unloading time
		 */
		std::unordered_map<std::string, double> _collection_points_unloading_times;

		/*!
		 *	@brief To store information on the different types of trucks
		 */
		struct Truck
		{
			/*!
			 *	@brief Name of the truck
			 */
			std::string _name;

			/*!
			 *	@brief Maximum working hours per day
			 */
			double _max_hours;

			/*!
			 *	@brief Fixed costs for one truck
			 */
			double _fixed_costs;

			/*!
			 *	@brief Operating costs per hour
			 */
			double _operating_costs;

			/*!
			 *	@brief	The capacity of the truck for the various types of waste
			 *			string == waste type; double == capacity for that type
			 */
			std::unordered_map<std::string, double> _capacities;
		};

		/*!
		 *	@brief The types of trucks that can be used
		 */
		std::vector<Truck> _trucks;

		/*!
		 *	@brief To store information on the zones (customers)
		 */
		struct Zone
		{
			/*!
			 *	@brief Name of the zone (customer)
			 */
			std::string _name;

			/*!
			 *	@brief	Demand for the various types of waste
			 *			string == waste type; double == demand for that type
			 */
			std::unordered_map<std::string, double> _demands;

			/*!
			 *	@brief	Collection time for the various types of waste
			 *			string == waste type; double == collection time for that type
			 */
			std::unordered_map<std::string, double> _collection_times;

			/*!
			 *	@brief	Current pickup day for the various types of waste
			 *			string == waste type; double == current pickup day for that type
			 */
			std::unordered_map<std::string, int> _current_calendar_day; ///< Only one day possible currently!

			/*!
			 *	@brief	Current pickup week for the various types of waste
			 *			string == waste type; double == current pickup week for that type
			 */
			std::unordered_map<std::string, int> _current_calendar_week; ///< Only one week possible currently!

			/*!
			 *	@brief	Driving time from this zone to various destinations
			 *			string == destination; double == driving time
			 */
			std::unordered_map<std::string, double> _driving_time;

			/*!
			 *	@brief	Days on which a pickup is not allowed
			 */
			std::vector<int> _forbidden_days;
		};

		/*!
		 *	@brief To store information on the zones (customers)
		 */
		std::vector<Zone> _zones;

	public:
		/*!
		 *	@brief Obtain data from an XML file
		 *  @param	filename	The name of the XML file
		 */
		void read_data_xml(const std::string& filename);

		/*!
		 *	@brief Clear all data
		 */
		void clear_data();

		size_t nb_waste_types() const { return _waste_types.size(); }
		size_t nb_zones() const { return _zones.size(); }
		size_t nb_days() const { return _nb_days; }
		size_t nb_weeks() const { return _nb_weeks; }
		size_t max_visits() const { return _max_visits; }

		const std::string& waste_type(size_t index) const { return _waste_types[index]; }
		double demand(int zone, const std::string& waste_type) const { return _zones[zone]._demands.at(waste_type); }
		bool current_calendar(size_t zone, const std::string& waste_type, size_t day, size_t week) const;
		size_t nb_pickups_current_calendar() const;

	};


}

#endif // !DATA_H
