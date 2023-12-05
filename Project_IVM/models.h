
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
	// forward declarations
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
		 */
		void solve_problem(const Instance& data);

		/*!
		 *	@brief Release CPLEX memory
		 */
		void clear_cplex();

		/*!
		 *	@brief The available number of trucks
		 */
		const size_t _max_nb_trucks = 10;

		std::vector<double> _Q_tmd;

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
