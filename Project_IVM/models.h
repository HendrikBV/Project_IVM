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

	///////////////////////////////////////////////////////////////////////////////////////////////

	/*!
	 *	@brief The full IP model
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
		 *	@brief CPLEX solve time limit for the monolithic model (seconds)
		 */
		double _time_limit = 120;

	public:
		/*!
		 *	@brief Build and solve the monolithic MIP model
		 *  @param	data	The problem data
		 */
		void run(const Data& data);

		/*!
		 *	@brief Set a time limit on the maximum time CPLEX can take to solve the general model
		 *  @param	time_limit	The time limit
		 */
		void set_time_limit(double time_limit) { _time_limit = time_limit; }
	};

	///////////////////////////////////////////////////////////////////////////////////////////////

	/*!
	 *	@brief The full IP model but solved using a VNDS heuristic
	 */
	class IP_VNDS
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
		 *	@brief Release CPLEX memory
		 */
		void clear_cplex();

		/*!
		 *	@brief VNDS heuristic: let CPLEX solve the subproblem
		 *  @param time_limit	CPLEX time limit for the subproblem
		 *  @returns	True if a feasible solution was found, false otherwise
		 */
		bool solve_subproblem(double time_limit);

		/*!
		 *	@brief VNDS heuristic: find initial solution
		 *  @param data		The problem data
		 */
		void VNDS_initial_solution(const Data& data);

		/*!
		 *	@brief VNDS heuristic: local search using a neighborhood based on the vehicles
		 *  @param data		The problem data
		 *  @param size		The size of the neighborhood
		 *  @returns	The objective value of the new solution if one exists; otherwise 1e100
		 */
		double VNDS_neighborhood_vehicles(const Data& data, size_t size);

		/*!
		 *	@brief VNDS heuristic: local search using a neighborhood based on the days
		 *  @param data		The problem data
		 *  @param size		The size of the neighborhood
		 *  @returns	The objective value of the new solution if one exists; otherwise 1e100
		 */
		double VNDS_neighborhood_days(const Data& data, size_t size);

		/*!
		 *	@brief VNDS heuristic: local search using a neighborhood based on the customers
		 *  @param data		The problem data
		 *  @param size		The size of the neighborhood
		 *  @returns	The objective value of the new solution if one exists; otherwise 1e100
		 */
		double VNDS_neighborhood_customers(const Data& data, size_t size);

		/*!
		 *	@brief VNDS heuristic: local search using a neighborhood based on the customers
		 *  @param data		The problem data
		 *  @returns	The objective value of the new solution if one exists; otherwise 1e100
		 */
		double VNDS_neighborhood_TEST(const Data& data);

		/*!
		 *	@brief VNDS heuristic: shake phase
		 *  @param data		The problem data
		 *  @param objval	The objective value of the solution found by the shake phase
		 *  @returns	True if solution found, false otherwise
		 */
		bool VNDS_shaking(const Data& data, double& objval);

		/*!
		 *	@brief The start time for the VNDS heuristic (to calculate remaining time)
		 */
		std::chrono::system_clock::time_point _start_time;

		/*!
		 *	@brief Time limit for the VNDS heuristic (seconds)
		 */
		const double _time_limit_VNDS = 600;

		/*!
		 *	@brief CPLEX solve time limit for a subproblem (seconds)
		 */
		const double _time_limit_subproblem = 20;

		/*!
		 *	@brief Maximum number of iterations without improvement during the VND (local search) phase
		 */
		const size_t _max_iterations_VND = 3;

		/*!
		 *	@brief Objective value of the best solution thus far
		 */
		double _best_objval = 1e100;

		/*!
		 *	@brief The best solution thus far
		 */
		std::unique_ptr<double[]> _best_solution = nullptr;

		/*!
		 *	@brief The current solution
		 */
		std::unique_ptr<double[]> _current_solution = nullptr;

		/*!
		 *	@brief The new solution
		 */
		std::unique_ptr<double[]> _new_solution = nullptr;

	public:
		/*!
		 *	@brief Build the model and solve it using a VNDS heuristic
		 *  @param	data	The problem data
		 */
		void run(const Data& data);
	};

	///////////////////////////////////////////////////////////////////////////////////////////////

	class IP_column_generation
	{

	};

	///////////////////////////////////////////////////////////////////////////////////////////////

	class IP_two_trip_heuristic
	{

	};
}

#endif // !MODELS_H
