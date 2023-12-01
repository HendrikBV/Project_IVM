#pragma once
#ifndef MODELS_H
#define MODELS_H

#include "ilcplex/cplex.h"
#include <memory>
#include <chrono>
#include <vector>

namespace IVM
{
	// forward declaration
	class Data;



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
		void build_problem(const Data& data);

		/*!
		 *	@brief Solve the CPLEX model
		 *  @param	data	The problem data
		 */
		void solve_problem(const Data& data);

		/*!
		 *	@brief Release CPLEX memory
		 */
		void clear_cplex();

		/*!
		 *	@brief The percentage of deviations that is allowed compared to the current calendar
		 */
		double _pct_allowed_deviations;

		/*!
		 *	@brief The scenario (which constraints are in the model)
		 */
		int _scenario = 0;

	public:
		/*!
		 *	@brief Set the percentage of deviations that is allowed compared to the current calendar
		 *  @param	pct	The percentage of deviations that is allowed
		 */
		void set_pct_allowed_deviations(double pct) { _pct_allowed_deviations = pct; }

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
		void run(const Data& data);
	};
}

#endif // !MODELS_H
