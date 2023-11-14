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
		/*!
		 *	@brief CPLEX environment pointer
		 */
		CPXENVptr env = nullptr;

		/*!
		 *	@brief CPLEX LP pointer for master problem
		 */
		CPXLPptr masterproblem = nullptr;

		/*!
		 *	@brief CPLEX LP pointer for pricing problem
		 */
		CPXLPptr pricingproblem = nullptr;

		/*!
		 *	@brief Dual prices obtained from the master problem
		 */
		std::unique_ptr<double[]> _dual_prices;

		/*!
		 *	@brief Solution of the pricing problem
		 */
		std::unique_ptr<double[]> _solution_pricingproblem;

		/*!
		 *	@brief Initialize the CPLEX environment & problem
		 */
		void initialize_cplex();

		/*!
		 *	@brief Release CPLEX memory
		 */
		void clear_cplex();

		/*!
		 *	@brief Build the CPLEX model for the master problem
		 *  @param	data	The problem data
		 */
		void build_masterproblem(const Data& data);

		/*!
		 *	@brief Build the CPLEX model for the pricing problem
		 *  @param	data	The problem data
		 */
		void build_pricingproblem(const Data& data);

		/*!
		 *	@brief Solve the CPLEX model for the master problem
		 *  @returns Objective value master
		 */
		double solve_masterproblem();

		/*!
		 *	@brief Solve the CPLEX model for the master problem
		 *  @param data		The problem data
		 */
		void change_coefficients_pricingproblem(const Data& data);

		/*!
		 *	@brief Solve the CPLEX model for the master problem
		 *  @returns Reduced cost
		 */
		double solve_pricingproblem();

		/*!
		 *	@brief Add the new column to the master problem 
		 *  @param data		The problem data
		 *  @param iteration	The iteration of the column generation
		 */
		void add_column_to_masterproblem(const Data& data, int iteration);

		/*!
		 *	@brief Should the valid inequality (eq. 37 paper) be added to the pricing problem?
		 */
		const bool _add_valid_inequality_pricing = true;

		/*!
		 *	@brief The big M used in the master problem and pricing problem
		 */
		const int _big_M = 1000;

		/*!
		 *	@brief Solve the master problem as a MIP after column generation is finished
		 *  @returns	The objective value of the best solution
		 */
		double solve_master_as_MIP();

		/*!
		 *	@brief Time limit for solving the master problem as MIP
		 */
		double _time_limit_MIP = 3600;

		/*!
		 *	@brief Find the branching variable
		 *  @param data		The problem data
		 *  @param branching_variable_index		To store the found branching variable
		 *  @returns	True if fractional solution, false if integer
		 */
		bool find_branching_variable_diving(const Data& data, int& branching_variable_index);

		/*!
		 *	@brief Add the branching restriction
		 *  @param branching_variable_index		The index of the variable set to 1
		 */
		void add_branching_restriction_diving(int branching_variable_index);
			
		/*!
		 *	@brief Remove columns that are no longer necessary after a variable is set to 1
		 *  @param branching_variable_index		The index of the variable set to 1
		 */
		void remove_columns_that_violate_restriction_diving(int branching_variable_index);

		/*!
		 *	@brief Save the solution
		 */
		void save_solution();

		/*!
		 *	@brief Find the branching variable
		 *  @param data		The problem data
		 *  @param branching_customer	The customer on which we branch
		 *  @param branching_day		The day on which we branch
		 *  @param branching_amount		The value of a_km 
		 *  @returns	True if fractional solution, false if integer
		 */
		bool find_branching_variable_bp(const Data& data, int& branching_customer, int& branching_day, double& branching_amount);

		/*!
		 *	@brief Add the branching restriction to the pricing problem
		 *  @param data		The problem data
		 *  @param left_branch			True if left branch, false if right branch
		 *  @param branching_customer	The customer on which we branch
		 *  @param branching_day		The day on which we branch
		 *  @param branching_amount		The value of a_km 
		 */
		void add_branching_restriction_bp(const Data& data, bool left_branch, int branching_customer, int branching_day, double branching_amount);

		/*!
		 *	@brief Delete the last branching restriction upon backtracking
		 */
		void delete_branching_restriction_bp();

		/*!
		 *	@brief Delete the columns that violate the new branching restriction from the master problem
		 *  @param data		The problem data
		 *  @param left_branch			True if left branch, false if right branch
		 *  @param branching_customer	The customer on which we branch
		 *  @param branching_day		The day on which we branch
		 *  @param branching_amount		The value of a_km 
		 */
		void delete_columns_bp(const Data& data, bool left_branch, int branching_customer, int branching_day, double branching_amount);

		/*!
		 *	@brief Restore columns that were deleted with the last branching restriction to the master problem
		 */
		void restore_columns_bp();


	public:
		/*!
		 *	@brief Run column generation (fractional solution)
		 *  @param	data	The problem data
		 */
		void run_column_generation(const Data& data);

		/*!
		 *	@brief Solve the root node of the column generation phase with integrality constraints
		 *  @param	data	The problem data
		 */
		void run_CG_MIP_heuristic(const Data& data);

		/*!
		 *	@brief Run a diving heuristic (heuristic branching)
		 *  @param	data	The problem data
		 */
		void run_diving_heuristic(const Data& data);

		/*!
		 *	@brief Run branch-and-price (optimal solution)
		 *  @param	data	The problem data
		 */
		void run_branch_and_price(const Data& data);
	};

	///////////////////////////////////////////////////////////////////////////////////////////////

	class IP_two_trip_heuristic
	{

	};
}

#endif // !MODELS_H
