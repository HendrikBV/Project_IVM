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
		 *	@brief The number of depots in the instance
		 */
		size_t _nb_depots = 1; ///< Kan niet veranderen?

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
		const std::unordered_map<std::string, int> _dag_naam_index{ {"maandag", 0}, {"dinsdag", 1}, {"woensdag", 2}, {"donderdag", 3}, {"vrijdag", 4} };

		std::string _name;
		size_t _nb_days;
		size_t _nb_weeks;
		size_t _max_visits;
		std::vector<std::string> _waste_types;
		std::unordered_map<std::string, double> _collection_points_unloading_times;

		struct Truck
		{
			std::string _name;
			double _max_hours;
			double _fixed_costs;
			double _operating_costs;
			std::unordered_map<std::string, double> _capacities;
		};
		std::vector<Truck> _trucks;

		struct Zone
		{
			std::string _name;
			std::unordered_map<std::string, double> _demands;
			std::unordered_map<std::string, double> _collection_times;
			std::unordered_map<std::string, int> _current_calendar_day;
			std::unordered_map<std::string, int> _current_calendar_week;
			std::unordered_map<std::string, double> _driving_time;
			std::vector<int> _forbidden_days;
		};
		std::vector<Zone> _zones;

	public:
		void read_data(const std::string& filename);
		void clear_data() {}
		void print_data() {}

		// access functions
	};








#ifdef BIJVOEGEN		
	/*!
	 *	@brief Class to store input/output all data
	 */
	class Data
	{
		/*!
		 *	@brief Struct to store data for every customer (== zone)
		 */
		struct Customer
		{
			/*!
			 *	@brief Customer name
			 */
			std::string _name;

			/*!
			 *	@brief	Demand of the various types of waste to be collected at the customer.
			 *			index == type of waste
			 */
			std::vector<double> _demands;

			/*!
			 *	@brief	Collection time per type of waste (hours per kg)
			 *			index == type of waste
			 */
			std::vector<double> _collection_times;

			/*!
			 *	@brief	The current calendar (true == pickup happens, false no pickup happens)
			 *			First index == week, second index == type of waste, third index == day
			 */
			std::vector<bool> _current_calendar;

			/*!
			 *	@brief	Days on which no waste can be picked up at this customer (Monday--Friday)
			 */
			std::vector<bool> _forbidden_days;

			/*!
			 *	@brief	Travel time from this customer to the collection points
			 */
			std::vector<double> _travel_times_collection_points;

			/*!
			 *	@brief	Travel time from this customer to the truck depots for the areas
			 */
			std::vector<double> _travel_times_areas;

			/*!
			 *	@brief	Travel times to other customers (by index)
			 */
			std::vector<double> _travel_times_customers;

			/*!
			 *	@brief Current area (perceel)
			 */
			size_t _current_area;
		};

		/*!
		 *	@brief All the customers in the dataset
		 */
		std::vector<Customer> _customers;

		/*!
		 *	@brief Number of days per week in the planning horizon
		 */
		size_t _nb_days = 5;

		/*!
		 *	@brief Number of weeks in the planning horizon
		 */
		size_t _nb_weeks = 2;

		/*!
		 *	@brief Max number of visits to customers
		 */
		size_t _max_visits_customers = 1;

		/*!
		 *	@brief The types of waste
		 */
		std::vector<std::string> _waste_types;

		/*!
		 *	@brief The types of trucks
		 */
		std::vector<std::string> _truck_types;

		/*!
		 *	@brief Collection points
		 */
		std::vector<std::string> _collection_points;

		/*!
		 *	@brief Current areas (percelen)
		 */
		std::vector<std::string> _current_areas;

		/*!
		 *	@brief The unloading time at the various collection points
		 */
		std::vector<double> _times_unloading_collection_points;

		/*!
		 *	@brief	The capacity of the different types of trucks
		 *			First index == type of truck, second index == type of waste
		 */
		std::vector<double> _truck_capacities;

		/*!
		 *	@brief	Maximum number of working hours per day
		 *			Index == type of truck
		 */
		std::vector<double> _truck_maxworkinghours;

		/*!
		 *	@brief	Fixed costs per truck
		 *			Index == type of truck
		 */
		std::vector<double> _truck_fixedcosts;

		/*!
		 *	@brief	Operating costs per truck per hour
		 *			Index == type of truck
		 */
		std::vector<double> _truck_operatingcosts;

		/*!
		 *	@brief Name of the instance
		 */
		std::string _name_instance;

	public:
		/*!
		 *	@brief Get the travel time between vehicle depot and customer for customer m
		 *  @param	filename	Filename of the txt-file containing the data
		 *	@returns	True if succesful, false otherwise
		 */
		bool read_data(const std::string& filename);

		/*!
		 *	@brief Clear all data
		 */
		void clear_data();

		/*!
		 *	@brief Print data to screen
		 */
		void print_data();

		/*!
		 *	@brief Get the number of the instance
		 *  @returns	The number of instance
		 */
		const std::string& instance_name() const { return _name_instance; }

		/*!
		 *	@brief Get the number of days per week in the planning horizon
		 *  @returns	The number of days per week in the planning horizon
		 */
		size_t days() const { return _nb_days; }

		/*!
		 *	@brief Get the number of weeks in the planning horizon
		 *  @returns	The number of weeks in the planning horizon
		 */
		size_t weeks() const { return _nb_weeks; }

		/*!
		 *	@brief Get the max number of visits to customers
		 *  @returns	The max number of visits to customers
		 */
		size_t max_visits() const { return _max_visits_customers; }

		/*!
		 *	@brief Get the number of waste types
		 *  @returns	The number of waste types
		 */
		size_t waste_types() const { return _waste_types.size(); }

		/*!
		 *	@brief Get the number of pickups in the current calendar
		 *  @returns	The number of pickups in the current calendar
		 */
		size_t nb_pickups_current_calendar() const;

		/*!
		 *	@brief Get the name of a given type of waste
		 *  @param	waste_type	The index for the waste type
		 *  @returns	The name for the given type of waste
		 */
		const std::string& waste_type(size_t waste_type) const { return _waste_types[waste_type]; }

		/*!
		 *	@brief Get the name of a given type of truck
		 *  @param	truck_type	The index for the truck type
		 *  @returns	The name for the given type of truck
		 */
		const std::string& truck_type(size_t truck_type) const { return _truck_types[truck_type]; }

		/*!
		 *	@brief Get the name of a given collection point
		 *  @param	collection_point	The index for the collection point
		 *  @returns	The name for the given collection point
		 */
		const std::string& collection_point(size_t collection_point) const { return _collection_points[collection_point]; }

		/*!
		 *	@brief Get the name of a given area (perceel)
		 *  @param	area	The index for the area
		 *  @returns	The name for the given area
		 */
		const std::string& name_current_area(size_t area) const { return _current_areas[area]; }

		/*!
		 *	@brief Get the unloading time at a given collection point
		 *  @param	collection_point	The index for the collection point
		 *  @returns	The unloading time at the collection point
		 */
		double unloading_time(size_t collection_point) const { return _times_unloading_collection_points[collection_point]; }

		/*!
		 *	@brief Get the capacity of a given type of truck for a given type of waste
		 *  @param	truck_type		The index for the type of truck
		 *  @param	waste_type		The index for the type of waste
		 *  @returns	The capacity of a truck of type truck_type for waste of type waste_type
		 */
		double capacity(size_t truck_type, size_t waste_type) const { return _truck_capacities[truck_type * _waste_types.size() + waste_type]; }

		/*!
		 *	@brief Get the maximum working time per day (hours) for a truck of a given type
		 *  @param	truck_type	The index for the type of truck
		 *  @returns	The maximum working time for a truck of type truck_type
		 */
		double max_working_time(size_t truck_type) const { return _truck_maxworkinghours[truck_type]; }

		/*!
		 *	@brief Get the fixed costs for a truck of a given type
		 *  @param	truck_type	The index for the type of truck
		 *  @returns	The fixed costs for a truck of type truck_type
		 */
		double fixed_costs(size_t truck_type) const { return _truck_fixedcosts[truck_type]; }

		/*!
		 *	@brief Get the operating costs for a truck of a given type
		 *  @param	truck_type	The index for the type of truck
		 *  @returns	The operating costs for a truck of type truck_type
		 */
		double operating_costs(size_t truck_type) const { return _truck_operatingcosts[truck_type]; }


		/*!
		 *	@brief Get the number of customers
		 *  @returns	The number of customers
		 */
		size_t nb_customers() const { return _customers.size(); }

		/*!
		 *	@brief Get the travel time between vehicle depot and customer for customer m
		 *  @param	customer	The index of the customer
		 *  @returns	The name of the customer
		 */
		const std::string& name(size_t customer) const { return _customers[customer]._name; }

		/*!
		 *	@brief Get the current area for the customer
		 *  @param	customer	The index for the customer
		 *  @returns	The current area for the customer
		 */
		size_t current_area(size_t customer) const { return _customers[customer]._current_area; }

		/*!
		 *	@brief Get the demand to be collected at customer m for waste type t
		 *  @param	customer	The index of the customer
		 *  @param	waste_type	The type of waste to be collected
		 *  @returns	The demand of waste_type to be collected at customer m
		 */
		double demand(size_t customer, size_t waste_type) const { return _customers[customer]._demands[waste_type]; }

		/*!
		 *	@brief Get the collection time for a type of waste at a given customer
		 *  @param	customer	The index of the customer
		 *  @param	waste_type	The type of waste to be collected
		 *  @returns	The collection time of waste_type to be collected at customer m
		 */
		double collection_time(size_t customer, size_t waste_type) const { return _customers[customer]._collection_times[waste_type]; }

		/*!
		 *	@brief Check whether the current calendar for a given customer collects a certain type of waste in a given week on a given day
		 *  @param	customer	The index of the customer
		 *  @param	waste_type	The type of waste to be collected
		 *  @param	week	The index of the week
		 *  @param	day		The index of the day
		 *  @returns	True if there is a pickup on that day in the current calendar, false otherwise
		 */
		bool current_calendar(size_t customer, size_t waste_type, size_t week, size_t day) const
		{
			// First index == week, second index == type of waste, third index == day
			return _customers[customer]._current_calendar[week * _waste_types.size() * _nb_days + waste_type * _nb_days + day];
		}

		/*!
		 *	@brief Get the demand to be collected at customer m for waste type t
		 *  @param	customer	The index of the customer
		 *  @param	day		The index of the day
		 *  @returns	True if it is forbidden to collect waste on that day, false otherwise
		 */
		bool forbidden_day(size_t customer, size_t day) const { return _customers[customer]._forbidden_days[day]; }

		/*!
		 *	@brief Get the travel time from a customer to a collection point
		 *  @param	customer	The index of the customer
		 *  @param	collection_point	The index of the collection point
		 *  @returns	The travel time from the customer to the collection point
		 */
		double time_customer_collection_point(size_t customer, size_t collection_point) const { return _customers[customer]._travel_times_collection_points[collection_point]; }

		/*!
		 *	@brief Get the travel time from a customer to an area
		 *  @param	customer	The index of the customer
		 *  @param	area	The index of the area
		 *  @returns	The travel time from the customer to the area
		 */
		double time_customer_areas(size_t customer, size_t area) const { return _customers[customer]._travel_times_areas[area]; }

		/*!
		 *	@brief Get the travel time from a customer to an area
		 *  @param	customer1	The index of the first customer
		 *  @param	customer2	The index of the second customer
		 *  @returns	The travel time from the customer to the other customer
		 */
		double time_customer_customer(size_t customer1, size_t customer2) const { return _customers[customer1]._travel_times_customers[customer2]; }
	};
#endif // !INCLUDE

}

#endif // !DATA_H
