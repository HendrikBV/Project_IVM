/*
	Copyright (c) 2023 Hendrik Vermuyten

	Licensed under the Apache License, Version 2.0 (the "License");
	you may not use this file except in compliance with the License.
	You may obtain a copy of the License at

	http://www.apache.org/licenses/LICENSE-2.0
*/



/*!
 *  @file       Models.h
 *  @brief      Defines the IP models for the IVM scheduling problem
 */

#pragma once
#ifndef MODELS_H
#define MODELS_H

#include "ilcplex/cplex.h"
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
		 *	@brief The maximum computation time (in seconds)
		 */
		double _max_computation_time = 60;

	public:
		/*!
		 *	@brief Set the fraction of deviations that is allowed compared to the current calendar
		 *  @param	fraction	The fraction of deviations that is allowed (between 0 and 1)
		 */
		void set_fraction_allowed_deviations(double fraction) { _fraction_allowed_deviations = fraction; }

		/*!
		 *	@brief The possible scenarios
		 */
		enum Scenario
		{
			FIXED_WEEK_SAME_DAY,
			FIXED_WEEK_FREE_DAY,
			FREE_WEEK_FREE_DAY
		};

		/*!
		 *	@brief Set the scenario
		 *  @param scenario	The scenario
		 */
		void set_scenario(int scenario) { _scenario = scenario; }

		/*!
		 *	@brief Build and solve the MIP model
		 *  @param	data	The problem data
		 */
		void run(const Instance& data);
	};

	///////////////////////////////////////////////////////////////////////////

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
		const size_t _max_nb_trucks = 10;

		/*!
		 *	@brief The maximum number of segments in a route
		 */
		const size_t _max_nb_segments = 10;

		/*!
		 *	@brief Auxiliary function to give the correct index for the x variables
		 *  @param data	The problem data
		 *  @param q	index for the truck type
		 *  @param v	index for the vehicle
		 *  @param i	index for the origin location
		 *  @param j	index for the destination location
		 *  @param k	index for the route segment
		 *  @returns	the index of the x_qvijk variable in the model
		 */
		size_t _index_x_qvijk(const Instance& data, int q, int v, int i, int j, int k) const;

		/*!
		 *	@brief Auxiliary function to give the correct index for the w variables
		 *  @param data	The problem data
		 *  @param t	index for the waste type
		 *  @param q	index for the truck type
		 *  @param v	index for the vehicle
		 *  @param i	index for the location
		 *  @param k	index for the route segment
		 *  @returns	the index of the w_tqvik variable in the model
		 */
		size_t _index_w_tqvik(const Instance& data, int t, int q, int v, int i_zone, int k) const;

		/*!
		 *	@brief Auxiliary function to give the correct index for the y variables
		 *  @param data	The problem data
		 *  @param q	index for the truck type
		 *  @param v	index for the vehicle
		 *  @returns	the index of the y_qv variable in the model
		 */
		size_t _index_y_qv(const Instance& data, int q, int v) const;

		/*!
		 *	@brief Auxiliary function to give the correct index for the beta variables
		 *  @param data	The problem data
		 *  @param q	index for the truck type
		 *  @param v	index for the vehicle
		 *  @returns	the index of the beta_qv variable in the model
		 */
		size_t _index_beta_qv(const Instance& data, int q, int v) const;

		/*!
		 *	@brief Indicates whether we minimize the number of trucks in the objective function or not
		 */
		bool _include_nb_truck_objective = true;

		/*!
		 *	@brief The maximum computation time (in seconds)
		 */
		double _max_computation_time = 600;

	public:
		/*!
		 *	@brief Build and solve the CPLEX model
		 *  @param	data	The problem data
		 *  @param	day		The day for which to build the routing problem
		 */
		void run(const Instance& data, size_t day);
	};
}

#endif // !MODELS_H
