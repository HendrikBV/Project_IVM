#pragma once
#ifndef MODELS_H
#define MODELS_H

#include "ilcplex/cplex.h"

namespace IVM
{
	// forward declaration
	class Data;

	/*!
	 *	@brief A small IP model to determine the required fleet size
	 */
	class IP_model_fleet_size
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

	public:
		/*!
		 *	@brief Build and solve the MIP model
		 *  @param	data	The problem data
		 */
		void run(const Data& data);
	};


	/*!
	 *	@brief A small IP model to determine the required fleet size
	 */
	class IP_model_monolithic
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
		 *	@brief CPLEX solve time limit (seconds)
		 */
		double _time_limit = 120;

	public:
		/*!
		 *	@brief Build and solve the MIP model
		 *  @param	data	The problem data
		 */
		void run(const Data& data);

		/*!
		 *	@brief Set a time limit on the maximum time CPLEX can take to solve the model
		 *  @param	time_limit	The time limit
		 */
		void set_time_limit(double time_limit) { _time_limit = time_limit; }
	};



	class IP_column_generation
	{

	};

	class IP_two_trip_heuristic
	{

	};
}

#endif // !MODELS_H
