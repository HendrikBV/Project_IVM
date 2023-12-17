/*
	Copyright (c) 2023 Hendrik Vermuyten

	Licensed under the Apache License, Version 2.0 (the "License"); 
	you may not use this file except in compliance with the License. 
	You may obtain a copy of the License at 
	
	http://www.apache.org/licenses/LICENSE-2.0
*/



/*!
 *  @file       Data.h
 *  @brief      Defines the data for the IVM scheduling problem
 */

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
	 *	@brief Class to store input/output all data
	 */
	class Instance
	{
		/*!
		 *	@brief To switch between indices and names for the days
		 */
		const std::unordered_map<std::string, int> _dag_naam_index{ {"maandag", 0}, {"dinsdag", 1}, {"woensdag", 2}, {"donderdag", 3}, {"vrijdag", 4} };

		/*!
		 *	@brief To switch between indices and names for the days
		 */
		const std::unordered_map<int, std::string> _naam_dag_index{ {0, "maandag"}, {1, "dinsdag"}, {2, "woensdag"}, {3, "donderdag"}, {4, "vrijdag"} };

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
		 *	@brief The names of the collection points
		 */
		std::vector<std::string> _collection_points;

		/*!
		 *	@brief	Stores the unloading times for the various types of waste (the same at all facilities)
		 *			string == type of waste; double == unloading time
		 */
		std::unordered_map<std::string, double> _waste_type_unloading_time;

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

		/*!
		 *	@brief	The solution from the allocation model that serves as
		 *			input to the routing model 
		 */
		std::vector<double> _sol_alloc_x_tmdw;

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

		/*!
		 *	@brief Get the number of waste types
		 *  @returns	The number of waste types
		 */
		size_t nb_waste_types() const { return _waste_types.size(); }

		/*!
		 *	@brief Get the number of truck types
		 *  @returns	The number of truck types
		 */
		size_t nb_truck_types() const { return _trucks.size(); }

		/*!
		 *	@brief Get the number of zones
		 *  @returns	The number of zones
		 */
		size_t nb_zones() const { return _zones.size(); }

		/*!
		 *	@brief Get the number of collection points
		 *  @returns	The number of collection points
		 */
		size_t nb_collection_points() const { return _collection_points.size(); }

		/*!
		 *	@brief Get the number of days per week
		 *  @returns	The number of days 
		 */
		size_t nb_days() const { return _nb_days; }

		/*!
		 *	@brief Get the number of weeks
		 *  @returns	The number of weeks
		 */
		size_t nb_weeks() const { return _nb_weeks; }

		/*!
		 *	@brief Get the maximum number of visits to every zone
		 *  @returns	The maximum number of visits
		 */
		size_t max_visits() const { return _max_visits; }

		/*!
		 *	@brief Get the name of a given day
		 *  @param	index	The index for the day
		 *  @returns	The name of the day
		 */
		const std::string& day_name(size_t index) const { return _naam_dag_index.at(index); }

		/*!
		 *	@brief Get the name of a waste type
		 *  @param	index	The index for the waste type
		 *  @returns	The name of the waste type
		 */
		const std::string& waste_type(size_t index) const { return _waste_types[index]; }

		/*!
		 *	@brief Get the name of a zone
		 *  @param	index	The index for the zone
		 *  @returns	The name of the zone
		 */
		const std::string& zone_name(size_t index) const { return _zones[index]._name; }

		/*!
		 *	@brief Get the name of a truck type
		 *  @param	index	The index for the truck type
		 *  @returns	The name of the truck type
		 */
		const std::string& truck_type(size_t index) const { return _trucks[index]._name; }

		/*!
		 *	@brief Get the name of a collection point
		 *  @param	index	The index for the collection point
		 *  @returns	The name of the collection point
		 */
		const std::string& collection_point(size_t index) const { return _collection_points[index]; }

		/*!
		 *	@brief Get the demand for a given waste type in a given zone
		 *  @param	zone	The index for the zone
		 *  @param	waste_type	The name of the waste type
		 *  @returns	The demand in the given zone for the given waste type
		 */
		double demand(int zone, const std::string& waste_type) const { return _zones[zone]._demands.at(waste_type); }

		/*!
		 *	@brief Check whether there is a pickup on a certain day of a given week for a given waste type in a given zone
		 *  @param	zone	The index for the zone
		 *  @param	waste_type	The name of the waste type
		 *  @param	day	The index for the day
		 *  @param	week	The index for the week
		 *  @returns	True if there is a pickup on that day, false if not
		 */
		bool current_calendar(size_t zone, const std::string& waste_type, size_t day, size_t week) const;

		/*!
		 *	@brief Get the number of pickups in the current calendar
		 *  @returns	The number of pickups
		 */
		size_t nb_pickups_current_calendar() const;

		/*!
		 *	@brief Get the operating costs per hour for a truck of a given type
		 *  @param	truck_type	The index for the truck type
		 *  @returns	The operating costs for that truck type
		 */
		double operating_costs(size_t truck_type) const { return _trucks[truck_type]._operating_costs; }

		/*!
		 *	@brief Get the name of a waste type
		 *  @param	index	The index for the waste type
		 *  @returns	The name of the waste type
		 */
		double fixed_costs(size_t truck_type) const { return _trucks[truck_type]._fixed_costs; }

		/*!
		 *	@brief Get the driving time from a given zone to a given collection point
		 *  @param	index	The index for the zone
		 *  @param	collection_point	The name of the collection point
		 *  @returns	The driving time
		 */
		double time_driving_zone_collectionpoint(size_t zone, const std::string& collection_point) const { return _zones[zone]._driving_time.at(collection_point); }

		/*!
		 *	@brief Get the driving time from a given zone to the depot
		 *  @param	index	The index for the zone
		 *  @returns	The driving time
		 */
		double time_driving_zone_depot(size_t zone) const { return _zones[zone]._driving_time.at("Depot"); }

		/*!
		 *	@brief Get the pickup time per unit of waste for a given type of waste at a given zone
		 *  @param	index	The index for the zone
		 *  @param	waste_type	The name of the waste time
		 *  @returns	The pickup time
		 */
		double time_pickup(size_t zone, const std::string& waste_type) const { return _zones[zone]._collection_times.at(waste_type); }

		/*!
		 *	@brief Get the (fixed) unloading time for a given type of waste 
		 *  @param	waste_type	The name of the type of waste
		 *  @returns	The unloading time
		 */
		double time_unloading(const std::string& waste_type) const { return _waste_type_unloading_time.at(waste_type); }

		/*!
		 *	@brief Get the maximum driving time for a given type of truck
		 *  @param	index	The index for the type of truck
		 *  @returns	The the maximum driving time
		 */
		double max_driving_time(size_t truck_type) const { return _trucks[truck_type]._max_hours; }

		/*!
		 *	@brief Get the capacity of a given type of truck for a given type of waste
		 *  @param	index	The index for the type of truck
		 *  @param	waste_type	The name of the type of waste
		 *  @returns	The capacity
		 */
		double capacity(size_t truck_type, const std::string& waste_type) const { return _trucks[truck_type]._capacities.at(waste_type); }

		/*!
		 *	@brief Get the solution from the allocation model
		 *  @param	waste_type	Index of the waste type
		 *  @param	zone	Index of the zone
		 *  @param	day	Index of the day
		 *  @param	week	Index of the week
		 *  @returns	The amount of waste of the given type to pick up on the given day and week in the given zone
		 */
		double x_tmdw(size_t waste_type, size_t zone, size_t day, size_t week) const;

		/*!
		 *	@brief	Set the solution from the allocation model
		 *  @param	x	The values of the x_tmdw variables
		 */
		void set_solution_x(const std::vector<double>& x) { _sol_alloc_x_tmdw = x; }
	};

	///////////////////////////////////////////////////////////////////////////

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

#endif // !DATA_H
