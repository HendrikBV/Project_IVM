/*
	Copyright (c) 2024 KU Leuven
	Code author: Hendrik Vermuyten
*/



/*!
 *  @file       Models.h
 *  @brief      Defines the IP models for the IVM scheduling problem
 *
 *  The IP_model_allocation class builds and solves an IP model to
 *  assign pickup days to zones. The objective function tries to
 *  level the amounts of waste picked up over the days in the planning
 *  horizon. The constraints take into account the maximum number of
 *  visits per week per zone, days on which no pickups are allowed,
 *  and the structure of the calendar (e.g. same day for different
 *  types of waste). This model is solved _before_ the routing model
 *  is solved.
 *
 *  The IP_model_routing class builds and solves an IP model to
 *  find the optimal routes to organize the pickups in all zones
 *  for a given day. A calendar needs to be specified through the
 *  Instance class so that the model knows the amounts of the
 *  different types of waste that need to be picked up in every zone.
 *
 *  The IP_model_allocation_post class builds and solves an IP model
 *  that takes a set of proposed routes and triest to assign these
 *  routes to the different days in the planning horizon to build a
 *  calendar that (i) tries to minimize the maximum number of routes
 *  assigned to a single day, (ii) tries to minimize the number of
 *  pickup days for every zone, and (iii) tries to take the preferred
 *  pickup days into account. This model is solved _after_ the routing
 *  model is solved.
 */

#pragma once
#ifndef MODELS_H
#define MODELS_H

#include "ilcplex/cplex.h"
#include <string>
#include <memory>
#include <chrono>
#include <vector>
#include <unordered_map>



namespace IVM
{
	// forward declaration
	class Instance;


	/*!
	 *	@brief The model to decide which zones are visited on which days to level the collection amounts
	 */
	class IP_model_allocation
	{
		/*!
		 *	@brief CPLEX environment pointer
		 */
		CPXENVptr env = nullptr;

		/*!
		 *	@brief CPLEX LP pointer
		 */
		CPXLPptr problem = nullptr;

		/*!
		 *	@brief Initialize the CPLEX environment & problem
		 */
		void initialize_cplex();

		/*!
		 *	@brief Build the CPLEX model
		 *  @param	data	The problem data
		 */
		void build_problem(const Instance& data);

		/*!
		 *	@brief Solve the CPLEX model
		 *  @param	data	The problem data
		 */
		void solve_problem(const Instance& data);

		/*!
		 *	@brief Release CPLEX memory
		 */
		void clear_cplex();

		/*!
		 *	@brief	The fraction of deviations that is allowed compared to the current calendar
		 *			Value should be between 0 and 1
		 */
		double _fraction_allowed_deviations = 0.1;

		/*!
		 *	@brief The scenario (which constraints are in the model)
		 */
		int _scenario = 0;

		/*!
		 *	@brief Get a string that describes the current scenario
		 *	@returns	A string describing the current scenario
		 */
		const std::string scenario_name() const;

		/*!
		 *	@brief The maximum computation time (in seconds)
		 */
		double _max_computation_time = 60;

		/*!
		 *	@brief The objective value of the solution
		 */
		double _objective_value = -1;

		/*!
		 *        @brief Print the solver's output to screen
		 */
		bool _output_solver = false;


	public:
		/*!
		 *	@brief Set the fraction of deviations that is allowed compared to the current calendar
		 *  @param	fraction	The fraction of deviations that is allowed (between 0 and 1)
		 */
		void set_fraction_allowed_deviations(double fraction) { _fraction_allowed_deviations = fraction; }

		/*!
         *	@brief Set the output to screen for the solver on/off.
         *  @param	on	If true, output is turned on; otherwise output is turned off
         */
		void set_solver_output_on(bool on) { _output_solver = on; }

		/*!
		 *	@brief The possible scenarios
		 */
		enum Scenario
		{
			FIXED_WEEK_SAME_DAY,	///< Restafval in ene week, GFT in andere week, zelfde dag
			FIXED_WEEK_FREE_DAY,	///< Restafval in ene week, GFT in andere week, niet noodzakelijk zelfde dag
			FREE_WEEK_FREE_DAY,		///< Vrije keuze van week of dag
			CURRENT_CALENDAR		///< Huidige kalender (niet echt optimalisatie)
		};

		/*!
		 *	@brief Set the scenario
		 *  @param scenario	The scenario
		 */
		void set_scenario(int scenario) { _scenario = scenario; }

		/*!
		 *	@brief Set the maximum computation time
		 *  @param max_computation_time	The maximum computation time
		 */
		void set_max_computation_time(double max_computation_time) { _max_computation_time = max_computation_time; }

		/*!
		 *	@brief Get the objective value of the solution
		 *  @returns The objective value
		 */
		double objective_value() const { return _objective_value; }

		/*!
		 *	@brief Build and solve the MIP model
		 *  @param	data	The problem data
		 */
		void run(const Instance& data);
	};

	///////////////////////////////////////////////////////////////////////////////////////////////

	/*!
	 *	@brief The model to determine the pickup routes
	 */
	class IP_model_routing
	{
		/*!
		 *	@brief CPLEX environment pointer
		 */
		CPXENVptr env = nullptr;

		/*!
		 *	@brief CPLEX LP pointer
		 */
		CPXLPptr problem = nullptr;

		/*!
		 *	@brief Initialize the CPLEX environment & problem
		 */
		void initialize_cplex();

		/*!
		 *	@brief Build the CPLEX model
		 *  @param	data	The problem data
		 *  @param	day		The day for which to build the routing problem
		 */
		void build_problem(const Instance& data, size_t day);

		/*!
		 *	@brief Solve the CPLEX model
		 *  @param	data	The problem data
		 *  @param	day		The day for which to solve the routing problem
		 */
		void solve_problem(const Instance& data, size_t day);

		/*!
		 *	@brief Release CPLEX memory
		 */
		void clear_cplex();

		/*!
		 *	@brief The available number of trucks
		 */
		size_t _max_nb_trucks = 30;

		/*!
		 *	@brief	The maximum number of segments in a route
		 *			Should be minimum 3
		 *			1 zone == 3 segments, 2 zones == 5 segments, 3 zones == 7 segments and so on
		 */
		size_t _max_nb_segments = 9;

		/*!
		 *	@brief Indicates whether we minimize the number of trucks in the objective function or not
		 */
		bool _include_nb_truck_objective = true;

		/*!
		 *	@brief The maximum computation time (in seconds)
		 */
		double _max_computation_time = 600;

		/*!
		 *	@brief	When the value |bestbound-bestinteger|/(1e-10+|bestinteger|) falls below the value of this parameter, the mixed integer optimization is stopped.
		 *			Between 0.0 and 1.0
		 *			Standard CPLEX value = 0.0001
		 */
		double _optimality_tolerance = 0.0001;

		/*!
		 *	@brief The objective value of the optimal solution
		 */
		double _objective_value = -1;

		/*!
		 *        @brief Print the solver's output to screen
		 */
		bool _output_solver = false;


	public:

		/*!
		 *	@brief Set the output to screen for the solver on/off.
		 *  @param	on	If true, output is turned on; otherwise output is turned off
		 */
		void set_solver_output_on(bool on) { _output_solver = on; }

		/*!
		 *	@brief Set the maximum number of trucks (for each type)
		 *  @param	max_nb_trucks	The maximum number of trucks
		 */
		void set_max_nb_trucks(size_t max_nb_trucks) { _max_nb_trucks = max_nb_trucks; }

		/*!
		 *	@brief Set the maximum number of segments per route
		 *  @param	max_nb_segements	The maximum number of segments
		 */
		void set_max_nb_segments(size_t max_nb_segments) { _max_nb_segments = max_nb_segments; }

		/*!
		 *	@brief Set whether to include truck objective in objective function or not
		 *  @param	include		True if truck objective should be included, false if not
		 */
		void include_truck_objective(bool include) { _include_nb_truck_objective = include; }

		/*!
		 *	@brief Set the maximum computation time
		 *  @param	max_computation_time	The maximum computation time
		 */
		void set_max_computation_time(double max_computation_time) { _max_computation_time = max_computation_time; }

		/*!
		 *	@brief Set the optimality tolerance
		 *  @param	optimality_tolerance	The optimality tolerance (should be between 0.0 and 1.0)
		 */
		void set_optimality_tolerance(double optimality_tolerance) { _optimality_tolerance = optimality_tolerance; }

		/*!
		 *	@brief Get the objective value of the solution
		 *  @returns The objective value
		 */
		double objective_value() const { return _objective_value; }

		/*!
		 *	@brief Build and solve the CPLEX model
		 *  @param	data	The problem data
		 *  @param	day		The day for which to build the routing problem
		 */
		void run(const Instance& data, size_t day);
	};

	///////////////////////////////////////////////////////////////////////////////////////////////

	/*!
	 *	@brief The model to assign generated routes to days
	 */
	class IP_model_allocation_post
	{
		/*!
		 *	@brief CPLEX environment pointer
		 */
		CPXENVptr env = nullptr;

		/*!
		 *	@brief CPLEX LP pointer
		 */
		CPXLPptr problem = nullptr;

		/*!
		 *	@brief Initialize the CPLEX environment & problem
		 */
		void initialize_cplex();

		/*!
		 *	@brief Build the CPLEX model
		 *  @param	data	The problem data
		 */
		void build_problem(const Instance& data);

		/*!
		 *	@brief Solve the CPLEX model
		 *  @param	data	The problem data
		 */
		void solve_problem(const Instance& data);

		/*!
		 *	@brief Release CPLEX memory
		 */
		void clear_cplex();

		/*!
		 *	@brief The scenario (which constraints are in the model)
		 */
		int _scenario = 0;

		/*!
		 *	@brief The maximum computation time (in seconds)
		 */
		double _max_computation_time = 600;

		/*!
		 *	@brief The objective value of the solution
		 */
		double _objective_value = -1;

		/*!
		 *	@brief The coefficient of the z_tmdw variables in the objective function
		 */
		double _objcoeff_z_tmdw = 1;

		/*!
		 *	@brief The coefficient of the beta variable in the objective function
		 */
		double _objcoeff_beta = 1;

		/*!
		 *	@brief The coefficient of the theta variables in the objective function
		 */
		double _objcoeff_theta = 1;

		/*!
		 *	@brief If true, we include theta_r per route (constraint on assignment of routes to days)
		 *		   If false, we include theta_tm per waste type and zone (constraint on max visits per zone)
		 */
		bool _penalty_on_route_assignment = false;

		/*!
		 *        @brief Print the solver's output to screen
		 */
		bool _output_solver = false;


	public:

		/*!
		 *	@brief The possible scenarios
		 */
		enum Scenario
		{
			FIXED_WEEK_SAME_DAY,	///< Restafval in ene week, GFT in andere week, zelfde dag
			FIXED_WEEK_FREE_DAY,	///< Restafval in ene week, GFT in andere week, niet noodzakelijk zelfde dag
			FREE_WEEK_FREE_DAY,		///< Vrije keuze van week of dag
		};

		/*!
		 *	@brief Set the scenario
		 *  @param scenario	The scenario
		 */
		void set_scenario(int scenario) { _scenario = scenario; }

		/*!
		 *	@brief Set the maximum computation time
		 *  @param max_computation_time	The maximum computation time
		 */
		void set_max_computation_time(double max_computation_time) { _max_computation_time = max_computation_time; }

		/*!
		 *	@brief Set the coefficient of the z_tmdw variables in the objective function
		 *  @param	value	The new value for the coefficient
		 */
		void set_coefficient_z_tmdw(double value) { _objcoeff_z_tmdw = value; }

		/*!
		 *	@brief Set the coefficient of the beta variable in the objective function
		 *  @param	value	The new value for the coefficient
		 */
		void set_coefficient_beta(double value) { _objcoeff_beta = value; }

		/*!
		 *	@brief Set the coefficient of the theta variables in the objective function
		 *  @param	value	The new value for the coefficient
		 */
		void set_coefficient_theta(double value) { _objcoeff_theta = value; }

		/*!
		 *	@brief Choose between penalty on route assignment or on max visits
		 *  @param	yes		True if penalty on route assignment, false if penalty on max visits.
		 */
		void set_penalty_route_assignment(bool yes) { _penalty_on_route_assignment = yes; }

		/*!
		 *	@brief Set the output to screen for the solver on/off.
		 *  @param	on	If true, output is turned on; otherwise output is turned off
		 */
		void set_solver_output_on(bool on) { _output_solver = on; }

		/*!
		 *	@brief Get the objective value of the solution
		 *  @returns The objective value
		 */
		double objective_value() const { return _objective_value; }

		/*!
		 *	@brief Build and solve the MIP model
		 *  @param	data	The problem data
		 */
		void run(const Instance& data);
	};
}

#endif // !MODELS_H
