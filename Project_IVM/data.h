#pragma once
#ifndef DATA_H
#define DATA_H

#include <vector>
#include <string>
#include <exception>

namespace IVM
{
	/*!
	 *	@brief Class to store input/output all data
	 */
	class Data
	{
		struct Customer
		{
			/*!
			 *	@brief Customer name
			 */
			std::string _name;

			/*!
			 *	@brief Travel time between vehicle depot and customer (h)
			 */
			double _t_dep;

			/*!
			 *	@brief Travel time between disposal facility and customer (h)
			 */
			double _t_disp;

			/*!
			 *	@brief Travel time between disposal facility and vehicle depot (h)
			 */
			double _t_disp_dep;

			/*!
			 *	@brief Time of unloading at disposal facility, independent of the amounts collected (h)
			 */
			double _t_unl;

			/*!
			 *	@brief Time for a type 1 trip to the customer (exclusive of the collection time at the customer) t_1 = t_dep + t_disp + t_disp_dep + t_unl
			 */
			double _t_trip1;

			/*!
			 *	@brief Time for a type 2 trip to the customer (exclusive of the collection time at the customer) t_2 = 2 * t_disp + t_unl
			 */
			double _t_trip2;

			/*!
			 *	@brief Demand to be collected at the customer (> 0)
			 */
			double _Q;

			/*!
			 *	@brief Collection speed at the customer (h/tonne)
			 */
			double _s;
		};

		/*!
		 *	@brief All the customers in the dataset
		 */
		std::vector<Customer> _customers;

		/*!
		 *	@brief Travel/unloading cost per hour (euro/h)
		 */
		double _c_h;

		/*!
		 *	@brief Depreciation cost of a vehicle (euro)
		 */
		double _c_veh;

		/*!
		 *	@brief Maximal number of hours available per day (h)
		 */
		double _T;

		/*!
		 *	@brief Maximal load of a vehicle (tonne)
		 */
		double _L;

		/*!
		 *	@brief Number of days in the planning horizon
		 */
		size_t _D;

		/*!
		 *	@brief Number of available vehicles in the instance
		 */
		size_t _max_vehicles;

		/*!
		 *	@brief Max number of visits to customers
		 */
		size_t _Wm = 3;

		/*!
		 *	@brief Name of the instance
		 */
		std::string _name_instance;

	public:

		/*!
		 *	@brief Get the travel time between vehicle depot and customer for customer m
		 *  @param	filename	Filename of the txt-file containing the data
		 *  @returns	True if successful, false otherwise
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
		 *	@brief Get the number of customers
		 *  @returns	The number of customers
		 */
		size_t nb_customers() const { return _customers.size(); }

		/*!
		 *	@brief Get the travel time between vehicle depot and customer for customer m
		 *  @param	customer	The index of the customer
		 *  @returns	The name of the customer
		 */
		const std::string& name(int customer) const { return _customers[customer]._name; }

		/*!
		 *	@brief Get the travel time between vehicle depot and customer for customer m
		 *  @param	customer	The index of the customer
		 *  @returns	The travel time between vehicle depot and customer for customer m
		 */
		double t_dep(int customer) const { return _customers[customer]._t_dep; }

		/*!
		 *	@brief Get the travel time between disposal facility and customer for customer m
		 *  @param	customer	The index of the customer
		 *  @returns	The travel time between disposal facility and customer for customer m
		 */
		double t_disp(int customer) const { return _customers[customer]._t_disp; }

		/*!
		 *	@brief Get the travel time between disposal facility and vehicle depot for customer m
		 *  @param	customer	The index of the customer
		 *  @returns	The travel time between disposal facility and vehicle depot for customer m
		 */
		double t_disp_dep(int customer) const { return _customers[customer]._t_disp_dep; }

		/*!
		 *	@brief Get the time of unloading at disposal facility for customer m
		 *  @param	customer	The index of the customer
		 *  @returns	The time of unloading at disposal facility for customer m
		 */
		double t_unl(int customer) const { return _customers[customer]._t_unl; }

		/*!
		 *	@brief Get the time for a type 1 trip to the customer for customer m
		 *  @param	customer	The index of the customer
		 *  @returns	The time for a type 1 trip to the customer for customer m
		 */
		double t_trip1(int customer) const { return _customers[customer]._t_trip1; }

		/*!
		 *	@brief Get the time for a type 2 trip to the customer for customer m
		 *  @param	customer	The index of the customer
		 *  @returns	The time for a type 2 trip to the customer for customer m
		 */
		double t_trip2(int customer) const { return _customers[customer]._t_trip2; }

		/*!
		 *	@brief Get the demand to be collected at customer m
		 *  @param	customer	The index of the customer
		 *  @returns	The demand to be collected at customer m
		 */
		double demand(int customer) const { return _customers[customer]._Q; }

		/*!
		 *	@brief Get the collection speed at customer m
		 *  @param	customer	The index of the customer
		 *  @returns	The collection speed at customer m
		 */
		double collection_speed(int customer) const { return _customers[customer]._s; }

		/*!
		 *	@brief Get the travel/unloading cost per hour 
		 *  @returns	The travel/unloading cost per hour 
		 */
		double cost_hour() const { return _c_h; }

		/*!
		 *	@brief Get the cost of a vehicle
		 *  @returns	The cost of a vehicle
		 */
		double cost_vehicle() const { return _c_veh; }

		/*!
		 *	@brief Get the maximal number of hours available per day
		 *  @returns	The maximal number of hours available per day
		 */
		double max_hours() const { return _T; }

		/*!
		 *	@brief Get the maximal load of a vehicle
		 *  @returns	The maximal load of a vehicle
		 */
		double max_load() const { return _L; }

		/*!
		 *	@brief Get the number of days in the planning horizon
		 *  @returns	The number of days in the planning horizon
		 */
		size_t days() const { return _D; }

		/*!
		 *	@brief Get the number of available vehicles in the instance
		 *  @returns	The number of available vehicles in the instance
		 */
		size_t vehicles() const { return _max_vehicles; }

		/*!
		 *	@brief Get the max number of visits to customers
		 *  @returns	The max number of visits to customers
		 */
		size_t max_visits() const { return _Wm; }

	};
}


#endif // !DATA_H
