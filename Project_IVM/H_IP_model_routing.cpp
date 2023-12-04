#include "models.h"
#include "data.h"
#include <stdexcept>
#include <memory>
#include <iostream>
#include <chrono>



namespace IVM
{
	///////////////////////////////////////////
	///			  IP Model Routing			///
	///////////////////////////////////////////

	void IP_model_routing::initialize_cplex()
	{
		int status = 0;
		char error_text[CPXMESSAGEBUFSIZE];

		// open the CPLEX environment
		env = CPXopenCPLEX(&status);
		if (status != 0)
		{
			CPXgeterrorstring(env, status, error_text);
			throw std::runtime_error("Error in function IP_model_routing::initialize_cplex(). \nCouldn't open CPLEX. \nReason: " + std::string(error_text));
		}

		// turn output to screen on/off
		status = CPXsetintparam(env, CPX_PARAM_SCRIND, CPX_ON);
		if (status != 0)
		{
			CPXgeterrorstring(env, status, error_text);
			throw std::runtime_error("Error in function IP_model_routing::initialize_cplex(). \nCouldn't change param SCRIND. \nReason: " + std::string(error_text));
		}
	}

	void IP_model_routing::build_problem(const Instance& data, size_t day)
	{
		char error_text[CPXMESSAGEBUFSIZE];
		int status = 0;
		double obj[1];			// Objective function
		double lb[1];			// Lower bound variables
		double ub[1];			// Upper bound variables
		double rhs[1];			// Right-hand side constraints
		char sense[1];			// Sign of constraint
		char type[1];			// Type of variable (integer, binary, fractional)
		int nonzeroes = 0;		// To calculate number of nonzero coefficients in each constraint
		int matbeg[1];			// Begin position of the constraint
		std::unique_ptr<int[]> matind; // Position of each element in constraint matrix
		std::unique_ptr<double[]> matval; // Value of each element in constraint matrix

		matbeg[0] = 0;

		// allocate memory
		const size_t maxnonzeroes = 100000;
		matind = std::make_unique<int[]>(maxnonzeroes);
		matval = std::make_unique<double[]>(maxnonzeroes);


		// create the problem
		problem = CPXcreateprob(env, &status, "IP_model_routing");

		// problem is minimization
		status = CPXchgobjsen(env, problem, CPX_MIN);

		// data 
		const size_t nb_waste_types = data.nb_waste_types();
		const size_t nb_truck_types = data.nb_truck_types();
		const size_t nb_zones = data.nb_zones();
		const size_t nb_days = data.nb_days();
		const size_t nb_weeks = data.nb_weeks();
		const size_t nb_collection_points = data.nb_collection_points();
		const size_t nb_locations = nb_zones + 1 + nb_collection_points; // Order: Z1...Zn, depot, CP1...CPk

		const int bigM = 10000;


		// add variables
		// variable x_tqvij   day is given
		const int startindex_x_tqvij = 0;
		for (int t = 0; t < nb_waste_types; ++t)
		{
			for (int q = 0; q < nb_truck_types; ++q)
			{
				for (int v = 0; v < _max_nb_trucks; ++v)
				{
					for (int i = 0; i < nb_locations; ++i)
					{
						for (int j = 0; j < nb_locations; ++j)
						{
							obj[0] = 0;
							lb[0] = 0;
							ub[0] = 1;
							type[0] = 'B';

							status = CPXnewcols(env, problem, 1, obj, lb, ub, type, NULL);
							if (status != 0)
							{
								CPXgeterrorstring(env, status, error_text);
								throw std::runtime_error("Error in function IP_model_routing::build_problem(). \nCouldn't add variable. \nReason: " + std::string(error_text));
							}

							// change variable name
							std::string varname = "x_" + std::to_string(t + 1) + "_" + std::to_string(q + 1) + "_"
								+ std::to_string(v + 1) + "_" + std::to_string(i + 1) + "_" + std::to_string(j + 1);
							status = CPXchgname(env, problem, 'c', startindex_x_tqvij + t * nb_truck_types * _max_nb_trucks * nb_locations * nb_locations 
								+ q * _max_nb_trucks * nb_locations * nb_locations + v * nb_locations * nb_locations + i * nb_locations + j, varname.c_str());
							if (status != 0)
							{
								CPXgeterrorstring(env, status, error_text);
								throw std::runtime_error("Error in function IP_model_routing::build_problem(). \nCouldn't change variable name. \nReason: " + std::string(error_text));
							}
						}
					}
				}
			}
		}

		// variable y_tqv   day is given
		const int startindex_y_tqv = startindex_x_tqvij + nb_waste_types * nb_truck_types * _max_nb_trucks * nb_locations * nb_locations;
		for (int t = 0; t < nb_waste_types; ++t)
		{
			for (int q = 0; q < nb_truck_types; ++q)
			{
				for (int v = 0; v < _max_nb_trucks; ++v)
				{
					obj[0] = 0;
					lb[0] = 0;
					ub[0] = 1;
					type[0] = 'B';

					status = CPXnewcols(env, problem, 1, obj, lb, ub, type, NULL);
					if (status != 0)
					{
						CPXgeterrorstring(env, status, error_text);
						throw std::runtime_error("Error in function IP_model_routing::build_problem(). \nCouldn't add variable. \nReason: " + std::string(error_text));
					}

					// change variable name
					std::string varname = "y_" + std::to_string(t + 1) + "_" + std::to_string(q + 1) + "_" + std::to_string(v + 1);
					status = CPXchgname(env, problem, 'c', startindex_y_tqv + t * nb_truck_types * _max_nb_trucks + q * _max_nb_trucks + v, varname.c_str());
					if (status != 0)
					{
						CPXgeterrorstring(env, status, error_text);
						throw std::runtime_error("Error in function IP_model_routing::build_problem(). \nCouldn't change variable name. \nReason: " + std::string(error_text));
					}
				}
			}
		}

		// variable w_tqvm   day is given
		const int startindex_w_tqvm = startindex_y_tqv + nb_waste_types * nb_truck_types * _max_nb_trucks;
		for (int t = 0; t < nb_waste_types; ++t)
		{
			for (int q = 0; q < nb_truck_types; ++q)
			{
				for (int v = 0; v < _max_nb_trucks; ++v)
				{
					for (int m = 0; m < nb_zones; ++m)
					{
						obj[0] = 0;
						lb[0] = 0;

						status = CPXnewcols(env, problem, 1, obj, lb, NULL, NULL, NULL);
						if (status != 0)
						{
							CPXgeterrorstring(env, status, error_text);
							throw std::runtime_error("Error in function IP_model_routing::build_problem(). \nCouldn't add variable. \nReason: " + std::string(error_text));
						}

						// change variable name
						std::string varname = "w_" + std::to_string(t + 1) + "_" + std::to_string(q + 1) + "_" + std::to_string(v + 1) + "_" + std::to_string(m + 1);
						status = CPXchgname(env, problem, 'c', startindex_w_tqvm + t * nb_truck_types * _max_nb_trucks * nb_zones
							+ q * _max_nb_trucks * nb_zones + v * nb_zones + m, varname.c_str());
						if (status != 0)
						{
							CPXgeterrorstring(env, status, error_text);
							throw std::runtime_error("Error in function IP_model_routing::build_problem(). \nCouldn't change variable name. \nReason: " + std::string(error_text));
						}
					}
				}
			}
		}

		// variable beta_qv   day is given
		const int startindex_beta_qv = startindex_w_tqvm + nb_waste_types * nb_truck_types * _max_nb_trucks * nb_zones;
		for (int q = 0; q < nb_truck_types; ++q)
		{
			for (int v = 0; v < _max_nb_trucks; ++v)
			{
				obj[0] = 0;
				lb[0] = 0;

				status = CPXnewcols(env, problem, 1, obj, lb, NULL, NULL, NULL);
				if (status != 0)
				{
					CPXgeterrorstring(env, status, error_text);
					throw std::runtime_error("Error in function IP_model_routing::build_problem(). \nCouldn't add variable. \nReason: " + std::string(error_text));
				}

				// change variable name
				std::string varname = "beta_" + std::to_string(q + 1) + "_" + std::to_string(v + 1);
				status = CPXchgname(env, problem, 'c', startindex_beta_qv + q * _max_nb_trucks + v, varname.c_str());
				if (status != 0)
				{
					CPXgeterrorstring(env, status, error_text);
					throw std::runtime_error("Error in function IP_model_routing::build_problem(). \nCouldn't change variable name. \nReason: " + std::string(error_text));
				}
			}
		}

		// variable u_qvi   day is given
		const int startindex_u_qvi = startindex_beta_qv + nb_truck_types * _max_nb_trucks;
		for (int q = 0; q < nb_truck_types; ++q)
		{
			for (int v = 0; v < _max_nb_trucks; ++v)
			{
				for (int i = 0; i < nb_locations; ++i)
				{
					obj[0] = 0;
					lb[0] = 0;

					status = CPXnewcols(env, problem, 1, obj, lb, NULL, NULL, NULL);
					if (status != 0)
					{
						CPXgeterrorstring(env, status, error_text);
						throw std::runtime_error("Error in function IP_model_routing::build_problem(). \nCouldn't add variable. \nReason: " + std::string(error_text));
					}

					// change variable name
					std::string varname = "u_" + std::to_string(q + 1) + "_" + std::to_string(v + 1) + "_" + std::to_string(i + 1);
					status = CPXchgname(env, problem, 'c', startindex_u_qvi + q * _max_nb_trucks * nb_locations + v * nb_locations + i, varname.c_str());
					if (status != 0)
					{
						CPXgeterrorstring(env, status, error_text);
						throw std::runtime_error("Error in function IP_model_routing::build_problem(). \nCouldn't change variable name. \nReason: " + std::string(error_text));
					}
				}
			}
		}



		// add constraints
		int nb_constraints = -1;

		// 1: beta_qv - sum(i,j) tau_D_ij*x_qvij - sum(m) tau_P_m*w_tqvm - tau_U_t*y_tqv == 0   forall t,q,v
		for (int t = 0; t < nb_waste_types; ++t)
		{
			for (int q = 0; q < nb_truck_types; ++q)
			{
				for (int v = 0; v < _max_nb_trucks; ++v)
				{
					++nb_constraints;

					rhs[0] = 0;
					sense[0] = 'E';
					matbeg[0] = 0;

					nonzeroes = 0;

					// beta_qv
					matind[nonzeroes] = startindex_beta_qv + q * _max_nb_trucks + v;
					matval[nonzeroes] = 1;
					++nonzeroes;

					// -sum(i,j) tau_D_ij* x_qvij
					for (int i = 0; i < nb_locations; ++i)
					{
						for (int j = 0; j < nb_locations; ++j)
						{
							matind[nonzeroes] = startindex_x_tqvij + t * nb_truck_types * _max_nb_trucks * nb_locations * nb_locations + q * _max_nb_trucks * nb_locations * nb_locations
								+ v * nb_locations * nb_locations + i * nb_locations + j;
							matval[nonzeroes] = - 
							++nonzeroes;
						}
					}

					if (nonzeroes >= maxnonzeroes)
						throw std::runtime_error("Error in function IP_model_routing::build_problem(). Nonzeroes exceeds size of maxnonzeroes (matind and matval)");

					status = CPXaddrows(env, problem, 0, 1, nonzeroes, rhs, sense, matbeg, matind.get(), matval.get(), NULL, NULL);
					if (status != 0)
					{
						CPXgeterrorstring(env, status, error_text);
						throw std::runtime_error("Error in function IP_model_routing::build_problem(). \nCouldn't add constraint. \nReason: " + std::string(error_text));
					}

					// change name of constraint
					std::string conname = "c1_" + std::to_string(t + 1) + "_" + std::to_string(m + 1) + "_" + std::to_string(d + 1) + "_" + std::to_string(w + 1);
					status = CPXchgname(env, problem, 'r', nb_constraints, conname.c_str());
					if (status != 0)
					{
						CPXgeterrorstring(env, status, error_text);
						throw std::runtime_error("Error in function IP_model_routing::build_problem(). \nCouldn't change constraint name. \nReason: " + std::string(error_text));
					}
				}
			}
		}

		


		// write to file
		status = CPXwriteprob(env, problem, "IP_model_routing.lp", NULL);
		if (status != 0)
		{
			CPXgeterrorstring(env, status, error_text);
			throw std::runtime_error("Error in function IP_model_routing::build_problem(). \nCouldn't write problem to lp-file. \nReason: " + std::string(error_text));
		}
	}

	void IP_model_routing::solve_problem(const Instance& data)
	{
		char error_text[CPXMESSAGEBUFSIZE];
		int status = 0;
		int solstat = 0;
		std::unique_ptr<double[]> solution_problem;
		double objval;

		// Assign memory for solution
		solution_problem = std::make_unique<double[]>(CPXgetnumcols(env, problem));


		// Optimize the problem
		std::cout << "\n\n\nIP_model_allocation: CPLEX is solving the problem ...\n";
		auto start_time = std::chrono::system_clock::now();

		status = CPXmipopt(env, problem);
		if (status != 0)
		{
			CPXgeterrorstring(env, status, error_text);
			throw std::runtime_error("Error in function IP_model_routing::solve_problem(). \nCPXmipopt failed. \nReason: " + std::string(error_text));
		}

		std::chrono::duration<double, std::ratio<1, 1>> elapsed_time_IP = std::chrono::system_clock::now() - start_time;


		// Get the solution
		status = CPXsolution(env, problem, &solstat, &objval, solution_problem.get(), NULL, NULL, NULL);
		if (status != 0)
		{
			CPXgeterrorstring(env, status, error_text);
			throw std::runtime_error("Error in function IP_model_routing::solve_problem(). \nCPXsolution failed. \nReason: " + std::string(error_text));
		}

		char solstat_text[CPXMESSAGEBUFSIZE];
		auto p = CPXgetstatstring(env, solstat, solstat_text);
		if (p != nullptr)
		{
			std::cout << "\n\n\nDone solving ... \n\nSolution status: " << solstat_text;

			if (solstat == CPXMIP_OPTIMAL || solstat == CPXMIP_OPTIMAL_TOL)
			{
				std::cout << "\nObjval = " << objval;
				std::cout << "\nElapsed time (s): " << elapsed_time_IP.count();
			}
		}


	}

	void IP_model_routing::clear_cplex()
	{
		int status = 0;
		char error_text[CPXMESSAGEBUFSIZE];

		// Free the problem
		status = CPXfreeprob(env, &problem);
		if (status != 0)
		{
			CPXgeterrorstring(env, status, error_text);
			throw std::runtime_error("Error in function IP_model_routing::clear_cplex(). \nCouldn't free problem. \nReason: " + std::string(error_text));
		}

		// Close the cplex environment
		status = CPXcloseCPLEX(&env);
		if (status != 0)
		{
			CPXgeterrorstring(env, status, error_text);
			throw std::runtime_error("Error in function IP_model_routing::clear_cplex(). \nCouldn't close cplex environment. \nReason: " + std::string(error_text));
		}
	}

	void IP_model_routing::run(const Instance& data)
	{
		initialize_cplex();
		build_problem(data);
		solve_problem(data);
		clear_cplex();
	}

	///////////////////////////////////////////////////////////////////////////////////////////////
}