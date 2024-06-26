/*
	Copyright (c) 2024 KU Leuven
	Code author: Hendrik Vermuyten
*/



#include "models.h"
#include "data.h"
#include <stdexcept>
#include <memory>
#include <iostream>
#include <fstream>
#include <chrono>



namespace IVM
{
	///////////////////////////////////////////
	///			 IP Model Allocation 		///
	///////////////////////////////////////////

	void IP_model_allocation::initialize_cplex()
	{
		int status = 0;
		char error_text[CPXMESSAGEBUFSIZE];

		// open the CPLEX environment
		env = CPXopenCPLEX(&status);
		if (status != 0)
		{
			CPXgeterrorstring(env, status, error_text);
			throw std::runtime_error("Error in function IP_model_allocation::initialize_cplex(). \nCouldn't open CPLEX. \nReason: " + std::string(error_text));
		}

		// turn output to screen on/off
		if(_output_solver)
			status = CPXsetintparam(env, CPXPARAM_ScreenOutput, CPX_ON);
		else
			status = CPXsetintparam(env, CPXPARAM_ScreenOutput, CPX_OFF);
		if (status != 0)
		{
			CPXgeterrorstring(env, status, error_text);
			throw std::runtime_error("Error in function IP_model_allocation::initialize_cplex(). \nCouldn't change param SCRIND. \nReason: " + std::string(error_text));
		}
	}

	void IP_model_allocation::build_problem(const Instance& data)
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
		problem = CPXcreateprob(env, &status, "IP_model_allocation");
		if (status != 0)
		{
			CPXgeterrorstring(env, status, error_text);
			throw std::runtime_error("Error in function IP_model_allocation::build_problem(). \nCouldn't create problem. \nReason: " + std::string(error_text));
		}

		// problem is minimization
		status = CPXchgobjsen(env, problem, CPX_MIN);
		if (status != 0)
		{
			CPXgeterrorstring(env, status, error_text);
			throw std::runtime_error("Error in function IP_model_allocation::build_problem(). \nCouldn't change objective to minimization. \nReason: " + std::string(error_text));
		}


		// data 
		const size_t nb_types = data.nb_waste_types();
		const size_t nb_zones = data.nb_zones();
		const size_t nb_days = data.nb_days();
		const size_t nb_weeks = data.nb_weeks();

		const int bigM = 10000;


		// add variables
		int nb_variables = -1;

		// variable x_tmdw
		const int startindex_x_tmdw = 0;
		for (int t = 0; t < nb_types; ++t)
		{
			for (int m = 0; m < nb_zones; ++m)
			{
				for (int d = 0; d < nb_days; ++d)
				{
					for (int w = 0; w < nb_weeks; ++w)
					{
						++nb_variables;

						obj[0] = 0;
						lb[0] = 0;

						status = CPXnewcols(env, problem, 1, obj, lb, NULL, NULL, NULL);
						if (status != 0)
						{
							CPXgeterrorstring(env, status, error_text);
							throw std::runtime_error("Error in function IP_model_allocation::build_problem(). \nCouldn't add variable. \nReason: " + std::string(error_text));
						}

						// change variable name
						std::string varname = "x_" + std::to_string(t + 1) + "_" + std::to_string(m + 1) + "_" + std::to_string(d + 1) + "_" + std::to_string(w + 1);
						status = CPXchgname(env, problem, 'c', nb_variables, varname.c_str());
						if (status != 0)
						{
							CPXgeterrorstring(env, status, error_text);
							throw std::runtime_error("Error in function IP_model_allocation::build_problem(). \nCouldn't change variable name. \nReason: " + std::string(error_text));
						}
					}
				}
			}
		}

		// variable y_tmdw
		const int startindex_y_tmdw = startindex_x_tmdw + nb_types * nb_zones * nb_days * nb_weeks;
		for (int t = 0; t < nb_types; ++t)
		{
			for (int m = 0; m < nb_zones; ++m)
			{
				for (int d = 0; d < nb_days; ++d)
				{
					for (int w = 0; w < nb_weeks; ++w)
					{
						++nb_variables;

						obj[0] = 0;
						lb[0] = 0;
						ub[0] = 1;
						type[0] = 'B';

						status = CPXnewcols(env, problem, 1, obj, lb, ub, type, NULL);
						if (status != 0)
						{
							CPXgeterrorstring(env, status, error_text);
							throw std::runtime_error("Error in function IP_model_allocation::build_problem(). \nCouldn't add variable. \nReason: " + std::string(error_text));
						}

						// change variable name
						std::string varname = "y_" + std::to_string(t + 1) + "_" + std::to_string(m + 1) + "_" + std::to_string(d + 1) + "_" + std::to_string(w + 1);
						status = CPXchgname(env, problem, 'c', nb_variables, varname.c_str());
						if (status != 0)
						{
							CPXgeterrorstring(env, status, error_text);
							throw std::runtime_error("Error in function IP_model_allocation::build_problem(). \nCouldn't change variable name. \nReason: " + std::string(error_text));
						}
					}
				}
			}
		}

		// variable z_tmdw
		const int startindex_z_tmdw = startindex_y_tmdw + nb_types * nb_zones * nb_days * nb_weeks;
		for (int t = 0; t < nb_types; ++t)
		{
			for (int m = 0; m < nb_zones; ++m)
			{
				for (int d = 0; d < nb_days; ++d)
				{
					for (int w = 0; w < nb_weeks; ++w)
					{
						++nb_variables;

						obj[0] = 0;
						lb[0] = 0;
						ub[0] = 1;
						type[0] = 'B';

						status = CPXnewcols(env, problem, 1, obj, lb, ub, type, NULL);
						if (status != 0)
						{
							CPXgeterrorstring(env, status, error_text);
							throw std::runtime_error("Error in function IP_model_allocation::build_problem(). \nCouldn't add variable. \nReason: " + std::string(error_text));
						}

						// change variable name
						std::string varname = "z_" + std::to_string(t + 1) + "_" + std::to_string(m + 1) + "_" + std::to_string(d + 1) + "_" + std::to_string(w + 1);
						status = CPXchgname(env, problem, 'c', nb_variables, varname.c_str());
						if (status != 0)
						{
							CPXgeterrorstring(env, status, error_text);
							throw std::runtime_error("Error in function IP_model_allocation::build_problem(). \nCouldn't change variable name. \nReason: " + std::string(error_text));
						}
					}
				}
			}
		}

		// variable e_tdw
		const int startindex_e_tdw = startindex_z_tmdw + nb_types * nb_zones * nb_days * nb_weeks;
		for (int t = 0; t < nb_types; ++t)
		{
			for (int d = 0; d < nb_days; ++d)
			{
				for (int w = 0; w < nb_weeks; ++w)
				{
					++nb_variables;

					obj[0] = 1;
					lb[0] = 0;

					status = CPXnewcols(env, problem, 1, obj, lb, NULL, NULL, NULL);
					if (status != 0)
					{
						CPXgeterrorstring(env, status, error_text);
						throw std::runtime_error("Error in function IP_model_allocation::build_problem(). \nCouldn't add variable. \nReason: " + std::string(error_text));
					}

					// change variable name
					std::string varname = "beta_" + std::to_string(t + 1) + "_" + std::to_string(d + 1) + "_" + std::to_string(w + 1);
					status = CPXchgname(env, problem, 'c', nb_variables, varname.c_str());
					if (status != 0)
					{
						CPXgeterrorstring(env, status, error_text);
						throw std::runtime_error("Error in function IP_model_allocation::build_problem(). \nCouldn't change variable name. \nReason: " + std::string(error_text));
					}
				}
			}
		}



		// add constraints
		int nb_constraints = -1;

		// 1: x_tmdw <= N y_tmdw   forall t,m,d,w
		for (int t = 0; t < nb_types; ++t)
		{
			for (int m = 0; m < nb_zones; ++m)
			{
				for (int d = 0; d < nb_days; ++d)
				{
					for (int w = 0; w < nb_weeks; ++w)
					{
						++nb_constraints;

						rhs[0] = 0;
						sense[0] = 'L';
						matbeg[0] = 0;

						nonzeroes = 0;

						// x_tmdw
						matind[nonzeroes] = startindex_x_tmdw + t * nb_zones * nb_days * nb_weeks + m * nb_days * nb_weeks + d * nb_weeks + w;
						matval[nonzeroes] = 1;
						++nonzeroes;

						// y_tmdw
						matind[nonzeroes] = startindex_y_tmdw + t * nb_zones * nb_days * nb_weeks + m * nb_days * nb_weeks + d * nb_weeks + w;
						matval[nonzeroes] = -bigM;
						++nonzeroes;

						if (nonzeroes >= maxnonzeroes)
							throw std::runtime_error("Error in function IP_model_allocation::build_problem(). Nonzeroes exceeds size of maxnonzeroes (matind and matval)");

						status = CPXaddrows(env, problem, 0, 1, nonzeroes, rhs, sense, matbeg, matind.get(), matval.get(), NULL, NULL);
						if (status != 0)
						{
							CPXgeterrorstring(env, status, error_text);
							throw std::runtime_error("Error in function IP_model_allocation::build_problem(). \nCouldn't add constraint. \nReason: " + std::string(error_text));
						}

						// change name of constraint
						std::string conname = "c1_" + std::to_string(t + 1) + "_" + std::to_string(m + 1) + "_" + std::to_string(d + 1) + "_" + std::to_string(w + 1);
						status = CPXchgname(env, problem, 'r', nb_constraints, conname.c_str());
						if (status != 0)
						{
							CPXgeterrorstring(env, status, error_text);
							throw std::runtime_error("Error in function IP_model_allocation::build_problem(). \nCouldn't change constraint name. \nReason: " + std::string(error_text));
						}
					}
				}
			}
		}

		// 2: sum(d,w) x_tmdw == Q_tm   forall t,m
		for (int t = 0; t < nb_types; ++t)
		{
			for (int m = 0; m < nb_zones; ++m)
			{
				++nb_constraints;

				const std::string& waste_type = data.waste_type(t);
				rhs[0] = data.demand(m, waste_type);
				sense[0] = 'E';
				matbeg[0] = 0;

				nonzeroes = 0;

				// x_tmdw
				for (int d = 0; d < nb_days; ++d)
				{
					for (int w = 0; w < nb_weeks; ++w)
					{
						matind[nonzeroes] = startindex_x_tmdw + t * nb_zones * nb_days * nb_weeks + m * nb_days * nb_weeks + d * nb_weeks + w;
						matval[nonzeroes] = 1;
						++nonzeroes;
					}
				}

				if (nonzeroes >= maxnonzeroes)
					throw std::runtime_error("Error in function IP_model_allocation::build_problem(). Nonzeroes exceeds size of maxnonzeroes (matind and matval)");

				status = CPXaddrows(env, problem, 0, 1, nonzeroes, rhs, sense, matbeg, matind.get(), matval.get(), NULL, NULL);
				if (status != 0)
				{
					CPXgeterrorstring(env, status, error_text);
					throw std::runtime_error("Error in function IP_model_allocation::build_problem(). \nCouldn't add constraint. \nReason: " + std::string(error_text));
				}

				// change name of constraint
				std::string conname = "c2_" + std::to_string(t + 1) + "_" + std::to_string(m + 1);
				status = CPXchgname(env, problem, 'r', nb_constraints, conname.c_str());
				if (status != 0)
				{
					CPXgeterrorstring(env, status, error_text);
					throw std::runtime_error("Error in function IP_model_allocation::build_problem(). \nCouldn't change constraint name. \nReason: " + std::string(error_text));
				}
			}
		}

		// 3: sum(d,w) y_tmdw <= W   forall t,m
		for (int t = 0; t < nb_types; ++t)
		{
			for (int m = 0; m < nb_zones; ++m)
			{
				++nb_constraints;

				rhs[0] = data.max_visits();

				sense[0] = 'L';
				matbeg[0] = 0;

				nonzeroes = 0;

				// y_tmdw
				for (int d = 0; d < nb_days; ++d)
				{
					for (int w = 0; w < nb_weeks; ++w)
					{
						matind[nonzeroes] = startindex_y_tmdw + t * nb_zones * nb_days * nb_weeks + m * nb_days * nb_weeks + d * nb_weeks + w;
						matval[nonzeroes] = 1;
						++nonzeroes;
					}
				}

				if (nonzeroes >= maxnonzeroes)
					throw std::runtime_error("Error in function IP_model_allocation::build_problem(). Nonzeroes exceeds size of maxnonzeroes (matind and matval)");

				status = CPXaddrows(env, problem, 0, 1, nonzeroes, rhs, sense, matbeg, matind.get(), matval.get(), NULL, NULL);
				if (status != 0)
				{
					CPXgeterrorstring(env, status, error_text);
					throw std::runtime_error("Error in function IP_model_allocation::build_problem(). \nCouldn't add constraint. \nReason: " + std::string(error_text));
				}

				// change name of constraint
				std::string conname = "c3_" + std::to_string(t + 1) + "_" + std::to_string(m + 1);
				status = CPXchgname(env, problem, 'r', nb_constraints, conname.c_str());
				if (status != 0)
				{
					CPXgeterrorstring(env, status, error_text);
					throw std::runtime_error("Error in function IP_model_allocation::build_problem(). \nCouldn't change constraint name. \nReason: " + std::string(error_text));
				}
			}
		}

		// 4: y_tmdw - z_tmdw <= h_tmdw   forall t,m,d,w
		for (int t = 0; t < nb_types; ++t)
		{
			for (int m = 0; m < nb_zones; ++m)
			{
				for (int d = 0; d < nb_days; ++d)
				{
					for (int w = 0; w < nb_weeks; ++w)
					{
						++nb_constraints;

						const std::string& waste_type = data.waste_type(t);
						rhs[0] = data.current_calendar(m, waste_type, d, w); // convert bool to int

						sense[0] = 'L';
						matbeg[0] = 0;

						nonzeroes = 0;

						// y_tmdw
						matind[nonzeroes] = startindex_y_tmdw + t * nb_zones * nb_days * nb_weeks + m * nb_days * nb_weeks + d * nb_weeks + w;
						matval[nonzeroes] = 1;
						++nonzeroes;

						// z_tmdw
						matind[nonzeroes] = startindex_z_tmdw + t * nb_zones * nb_days * nb_weeks + m * nb_days * nb_weeks + d * nb_weeks + w;
						matval[nonzeroes] = -1;
						++nonzeroes;

						if (nonzeroes >= maxnonzeroes)
							throw std::runtime_error("Error in function IP_model_allocation::build_problem(). Nonzeroes exceeds size of maxnonzeroes (matind and matval)");

						status = CPXaddrows(env, problem, 0, 1, nonzeroes, rhs, sense, matbeg, matind.get(), matval.get(), NULL, NULL);
						if (status != 0)
						{
							CPXgeterrorstring(env, status, error_text);
							throw std::runtime_error("Error in function IP_model_allocation::build_problem(). \nCouldn't add constraint. \nReason: " + std::string(error_text));
						}

						// change name of constraint
						std::string conname = "c4_" + std::to_string(t + 1) + "_" + std::to_string(m + 1) + "_" + std::to_string(d + 1) + "_" + std::to_string(w + 1);
						status = CPXchgname(env, problem, 'r', nb_constraints, conname.c_str());
						if (status != 0)
						{
							CPXgeterrorstring(env, status, error_text);
							throw std::runtime_error("Error in function IP_model_allocation::build_problem(). \nCouldn't change constraint name. \nReason: " + std::string(error_text));
						}
					}
				}
			}
		}

		// 5: y_tmdw + z_tmdw >= h_tmdw   forall t,m,d,w
		for (int t = 0; t < nb_types; ++t)
		{
			for (int m = 0; m < nb_zones; ++m)
			{
				for (int d = 0; d < nb_days; ++d)
				{
					for (int w = 0; w < nb_weeks; ++w)
					{
						++nb_constraints;

						const std::string& waste_type = data.waste_type(t);
						rhs[0] = data.current_calendar(m, waste_type, d, w); // convert bool to int

						sense[0] = 'G';
						matbeg[0] = 0;

						nonzeroes = 0;

						// y_tmdw
						matind[nonzeroes] = startindex_y_tmdw + t * nb_zones * nb_days * nb_weeks + m * nb_days * nb_weeks + d * nb_weeks + w;
						matval[nonzeroes] = 1;
						++nonzeroes;

						// z_tmdw
						matind[nonzeroes] = startindex_z_tmdw + t * nb_zones * nb_days * nb_weeks + m * nb_days * nb_weeks + d * nb_weeks + w;
						matval[nonzeroes] = 1;
						++nonzeroes;

						if (nonzeroes >= maxnonzeroes)
							throw std::runtime_error("Error in function IP_model_allocation::build_problem(). Nonzeroes exceeds size of maxnonzeroes (matind and matval)");

						status = CPXaddrows(env, problem, 0, 1, nonzeroes, rhs, sense, matbeg, matind.get(), matval.get(), NULL, NULL);
						if (status != 0)
						{
							CPXgeterrorstring(env, status, error_text);
							throw std::runtime_error("Error in function IP_model_allocation::build_problem(). \nCouldn't add constraint. \nReason: " + std::string(error_text));
						}

						// change name of constraint
						std::string conname = "c5_" + std::to_string(t + 1) + "_" + std::to_string(m + 1) + "_" + std::to_string(d + 1) + "_" + std::to_string(w + 1);
						status = CPXchgname(env, problem, 'r', nb_constraints, conname.c_str());
						if (status != 0)
						{
							CPXgeterrorstring(env, status, error_text);
							throw std::runtime_error("Error in function IP_model_allocation::build_problem(). \nCouldn't change constraint name. \nReason: " + std::string(error_text));
						}
					}
				}
			}
		}

		// 6: sum(t,m,d,w) z_tmdw <= pi*theta
		if(_fraction_allowed_deviations < 0.99)
		{
			++nb_constraints;

			int max_deviations = static_cast<int>(_fraction_allowed_deviations * data.nb_pickups_current_calendar() + 0.1);

			rhs[0] = max_deviations;
			sense[0] = 'L';
			matbeg[0] = 0;

			nonzeroes = 0;

			// z_tmdw
			for (int t = 0; t < nb_types; ++t)
			{
				for (int m = 0; m < nb_zones; ++m)
				{
					for (int d = 0; d < nb_days; ++d)
					{
						for (int w = 0; w < nb_weeks; ++w)
						{
							matind[nonzeroes] = startindex_z_tmdw + t * nb_zones * nb_days * nb_weeks + m * nb_days * nb_weeks + d * nb_weeks + w;
							matval[nonzeroes] = 1;
							++nonzeroes;
						}
					}
				}
			}

			if (nonzeroes >= maxnonzeroes)
				throw std::runtime_error("Error in function IP_model_allocation::build_problem(). Nonzeroes exceeds size of maxnonzeroes (matind and matval)");

			status = CPXaddrows(env, problem, 0, 1, nonzeroes, rhs, sense, matbeg, matind.get(), matval.get(), NULL, NULL);
			if (status != 0)
			{
				CPXgeterrorstring(env, status, error_text);
				throw std::runtime_error("Error in function IP_model_allocation::build_problem(). \nCouldn't add constraint. \nReason: " + std::string(error_text));
			}

			// change name of constraint
			std::string conname = "c6";
			status = CPXchgname(env, problem, 'r', nb_constraints, conname.c_str());
			if (status != 0)
			{
				CPXgeterrorstring(env, status, error_text);
				throw std::runtime_error("Error in function IP_model_allocation::build_problem(). \nCouldn't change constraint name. \nReason: " + std::string(error_text));
			}
		}

		// 7: x_tmdw - e_tdw <= A_tw
		for (int t = 0; t < nb_types; ++t)
		{
			for (int d = 0; d < nb_days; ++d)
			{
				for (int w = 0; w < nb_weeks; ++w)
				{
					++nb_constraints;

					// scenario
					double Atw = 0;
					//if (_scenario == FREE_WEEK_FREE_DAY)
					{
						for (int m = 0; m < nb_zones; ++m) {
							const std::string& waste_type = data.waste_type(t);
							Atw += data.demand(m, waste_type);
						}
						Atw /= (nb_days * nb_weeks);
					}
					/*else if (std::abs(t - w) == 1)
					{
						for (int m = 0; m < nb_zones; ++m) {
							const std::string& waste_type = data.waste_type(t);
							Atw += data.demand(m, waste_type);
						}
						Atw /= nb_days;
					}
					else
						Atw = 0;*/


					rhs[0] = Atw;
					sense[0] = 'L';
					matbeg[0] = 0;

					nonzeroes = 0;

					// x_tmdw
					for (int m = 0; m < nb_zones; ++m)
					{
						matind[nonzeroes] = startindex_x_tmdw + t * nb_zones * nb_days * nb_weeks + m * nb_days * nb_weeks + d * nb_weeks + w;
						matval[nonzeroes] = 1;
						++nonzeroes;
					}

					// e_tdw
					matind[nonzeroes] = startindex_e_tdw + t * nb_days * nb_weeks + d * nb_weeks + w;
					matval[nonzeroes] = -1;
					++nonzeroes;

					if (nonzeroes >= maxnonzeroes)
						throw std::runtime_error("Error in function IP_model_allocation::build_problem(). Nonzeroes exceeds size of maxnonzeroes (matind and matval)");

					status = CPXaddrows(env, problem, 0, 1, nonzeroes, rhs, sense, matbeg, matind.get(), matval.get(), NULL, NULL);
					if (status != 0)
					{
						CPXgeterrorstring(env, status, error_text);
						throw std::runtime_error("Error in function IP_model_allocation::build_problem(). \nCouldn't add constraint. \nReason: " + std::string(error_text));
					}

					// change name of constraint
					std::string conname = "c7_" + std::to_string(t + 1) + "_" + std::to_string(d + 1) + "_" + std::to_string(w + 1);
					status = CPXchgname(env, problem, 'r', nb_constraints, conname.c_str());
					if (status != 0)
					{
						CPXgeterrorstring(env, status, error_text);
						throw std::runtime_error("Error in function IP_model_allocation::build_problem(). \nCouldn't change constraint name. \nReason: " + std::string(error_text));
					}
				}
			}
		}

		// 8: x_tmdw + e_tdw >= A_tw
		for (int t = 0; t < nb_types; ++t)
		{
			for (int d = 0; d < nb_days; ++d)
			{
				for (int w = 0; w < nb_weeks; ++w)
				{
					++nb_constraints;

					// scenario
					double Atw = 0;
					//if (_scenario == FREE_WEEK_FREE_DAY) // altijd want vrije keuze
					{
						for (int m = 0; m < nb_zones; ++m) {
							const std::string& waste_type = data.waste_type(t);
							Atw += data.demand(m, waste_type);
						}
						Atw /= (nb_days * nb_weeks);
					}
					/*else if (std::abs(t - w) == 1)
					{
						for (int m = 0; m < nb_zones; ++m) {
							const std::string& waste_type = data.waste_type(t);
							Atw += data.demand(m, waste_type);
						}
						Atw /= nb_days;
					}
					else
						Atw = 0;*/


					rhs[0] = Atw;
					sense[0] = 'G';
					matbeg[0] = 0;

					nonzeroes = 0;

					// x_tmdw
					for (int m = 0; m < nb_zones; ++m)
					{
						matind[nonzeroes] = startindex_x_tmdw + t * nb_zones * nb_days * nb_weeks + m * nb_days * nb_weeks + d * nb_weeks + w;
						matval[nonzeroes] = 1;
						++nonzeroes;
					}

					// e_tdw
					matind[nonzeroes] = startindex_e_tdw + t * nb_days * nb_weeks + d * nb_weeks + w;
					matval[nonzeroes] = 1;
					++nonzeroes;

					if (nonzeroes >= maxnonzeroes)
						throw std::runtime_error("Error in function IP_model_allocation::build_problem(). Nonzeroes exceeds size of maxnonzeroes (matind and matval)");

					status = CPXaddrows(env, problem, 0, 1, nonzeroes, rhs, sense, matbeg, matind.get(), matval.get(), NULL, NULL);
					if (status != 0)
					{
						CPXgeterrorstring(env, status, error_text);
						throw std::runtime_error("Error in function IP_model_allocation::build_problem(). \nCouldn't add constraint. \nReason: " + std::string(error_text));
					}

					// change name of constraint
					std::string conname = "c8_" + std::to_string(t + 1) + "_" + std::to_string(d + 1) + "_" + std::to_string(w + 1);
					status = CPXchgname(env, problem, 'r', nb_constraints, conname.c_str());
					if (status != 0)
					{
						CPXgeterrorstring(env, status, error_text);
						throw std::runtime_error("Error in function IP_model_allocation::build_problem(). \nCouldn't change constraint name. \nReason: " + std::string(error_text));
					}
				}
			}
		}

		// Enkel indien scenario van toepassing is
		if (_scenario == FIXED_WEEK_SAME_DAY)
		{
			// 9. y_1,md,1 == y_2,md,2   forall m,d
			for (int m = 0; m < nb_zones; ++m)
			{
				for (int d = 0; d < nb_days; ++d)
				{
					++nb_constraints;

					rhs[0] = 0;
					sense[0] = 'E';
					matbeg[0] = 0;

					nonzeroes = 0;

					// y_1,md,1
					matind[nonzeroes] = startindex_y_tmdw + 0 * nb_zones * nb_days * nb_weeks + m * nb_days * nb_weeks + d * nb_weeks + 0;
					matval[nonzeroes] = 1;
					++nonzeroes;

					// y_2,md,2
					matind[nonzeroes] = startindex_y_tmdw + 1 * nb_zones * nb_days * nb_weeks + m * nb_days * nb_weeks + d * nb_weeks + 1;
					matval[nonzeroes] = -1;
					++nonzeroes;

					if (nonzeroes >= maxnonzeroes)
						throw std::runtime_error("Error in function IP_model_allocation::build_problem(). Nonzeroes exceeds size of maxnonzeroes (matind and matval)");

					status = CPXaddrows(env, problem, 0, 1, nonzeroes, rhs, sense, matbeg, matind.get(), matval.get(), NULL, NULL);
					if (status != 0)
					{
						CPXgeterrorstring(env, status, error_text);
						throw std::runtime_error("Error in function IP_model_allocation::build_problem(). \nCouldn't add constraint. \nReason: " + std::string(error_text));
					}

					// change name of constraint
					std::string conname = "c9_" + std::to_string(m + 1) + "_" + std::to_string(d + 1);
					status = CPXchgname(env, problem, 'r', nb_constraints, conname.c_str());
					if (status != 0)
					{
						CPXgeterrorstring(env, status, error_text);
						throw std::runtime_error("Error in function IP_model_allocation::build_problem(). \nCouldn't change constraint name. \nReason: " + std::string(error_text));
					}
				}
			}

			// 9bis. y_1,md,2 == y_2,md,1   forall m,d
			for (int m = 0; m < nb_zones; ++m)
			{
				for (int d = 0; d < nb_days; ++d)
				{
					++nb_constraints;

					rhs[0] = 0;
					sense[0] = 'E';
					matbeg[0] = 0;

					nonzeroes = 0;

					// y_1,md,1
					matind[nonzeroes] = startindex_y_tmdw + 0 * nb_zones * nb_days * nb_weeks + m * nb_days * nb_weeks + d * nb_weeks + 1;
					matval[nonzeroes] = 1;
					++nonzeroes;

					// y_2,md,2
					matind[nonzeroes] = startindex_y_tmdw + 1 * nb_zones * nb_days * nb_weeks + m * nb_days * nb_weeks + d * nb_weeks + 0;
					matval[nonzeroes] = -1;
					++nonzeroes;

					if (nonzeroes >= maxnonzeroes)
						throw std::runtime_error("Error in function IP_model_allocation::build_problem(). Nonzeroes exceeds size of maxnonzeroes (matind and matval)");

					status = CPXaddrows(env, problem, 0, 1, nonzeroes, rhs, sense, matbeg, matind.get(), matval.get(), NULL, NULL);
					if (status != 0)
					{
						CPXgeterrorstring(env, status, error_text);
						throw std::runtime_error("Error in function IP_model_allocation::build_problem(). \nCouldn't add constraint. \nReason: " + std::string(error_text));
					}

					// change name of constraint
					std::string conname = "c9bis_" + std::to_string(m + 1) + "_" + std::to_string(d + 1);
					status = CPXchgname(env, problem, 'r', nb_constraints, conname.c_str());
					if (status != 0)
					{
						CPXgeterrorstring(env, status, error_text);
						throw std::runtime_error("Error in function IP_model_allocation::build_problem(). \nCouldn't change constraint name. \nReason: " + std::string(error_text));
					}
				}
			}
		}

		// Enkel indien scenario van toepassing is
		if (_scenario == FIXED_WEEK_FREE_DAY)
		{
			// Dit veronderstelt dat max 1 bezoek per week, anders extra variabele nodig
			// 10. sum(t,d) y_tmdw <= 1   forall(w,m)
			for (int m = 0; m < nb_zones; ++m)
			{
				for (int w = 0; w < nb_weeks; ++w)
				{
					++nb_constraints;

					rhs[0] = 1;
					sense[0] = 'L';
					matbeg[0] = 0;

					nonzeroes = 0;

					// sum(t,d) y_tmdw
					for (int t = 0; t < nb_types; ++t)
					{
						for (int d = 0; d < nb_days; ++d)
						{
							matind[nonzeroes] = startindex_y_tmdw + t * nb_zones * nb_days * nb_weeks + m * nb_days * nb_weeks + d * nb_weeks + w;
							matval[nonzeroes] = 1;
							++nonzeroes;
						}
					}

					if (nonzeroes >= maxnonzeroes)
						throw std::runtime_error("Error in function IP_model_allocation::build_problem(). Nonzeroes exceeds size of maxnonzeroes (matind and matval)");

					status = CPXaddrows(env, problem, 0, 1, nonzeroes, rhs, sense, matbeg, matind.get(), matval.get(), NULL, NULL);
					if (status != 0)
					{
						CPXgeterrorstring(env, status, error_text);
						throw std::runtime_error("Error in function IP_model_allocation::build_problem(). \nCouldn't add constraint. \nReason: " + std::string(error_text));
					}

					// change name of constraint
					std::string conname = "c10_" + std::to_string(m + 1) + "_" + std::to_string(w + 1);
					status = CPXchgname(env, problem, 'r', nb_constraints, conname.c_str());
					if (status != 0)
					{
						CPXgeterrorstring(env, status, error_text);
						throw std::runtime_error("Error in function IP_model_allocation::build_problem(). \nCouldn't change constraint name. \nReason: " + std::string(error_text));
					}
				}
			}
		}

		// Enkel indien scenario van toepassing is
		if (_scenario == CURRENT_CALENDAR) // elke week restafval
		{
			// 12. x_tmd0 == x_tmd1
			for (int t = 0; t < nb_types; ++t)
			{
				for (int m = 0; m < nb_zones; ++m)
				{
					for (int d = 0; d < nb_days; ++d)
					{
						++nb_constraints;

						rhs[0] = 0;
						sense[0] = 'E';
						matbeg[0] = 0;

						nonzeroes = 0;

						// x_tmd0
						matind[nonzeroes] = startindex_x_tmdw + t * nb_zones * nb_days * nb_weeks + m * nb_days * nb_weeks + d * nb_weeks + 0;
						matval[nonzeroes] = 1;
						++nonzeroes;

						// x_tmd1
						matind[nonzeroes] = startindex_x_tmdw + t * nb_zones * nb_days * nb_weeks + m * nb_days * nb_weeks + d * nb_weeks + 1;
						matval[nonzeroes] = -1;
						++nonzeroes;

						if (nonzeroes >= maxnonzeroes)
							throw std::runtime_error("Error in function IP_model_allocation::build_problem(). Nonzeroes exceeds size of maxnonzeroes (matind and matval)");

						status = CPXaddrows(env, problem, 0, 1, nonzeroes, rhs, sense, matbeg, matind.get(), matval.get(), NULL, NULL);
						if (status != 0)
						{
							CPXgeterrorstring(env, status, error_text);
							throw std::runtime_error("Error in function IP_model_allocation::build_problem(). \nCouldn't add constraint. \nReason: " + std::string(error_text));
						}

						// change name of constraint
						std::string conname = "c12_" + std::to_string(t + 1) + "_" + std::to_string(m + 1) + "_" + std::to_string(d + 1);
						status = CPXchgname(env, problem, 'r', nb_constraints, conname.c_str());
						if (status != 0)
						{
							CPXgeterrorstring(env, status, error_text);
							throw std::runtime_error("Error in function IP_model_allocation::build_problem(). \nCouldn't change constraint name. \nReason: " + std::string(error_text));
						}
					}
				}
			}
		}



		// write to file
		status = CPXwriteprob(env, problem, "IP_model_allocation.lp", NULL);
		if (status != 0)
		{
			CPXgeterrorstring(env, status, error_text);
			throw std::runtime_error("Error in function IP_model_allocation::build_problem(). \nCouldn't write problem to lp-file. \nReason: " + std::string(error_text));
		}
	}

	void IP_model_allocation::solve_problem(const Instance& data)
	{
		char error_text[CPXMESSAGEBUFSIZE];
		int status = 0;
		int solstat = 0;
		std::unique_ptr<double[]> solution_problem;
		double objval;

		// Set allowed computation time
		status = CPXsetdblparam(env, CPXPARAM_TimeLimit, _max_computation_time);
		if (status != 0)
		{
			CPXgeterrorstring(env, status, error_text);
			throw std::runtime_error("Error in function IP_model_allocation::solve_problem(). \nCouldn't set time limit. \nReason: " + std::string(error_text));
		}

		// Assign memory for solution
		solution_problem = std::make_unique<double[]>(CPXgetnumcols(env, problem));


		// Optimize the problem
		std::cout << "\n\nSolving the allocation problem ...";
		auto start_time = std::chrono::system_clock::now();

		status = CPXmipopt(env, problem);
		if (status != 0)
		{
			CPXgeterrorstring(env, status, error_text);
			throw std::runtime_error("Error in function IP_model_allocation::solve_problem(). \nCPXmipopt failed. \nReason: " + std::string(error_text));
		}

		std::chrono::duration<double, std::ratio<1, 1>> elapsed_time_IP = std::chrono::system_clock::now() - start_time;


		// Get the solution
		status = CPXsolution(env, problem, &solstat, &objval, solution_problem.get(), NULL, NULL, NULL);
		if (status != 0)
		{
			CPXgeterrorstring(env, status, error_text);
			throw std::runtime_error("Error in function IP_model_allocation::solve_problem(). \nCPXsolution failed. \nReason: " + std::string(error_text));
		}

		char solstat_text[CPXMESSAGEBUFSIZE];
		auto p = CPXgetstatstring(env, solstat, solstat_text);
		if (p != nullptr)
		{
			std::cout << "\nSolution status: " << solstat_text;

			if (solstat == CPXMIP_OPTIMAL || solstat == CPXMIP_OPTIMAL_TOL || solstat == CPXMIP_TIME_LIM_FEAS)
			{
				_objective_value = objval;
				std::cout << "\nObjective value = " << objval;
				std::cout << "\nElapsed time (s): " << elapsed_time_IP.count();

				const size_t nb_waste_types = data.nb_waste_types();
				const size_t nb_zones = data.nb_zones();
				const size_t nb_days = data.nb_days();
				const size_t nb_weeks = data.nb_weeks();

				const int startindex_x_tmdw = 0;
				const int startindex_y_tmdw = startindex_x_tmdw + nb_waste_types * nb_zones * nb_days * nb_weeks;
				const int startindex_z_tmdw = startindex_y_tmdw + nb_waste_types * nb_zones * nb_days * nb_weeks;
				const int startindex_e_tdw = startindex_z_tmdw + nb_waste_types * nb_zones * nb_days * nb_weeks;

				// lambdas to get variable indices
				auto index_x_tmdw = [startindex_x_tmdw, nb_zones, nb_days, nb_weeks](int t, int m, int d, int w) -> int {
					return startindex_x_tmdw + t * nb_zones * nb_days * nb_weeks + m * nb_days * nb_weeks + d * nb_weeks + w;
					};
				auto index_y_tmdw = [startindex_y_tmdw, nb_zones, nb_days, nb_weeks](int t, int m, int d, int w) -> int {
					return startindex_y_tmdw + t * nb_zones * nb_days * nb_weeks + m * nb_days * nb_weeks + d * nb_weeks + w;
					};
				auto index_z_tmdw = [startindex_z_tmdw, nb_zones, nb_days, nb_weeks](int t, int m, int d, int w) -> int {
					return startindex_z_tmdw + t * nb_zones * nb_days * nb_weeks + m * nb_days * nb_weeks + d * nb_weeks + w;
					};
				auto index_e_tdw = [startindex_e_tdw, nb_days, nb_weeks](int t, int d, int w) -> int {
					return startindex_e_tdw + t * nb_days * nb_weeks + d * nb_weeks + w;
					};



				// Write solution to file
				{
					std::ofstream solfile;
					std::string filename = data.name_instance() + "_allocatie.txt";
					solfile.open(filename);

					solfile << "Instantie: " << data.name_instance() << "\n"
						<< "\nScenario: " << scenario_name()
						<< "\nMax pct. verandering: " << _fraction_allowed_deviations
						<< "\nMax rekentijd (s): " << _max_computation_time
						<< "\n\nDoelfunctiewaarde = " << objval;

					solfile << "\n\nx_tmdw  (opgehaalde hoeveelheid)";
					for (int t = 0; t < nb_waste_types; ++t) {
						for (int m = 0; m < nb_zones; ++m) {
							for (int d = 0; d < nb_days; ++d) {
								for (int w = 0; w < nb_weeks; ++w) {
									int index_var = index_x_tmdw(t, m, d, w);
									double value = solution_problem[index_var];
									if (value > 0.000001) {
										solfile << "\n" << data.waste_type(t) << ", " << data.zone_name(m) << ", " << data.day_name(d) << ", week " << w + 1 << ": x = " << value;
									}
								}
							}
						}
					}
					solfile << "\n\ny_tmdw  (wel of niet opgehaald)";
					for (int t = 0; t < nb_waste_types; ++t) {
						for (int m = 0; m < nb_zones; ++m) {
							for (int d = 0; d < nb_days; ++d) {
								for (int w = 0; w < nb_weeks; ++w) {
									int index_var = index_y_tmdw(t, m, d, w);
									int value = static_cast<int>(solution_problem[index_var] + 0.01);
									if (value > 0) {
										solfile << "\n" << data.waste_type(t) << ", " << data.zone_name(m) << ", " << data.day_name(d) << ", week " << w + 1 << ": y = " << value;
									}
								}
							}
						}
					}
					solfile << "\n\nz_tmdw  (andere ophaaldag dan huidig)";
					for (int t = 0; t < nb_waste_types; ++t) {
						for (int m = 0; m < nb_zones; ++m) {
							for (int d = 0; d < nb_days; ++d) {
								for (int w = 0; w < nb_weeks; ++w) {
									int index_var = index_z_tmdw(t, m, d, w);
									int value = static_cast<int>(solution_problem[index_var] + 0.01);
									if (value > 0) {
										solfile << "\n" << data.waste_type(t) << ", " << data.zone_name(m) << ", " << data.day_name(d) << ", week " << w + 1 << ": z = " << value;
									}
								}
							}
						}
					}
					solfile << "\n\ne_tdw  (afwijkingen doelfunctie)";
					for (int t = 0; t < nb_waste_types; ++t) {
						for (int d = 0; d < nb_days; ++d) {
							for (int w = 0; w < nb_weeks; ++w) {
								int index_var = index_e_tdw(t, d, w);
								double value = solution_problem[index_var];
								if (value > 0.000001) {
									solfile << "\n" << data.waste_type(t) << ", " << data.day_name(d) << ", week " << w + 1 << ": e = " << value;
								}
							}
						}
					}

					// andere layout
					for (int t = 0; t < nb_waste_types; ++t) {
						solfile << "\n\n\n\nKalender " << data.waste_type(t) << "\nZone\tMaandag\tDinsdag\tWoensdag\tDonderdag\tVrijdag\tMaandag\tDinsdag\tWoensdag\tDonderdag\tVrijdag";
						for (int m = 0; m < nb_zones; ++m) {
							solfile << "\n" << data.zone_name(m);
							for (int w = 0; w < nb_weeks; ++w) {
								for (int d = 0; d < nb_days; ++d) {
									solfile << "\t";

									int index_var = index_x_tmdw(t, m, d, w);
									double value = solution_problem[index_var];
									if (value > 0.000001) {
										solfile << value;
									}
								}
							}
						}
					}

					solfile.flush();
				}


				// Write output to XML-file
				{
					std::ofstream solfile;
					std::string filename = "oplossing_allocatie.xml";
					solfile.open(filename);

					solfile << "<?xml version=\"1.0\"?>"
						<< "\n<Allocatie instantie=\"" << data.name_instance() << "\""
						<< " scenario=\"" << scenario_name() << "\""
						<< " max_pct_veranderingen=\"" << _fraction_allowed_deviations << "\""
						<< " max_rekentijd=\"" << _max_computation_time << "\">";

					// Ophalingen
					for (int t = 0; t < nb_waste_types; ++t) {
						for (int m = 0; m < nb_zones; ++m) {
							for (int d = 0; d < nb_days; ++d) {
								for (int w = 0; w < nb_weeks; ++w) {
									int index_var = index_x_tmdw(t, m, d, w);
									double value = solution_problem[index_var];
									if (value > 0.000001) {
										solfile << "\n\t<Ophaling afval_type=\"" << data.waste_type(t) << "\""
											<< " zone=\"" << data.zone_name(m) << "\""
											<< " dag=\"" << data.day_name(d) << "\""
											<< " week=\"" << w + 1 << "\""
											<< " hoeveelheid=\"" << value << "\"/>";
									}
								}
							}
						}
					}

					// Algemeen
					solfile << "\n</Allocatie>";
				}
			}
		}
	}

	void IP_model_allocation::clear_cplex()
	{
		int status = 0;
		char error_text[CPXMESSAGEBUFSIZE];

		// Free the problem
		status = CPXfreeprob(env, &problem);
		if (status != 0)
		{
			CPXgeterrorstring(env, status, error_text);
			throw std::runtime_error("Error in function IP_model_allocation::clear_cplex(). \nCouldn't free problem. \nReason: " + std::string(error_text));
		}

		// Close the cplex environment
		status = CPXcloseCPLEX(&env);
		if (status != 0)
		{
			CPXgeterrorstring(env, status, error_text);
			throw std::runtime_error("Error in function IP_model_allocation::clear_cplex(). \nCouldn't close cplex environment. \nReason: " + std::string(error_text));
		}
	}

	void IP_model_allocation::run(const Instance& data)
	{
		initialize_cplex();
		build_problem(data);
		solve_problem(data);
		clear_cplex();
	}

	const std::string IP_model_allocation::scenario_name() const
	{
		if (_scenario == Scenario::FIXED_WEEK_SAME_DAY)
			return "vaste week en zelfde dag";
		else if (_scenario == Scenario::FIXED_WEEK_FREE_DAY)
			return "vaste week en vrije dag";
		else if (_scenario == Scenario::FREE_WEEK_FREE_DAY)
			return "vrije week en vrije dag";
		else if (_scenario == Scenario::CURRENT_CALENDAR)
			return "huidige kalender met wekelijkse ophaling restafval";
	}

	///////////////////////////////////////////////////////////////////////////////////////////////
}
