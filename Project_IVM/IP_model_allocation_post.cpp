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
	///		 IP Model Allocation Post		///
	///////////////////////////////////////////

	void IP_model_allocation_post::initialize_cplex()
	{
		int status = 0;
		char error_text[CPXMESSAGEBUFSIZE];

		// open the CPLEX environment
		env = CPXopenCPLEX(&status);
		if (status != 0)
		{
			CPXgeterrorstring(env, status, error_text);
			throw std::runtime_error("Error in function IP_model_allocation_post::initialize_cplex(). \nCouldn't open CPLEX. \nReason: " + std::string(error_text));
		}

		// turn output to screen on/off
		if (_output_solver)
			status = CPXsetintparam(env, CPXPARAM_ScreenOutput, CPX_ON);
		else
			status = CPXsetintparam(env, CPXPARAM_ScreenOutput, CPX_OFF);
		if (status != 0)
		{
			CPXgeterrorstring(env, status, error_text);
			throw std::runtime_error("Error in function IP_model_allocation_post::initialize_cplex(). \nCouldn't change param SCRIND. \nReason: " + std::string(error_text));
		}
	}

	void IP_model_allocation_post::build_problem(const Instance& data)
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
		problem = CPXcreateprob(env, &status, "IP_model_allocation_post");
		if (status != 0)
		{
			CPXgeterrorstring(env, status, error_text);
			throw std::runtime_error("Error in function IP_model_allocation_post::build_problem(). \nCouldn't create problem. \nReason: " + std::string(error_text));
		}

		// problem is minimization
		status = CPXchgobjsen(env, problem, CPX_MIN);
		if (status != 0)
		{
			CPXgeterrorstring(env, status, error_text);
			throw std::runtime_error("Error in function IP_model_allocation_post::build_problem(). \nCouldn't change objective to minimization. \nReason: " + std::string(error_text));
		}


		// data 
		const size_t nb_routes = data.nb_routes();
		const size_t nb_types = data.nb_waste_types();
		const size_t nb_zones = data.nb_zones();
		const size_t nb_days = data.nb_days();
		const size_t nb_weeks = data.nb_weeks();


		// add variables
		int nb_variables = -1;

		// variable x_rdw
		const int startindex_x_rdw = 0;
		for(int r = 0; r < nb_routes; ++r)
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
						throw std::runtime_error("Error in function IP_model_allocation_post::build_problem(). \nCouldn't add variable. \nReason: " + std::string(error_text));
					}

					// change variable name
					std::string varname = "x_" + std::to_string(r + 1) + "_" + std::to_string(d + 1) + "_" + std::to_string(w + 1);
					status = CPXchgname(env, problem, 'c', nb_variables, varname.c_str());
					if (status != 0)
					{
						CPXgeterrorstring(env, status, error_text);
						throw std::runtime_error("Error in function IP_model_allocation_post::build_problem(). \nCouldn't change variable name. \nReason: " + std::string(error_text));
					}
				}
			}
		}

		// variable y_tmdw
		const int startindex_y_tmdw = startindex_x_rdw + nb_routes * nb_days * nb_weeks;
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
							throw std::runtime_error("Error in function IP_model_allocation_post::build_problem(). \nCouldn't add variable. \nReason: " + std::string(error_text));
						}

						// change variable name
						std::string varname = "y_" + std::to_string(t + 1) + "_" + std::to_string(m + 1) + "_" + std::to_string(d + 1) + "_" + std::to_string(w + 1);
						status = CPXchgname(env, problem, 'c', nb_variables, varname.c_str());
						if (status != 0)
						{
							CPXgeterrorstring(env, status, error_text);
							throw std::runtime_error("Error in function IP_model_allocation_post::build_problem(). \nCouldn't change variable name. \nReason: " + std::string(error_text));
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

						obj[0] = _objcoeff_z_tmdw;
						lb[0] = 0;
						ub[0] = 1;
						type[0] = 'B';

						status = CPXnewcols(env, problem, 1, obj, lb, ub, type, NULL);
						if (status != 0)
						{
							CPXgeterrorstring(env, status, error_text);
							throw std::runtime_error("Error in function IP_model_allocation_post::build_problem(). \nCouldn't add variable. \nReason: " + std::string(error_text));
						}

						// change variable name
						std::string varname = "z_" + std::to_string(t + 1) + "_" + std::to_string(m + 1) + "_" + std::to_string(d + 1) + "_" + std::to_string(w + 1);
						status = CPXchgname(env, problem, 'c', nb_variables, varname.c_str());
						if (status != 0)
						{
							CPXgeterrorstring(env, status, error_text);
							throw std::runtime_error("Error in function IP_model_allocation_post::build_problem(). \nCouldn't change variable name. \nReason: " + std::string(error_text));
						}
					}
				}
			}
		}

		// variabele beta
		const int startindex_beta = startindex_z_tmdw + nb_types * nb_zones * nb_days * nb_weeks;
		{
			++nb_variables;

			obj[0] = _objcoeff_beta;
			lb[0] = 0;
			type[0] = 'I';

			status = CPXnewcols(env, problem, 1, obj, lb, NULL, type, NULL);
			if (status != 0)
			{
				CPXgeterrorstring(env, status, error_text);
				throw std::runtime_error("Error in function IP_model_allocation_post::build_problem(). \nCouldn't add variable. \nReason: " + std::string(error_text));
			}

			// change variable name
			std::string varname = "beta";
			status = CPXchgname(env, problem, 'c', nb_variables, varname.c_str());
			if (status != 0)
			{
				CPXgeterrorstring(env, status, error_text);
				throw std::runtime_error("Error in function IP_model_allocation_post::build_problem(). \nCouldn't change variable name. \nReason: " + std::string(error_text));
			}
		}

		// variable theta_tm or theta_r
		const int startindex_theta = startindex_beta + 1;
		if (_penalty_on_route_assignment)
		{
			for (int r = 0; r < nb_routes; ++r)
			{
				++nb_variables;

				obj[0] = _objcoeff_theta * data.route_nb_times_used(r);
				lb[0] = 0;
				type[0] = 'I';

				status = CPXnewcols(env, problem, 1, obj, lb, NULL, type, NULL);
				if (status != 0)
				{
					CPXgeterrorstring(env, status, error_text);
					throw std::runtime_error("Error in function IP_model_allocation_post::build_problem(). \nCouldn't add variable. \nReason: " + std::string(error_text));
				}

				// change variable name
				std::string varname = "theta_" + std::to_string(r + 1);
				status = CPXchgname(env, problem, 'c', nb_variables, varname.c_str());
				if (status != 0)
				{
					CPXgeterrorstring(env, status, error_text);
					throw std::runtime_error("Error in function IP_model_allocation_post::build_problem(). \nCouldn't change variable name. \nReason: " + std::string(error_text));
				}
			}
		}
		else
		{
			for (int t = 0; t < nb_types; ++t)
			{
				for (int m = 0; m < nb_zones; ++m)
				{
					++nb_variables;

					obj[0] = _objcoeff_theta;
					lb[0] = 0;
					type[0] = 'I';

					status = CPXnewcols(env, problem, 1, obj, lb, NULL, type, NULL);
					if (status != 0)
					{
						CPXgeterrorstring(env, status, error_text);
						throw std::runtime_error("Error in function IP_model_allocation_post::build_problem(). \nCouldn't add variable. \nReason: " + std::string(error_text));
					}

					// change variable name
					std::string varname = "theta_" + std::to_string(t + 1) + "_" + std::to_string(m + 1);
					status = CPXchgname(env, problem, 'c', nb_variables, varname.c_str());
					if (status != 0)
					{
						CPXgeterrorstring(env, status, error_text);
						throw std::runtime_error("Error in function IP_model_allocation_post::build_problem(). \nCouldn't change variable name. \nReason: " + std::string(error_text));
					}
				}
			}
		}


		// lambdas to get variable indices
		auto index_x_rdw = [startindex_x_rdw, nb_days, nb_weeks](int r, int d, int w) -> int {
			return startindex_x_rdw + r * nb_days * nb_weeks + d * nb_weeks + w;
			};
		auto index_y_tmdw = [startindex_y_tmdw, nb_zones, nb_days, nb_weeks](int t, int m, int d, int w) -> int {
			return startindex_y_tmdw + t * nb_zones * nb_days * nb_weeks + m * nb_days * nb_weeks + d * nb_weeks + w;
			};
		auto index_z_tmdw = [startindex_z_tmdw, nb_zones, nb_days, nb_weeks](int t, int m, int d, int w) -> int {
			return startindex_z_tmdw + t * nb_zones * nb_days * nb_weeks + m * nb_days * nb_weeks + d * nb_weeks + w;
			};
		auto index_theta_tm = [startindex_theta, nb_zones](int t, int m) -> int {
			return startindex_theta + t * nb_zones + m;
			};
		auto index_theta_r = [startindex_theta](int r) -> int {
			return startindex_theta + r;
			};


		// add constraints
		int nb_constraints = -1;

		// 1: sum(d,w) x_rdw == 1 - theta_r   forall r
		for (int r = 0; r < nb_routes; ++r)
		{
			++nb_constraints;

			rhs[0] = 1;
			sense[0] = 'E';
			matbeg[0] = 0;

			nonzeroes = 0;

			// x_tmdw
			for (int d = 0; d < nb_days; ++d)
			{
				for (int w = 0; w < nb_weeks; ++w)
				{
					matind[nonzeroes] = index_x_rdw(r, d, w);
					matval[nonzeroes] = 1;
					++nonzeroes;
				}
			}

			if (_penalty_on_route_assignment)
			{
				// theta_r
				matind[nonzeroes] = index_theta_r(r);
				matval[nonzeroes] = 1;
				++nonzeroes;
			}

			if (nonzeroes >= maxnonzeroes)
				throw std::runtime_error("Error in function IP_model_allocation_post::build_problem(). Nonzeroes exceeds size of maxnonzeroes (matind and matval)");

			status = CPXaddrows(env, problem, 0, 1, nonzeroes, rhs, sense, matbeg, matind.get(), matval.get(), NULL, NULL);
			if (status != 0)
			{
				CPXgeterrorstring(env, status, error_text);
				throw std::runtime_error("Error in function IP_model_allocation_post::build_problem(). \nCouldn't add constraint. \nReason: " + std::string(error_text));
			}

			// change name of constraint
			std::string conname = "c1_" + std::to_string(r + 1);
			status = CPXchgname(env, problem, 'r', nb_constraints, conname.c_str());
			if (status != 0)
			{
				CPXgeterrorstring(env, status, error_text);
				throw std::runtime_error("Error in function IP_model_allocation_post::build_problem(). \nCouldn't change constraint name. \nReason: " + std::string(error_text));
			}
		}

		// 2: sum(r) n_r x_rdw <= beta   forall d,w
		for (int d = 0; d < nb_days; ++d)
		{
			for (int w = 0; w < nb_weeks; ++w)
			{
				++nb_constraints;

				rhs[0] = 0;
				sense[0] = 'L';
				matbeg[0] = 0;

				nonzeroes = 0;

				// x_rdw
				for (int r = 0; r < nb_routes; ++r)
				{
					matind[nonzeroes] = index_x_rdw(r, d, w);
					matval[nonzeroes] = data.route_nb_times_used(r);
					++nonzeroes;
				}

				// beta
				{
					matind[nonzeroes] = startindex_beta;
					matval[nonzeroes] = -1;
					++nonzeroes;
				}

				if (nonzeroes >= maxnonzeroes)
					throw std::runtime_error("Error in function IP_model_allocation_post::build_problem(). Nonzeroes exceeds size of maxnonzeroes (matind and matval)");

				status = CPXaddrows(env, problem, 0, 1, nonzeroes, rhs, sense, matbeg, matind.get(), matval.get(), NULL, NULL);
				if (status != 0)
				{
					CPXgeterrorstring(env, status, error_text);
					throw std::runtime_error("Error in function IP_model_allocation_post::build_problem(). \nCouldn't add constraint. \nReason: " + std::string(error_text));
				}

				// change name of constraint
				std::string conname = "c2_" + std::to_string(d + 1) + "_" + std::to_string(w + 1);
				status = CPXchgname(env, problem, 'r', nb_constraints, conname.c_str());
				if (status != 0)
				{
					CPXgeterrorstring(env, status, error_text);
					throw std::runtime_error("Error in function IP_model_allocation_post::build_problem(). \nCouldn't change constraint name. \nReason: " + std::string(error_text));
				}
			}
		}

		// 3: a_rm b_rt x_rdw <= y_tmdw   forall r,t,m,d,w
		for (int r = 0; r < nb_routes; ++r)
		{
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

							// x_rdw
							{
								const std::string& waste_type = data.waste_type(t);
								matind[nonzeroes] = index_x_rdw(r, d, w);
								matval[nonzeroes] = data.route_visits_zone(r, m) * data.route_waste_type(r, waste_type); // bool to int
								++nonzeroes;
							}

							// y_tmdw
							{
								matind[nonzeroes] = index_y_tmdw(t, m, d, w);
								matval[nonzeroes] = -1;
								++nonzeroes;
							}

							if (nonzeroes >= maxnonzeroes)
								throw std::runtime_error("Error in function IP_model_allocation_post::build_problem(). Nonzeroes exceeds size of maxnonzeroes (matind and matval)");

							status = CPXaddrows(env, problem, 0, 1, nonzeroes, rhs, sense, matbeg, matind.get(), matval.get(), NULL, NULL);
							if (status != 0)
							{
								CPXgeterrorstring(env, status, error_text);
								throw std::runtime_error("Error in function IP_model_allocation_post::build_problem(). \nCouldn't add constraint. \nReason: " + std::string(error_text));
							}

							// change name of constraint
							std::string conname = "c3_" + std::to_string(t + 1) + "_" + std::to_string(m + 1);
							status = CPXchgname(env, problem, 'r', nb_constraints, conname.c_str());
							if (status != 0)
							{
								CPXgeterrorstring(env, status, error_text);
								throw std::runtime_error("Error in function IP_model_allocation_post::build_problem(). \nCouldn't change constraint name. \nReason: " + std::string(error_text));
							}
						}
					}
				}
			}
		}

		// 4: y_tmdw <= V_md   forall t,m,d,w
		for (int t = 0; t < nb_types; ++t)
		{
			for (int m = 0; m < nb_zones; ++m)
			{
				for (int d = 0; d < nb_days; ++d)
				{
					for (int w = 0; w < nb_weeks; ++w)
					{
						++nb_constraints;

						rhs[0] = !data.zone_forbidden_day(m,d); // bool to int
						sense[0] = 'L';
						matbeg[0] = 0;

						nonzeroes = 0;

						// y_tmdw
						{
							matind[nonzeroes] = index_y_tmdw(t, m, d, w);
							matval[nonzeroes] = 1;
							++nonzeroes;
						}

						if (nonzeroes >= maxnonzeroes)
							throw std::runtime_error("Error in function IP_model_allocation_post::build_problem(). Nonzeroes exceeds size of maxnonzeroes (matind and matval)");

						status = CPXaddrows(env, problem, 0, 1, nonzeroes, rhs, sense, matbeg, matind.get(), matval.get(), NULL, NULL);
						if (status != 0)
						{
							CPXgeterrorstring(env, status, error_text);
							throw std::runtime_error("Error in function IP_model_allocation_post::build_problem(). \nCouldn't add constraint. \nReason: " + std::string(error_text));
						}

						// change name of constraint
						std::string conname = "c4_" + std::to_string(t + 1) + "_" + std::to_string(m + 1) +"_" + std::to_string(d + 1) + "_" + std::to_string(w + 1);
						status = CPXchgname(env, problem, 'r', nb_constraints, conname.c_str());
						if (status != 0)
						{
							CPXgeterrorstring(env, status, error_text);
							throw std::runtime_error("Error in function IP_model_allocation_post::build_problem(). \nCouldn't change constraint name. \nReason: " + std::string(error_text));
						}
					}
				}
			}
		}

		// 5: sum(d,w) y_tmdw - theta_tm <= W   forall t,m
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
						matind[nonzeroes] = index_y_tmdw(t, m, d, w);
						matval[nonzeroes] = 1;
						++nonzeroes;
					}
				}

				// theta_tm
				if(!_penalty_on_route_assignment)
				{
					matind[nonzeroes] = index_theta_tm(t, m);
					matval[nonzeroes] = -1;
					++nonzeroes;
				}

				if (nonzeroes >= maxnonzeroes)
					throw std::runtime_error("Error in function IP_model_allocation_post::build_problem(). Nonzeroes exceeds size of maxnonzeroes (matind and matval)");

				status = CPXaddrows(env, problem, 0, 1, nonzeroes, rhs, sense, matbeg, matind.get(), matval.get(), NULL, NULL);
				if (status != 0)
				{
					CPXgeterrorstring(env, status, error_text);
					throw std::runtime_error("Error in function IP_model_allocation_post::build_problem(). \nCouldn't add constraint. \nReason: " + std::string(error_text));
				}

				// change name of constraint
				std::string conname = "c5_" + std::to_string(t + 1) + "_" + std::to_string(m + 1);
				status = CPXchgname(env, problem, 'r', nb_constraints, conname.c_str());
				if (status != 0)
				{
					CPXgeterrorstring(env, status, error_text);
					throw std::runtime_error("Error in function IP_model_allocation_post::build_problem(). \nCouldn't change constraint name. \nReason: " + std::string(error_text));
				}
			}
		}

		// 6: y_tmdw - z_tmdw <= h_tmdw   forall t,m,d,w
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
						matind[nonzeroes] = index_y_tmdw(t, m, d, w);
						matval[nonzeroes] = 1;
						++nonzeroes;

						// z_tmdw
						matind[nonzeroes] = index_z_tmdw(t, m, d, w);
						matval[nonzeroes] = -1;
						++nonzeroes;

						if (nonzeroes >= maxnonzeroes)
							throw std::runtime_error("Error in function IP_model_allocation_post::build_problem(). Nonzeroes exceeds size of maxnonzeroes (matind and matval)");

						status = CPXaddrows(env, problem, 0, 1, nonzeroes, rhs, sense, matbeg, matind.get(), matval.get(), NULL, NULL);
						if (status != 0)
						{
							CPXgeterrorstring(env, status, error_text);
							throw std::runtime_error("Error in function IP_model_allocation_post::build_problem(). \nCouldn't add constraint. \nReason: " + std::string(error_text));
						}

						// change name of constraint
						std::string conname = "c6_" + std::to_string(t + 1) + "_" + std::to_string(m + 1) + "_" + std::to_string(d + 1) + "_" + std::to_string(w + 1);
						status = CPXchgname(env, problem, 'r', nb_constraints, conname.c_str());
						if (status != 0)
						{
							CPXgeterrorstring(env, status, error_text);
							throw std::runtime_error("Error in function IP_model_allocation_post::build_problem(). \nCouldn't change constraint name. \nReason: " + std::string(error_text));
						}
					}
				}
			}
		}

		// 7: y_tmdw + z_tmdw >= h_tmdw   forall t,m,d,w
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
						matind[nonzeroes] = index_y_tmdw(t, m, d, w);
						matval[nonzeroes] = 1;
						++nonzeroes;

						// z_tmdw
						matind[nonzeroes] = index_z_tmdw(t, m, d, w);
						matval[nonzeroes] = 1;
						++nonzeroes;

						if (nonzeroes >= maxnonzeroes)
							throw std::runtime_error("Error in function IP_model_allocation_post::build_problem(). Nonzeroes exceeds size of maxnonzeroes (matind and matval)");

						status = CPXaddrows(env, problem, 0, 1, nonzeroes, rhs, sense, matbeg, matind.get(), matval.get(), NULL, NULL);
						if (status != 0)
						{
							CPXgeterrorstring(env, status, error_text);
							throw std::runtime_error("Error in function IP_model_allocation_post::build_problem(). \nCouldn't add constraint. \nReason: " + std::string(error_text));
						}

						// change name of constraint
						std::string conname = "c7_" + std::to_string(t + 1) + "_" + std::to_string(m + 1) + "_" + std::to_string(d + 1) + "_" + std::to_string(w + 1);
						status = CPXchgname(env, problem, 'r', nb_constraints, conname.c_str());
						if (status != 0)
						{
							CPXgeterrorstring(env, status, error_text);
							throw std::runtime_error("Error in function IP_model_allocation_post::build_problem(). \nCouldn't change constraint name. \nReason: " + std::string(error_text));
						}
					}
				}
			}
		}

		// Enkel indien scenario van toepassing is
		if (_scenario == FIXED_WEEK_SAME_DAY)
		{
			// 8. y_1,md,1 == y_2,md,2   forall m,d
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
					matind[nonzeroes] = index_y_tmdw(0, m, d, 0); 
					matval[nonzeroes] = 1;
					++nonzeroes;

					// y_2,md,2
					matind[nonzeroes] = index_y_tmdw(1, m, d, 1);
					matval[nonzeroes] = -1;
					++nonzeroes;

					if (nonzeroes >= maxnonzeroes)
						throw std::runtime_error("Error in function IP_model_allocation_post::build_problem(). Nonzeroes exceeds size of maxnonzeroes (matind and matval)");

					status = CPXaddrows(env, problem, 0, 1, nonzeroes, rhs, sense, matbeg, matind.get(), matval.get(), NULL, NULL);
					if (status != 0)
					{
						CPXgeterrorstring(env, status, error_text);
						throw std::runtime_error("Error in function IP_model_allocation_post::build_problem(). \nCouldn't add constraint. \nReason: " + std::string(error_text));
					}

					// change name of constraint
					std::string conname = "c8_" + std::to_string(m + 1) + "_" + std::to_string(d + 1);
					status = CPXchgname(env, problem, 'r', nb_constraints, conname.c_str());
					if (status != 0)
					{
						CPXgeterrorstring(env, status, error_text);
						throw std::runtime_error("Error in function IP_model_allocation_post::build_problem(). \nCouldn't change constraint name. \nReason: " + std::string(error_text));
					}
				}
			}

			// 8bis. y_1,md,2 == y_2,md,1   forall m,d
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
					matind[nonzeroes] = index_y_tmdw(0, m, d, 1); 
					matval[nonzeroes] = 1;
					++nonzeroes;

					// y_2,md,2
					matind[nonzeroes] = index_y_tmdw(1, m, d, 0); 
					matval[nonzeroes] = -1;
					++nonzeroes;

					if (nonzeroes >= maxnonzeroes)
						throw std::runtime_error("Error in function IP_model_allocation_post::build_problem(). Nonzeroes exceeds size of maxnonzeroes (matind and matval)");

					status = CPXaddrows(env, problem, 0, 1, nonzeroes, rhs, sense, matbeg, matind.get(), matval.get(), NULL, NULL);
					if (status != 0)
					{
						CPXgeterrorstring(env, status, error_text);
						throw std::runtime_error("Error in function IP_model_allocation_post::build_problem(). \nCouldn't add constraint. \nReason: " + std::string(error_text));
					}

					// change name of constraint
					std::string conname = "c8bis_" + std::to_string(m + 1) + "_" + std::to_string(d + 1);
					status = CPXchgname(env, problem, 'r', nb_constraints, conname.c_str());
					if (status != 0)
					{
						CPXgeterrorstring(env, status, error_text);
						throw std::runtime_error("Error in function IP_model_allocation_post::build_problem(). \nCouldn't change constraint name. \nReason: " + std::string(error_text));
					}
				}
			}
		}

		// Enkel indien scenario van toepassing is
		if (_scenario == FIXED_WEEK_FREE_DAY)
		{
			// Dit veronderstelt dat max 1 bezoek per week, anders extra variabele nodig
			// 9. sum(t,d) y_tmdw <= 1   forall(w,m)
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
							matind[nonzeroes] = index_y_tmdw(t, m, d, w); 
							matval[nonzeroes] = 1;
							++nonzeroes;
						}
					}

					if (nonzeroes >= maxnonzeroes)
						throw std::runtime_error("Error in function IP_model_allocation_post::build_problem(). Nonzeroes exceeds size of maxnonzeroes (matind and matval)");

					status = CPXaddrows(env, problem, 0, 1, nonzeroes, rhs, sense, matbeg, matind.get(), matval.get(), NULL, NULL);
					if (status != 0)
					{
						CPXgeterrorstring(env, status, error_text);
						throw std::runtime_error("Error in function IP_model_allocation_post::build_problem(). \nCouldn't add constraint. \nReason: " + std::string(error_text));
					}

					// change name of constraint
					std::string conname = "c9_" + std::to_string(m + 1) + "_" + std::to_string(w + 1);
					status = CPXchgname(env, problem, 'r', nb_constraints, conname.c_str());
					if (status != 0)
					{
						CPXgeterrorstring(env, status, error_text);
						throw std::runtime_error("Error in function IP_model_allocation_post::build_problem(). \nCouldn't change constraint name. \nReason: " + std::string(error_text));
					}
				}
			}
		}



		// write to file
		status = CPXwriteprob(env, problem, "IP_model_allocation_post.lp", NULL);
		if (status != 0)
		{
			CPXgeterrorstring(env, status, error_text);
			throw std::runtime_error("Error in function IP_model_allocation_post::build_problem(). \nCouldn't write problem to lp-file. \nReason: " + std::string(error_text));
		}
	}

	void IP_model_allocation_post::solve_problem(const Instance& data)
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
			throw std::runtime_error("Error in function IP_model_allocation_post::solve_problem(). \nCouldn't set time limit. \nReason: " + std::string(error_text));
		}

		// Assign memory for solution
		solution_problem = std::make_unique<double[]>(CPXgetnumcols(env, problem));


		// Optimize the problem
		std::cout << "\n\n\nIP_model_allocation: CPLEX is solving the problem ...\n\n";
		auto start_time = std::chrono::system_clock::now();

		status = CPXmipopt(env, problem);
		if (status != 0)
		{
			CPXgeterrorstring(env, status, error_text);
			throw std::runtime_error("Error in function IP_model_allocation_post::solve_problem(). \nCPXmipopt failed. \nReason: " + std::string(error_text));
		}

		std::chrono::duration<double, std::ratio<1, 1>> elapsed_time_IP = std::chrono::system_clock::now() - start_time;


		// Get the solution
		status = CPXsolution(env, problem, &solstat, &objval, solution_problem.get(), NULL, NULL, NULL);
		if (status != 0)
		{
			CPXgeterrorstring(env, status, error_text);
			throw std::runtime_error("Error in function IP_model_allocation_post::solve_problem(). \nCPXsolution failed. \nReason: " + std::string(error_text));
		}

		char solstat_text[CPXMESSAGEBUFSIZE];
		auto p = CPXgetstatstring(env, solstat, solstat_text);
		if (p != nullptr)
		{
			std::cout << "\n\n\nDone solving ... \n\nSolution status: " << solstat_text;

			if (solstat == CPXMIP_OPTIMAL || solstat == CPXMIP_OPTIMAL_TOL || solstat == CPXMIP_TIME_LIM_FEAS)
			{
				_objective_value = objval;
				std::cout << "\nObjval = " << objval;
				std::cout << "\nElapsed time (s): " << elapsed_time_IP.count();

				const size_t nb_routes = data.nb_routes();
				const size_t nb_types = data.nb_waste_types();
				const size_t nb_zones = data.nb_zones();
				const size_t nb_days = data.nb_days();
				const size_t nb_weeks = data.nb_weeks();

				const int startindex_x_rdw = 0;
				const int startindex_y_tmdw = startindex_x_rdw + nb_routes * nb_days * nb_weeks;
				const int startindex_z_tmdw = startindex_y_tmdw + nb_types * nb_zones * nb_days * nb_weeks;
				const int startindex_beta = startindex_z_tmdw + nb_types * nb_zones * nb_days * nb_weeks;
				const int startindex_theta = startindex_beta + 1;

				// lambdas to get variable indices
				auto index_x_rdw = [startindex_x_rdw, nb_days, nb_weeks](int r, int d, int w) -> int {
					return startindex_x_rdw + r * nb_days * nb_weeks + d * nb_weeks + w;
					};
				auto index_y_tmdw = [startindex_y_tmdw, nb_zones, nb_days, nb_weeks](int t, int m, int d, int w) -> int {
					return startindex_y_tmdw + t * nb_zones * nb_days * nb_weeks + m * nb_days * nb_weeks + d * nb_weeks + w;
					};
				auto index_z_tmdw = [startindex_z_tmdw, nb_zones, nb_days, nb_weeks](int t, int m, int d, int w) -> int {
					return startindex_z_tmdw + t * nb_zones * nb_days * nb_weeks + m * nb_days * nb_weeks + d * nb_weeks + w;
					};
				auto index_theta_tm = [startindex_theta, nb_zones](int t, int m) -> int {
					return startindex_theta + t * nb_zones + m;
					};
				auto index_theta_r = [startindex_theta](int r) -> int {
					return startindex_theta + r;
					};

				
				// Write solution to file
				{
					std::ofstream solfile;
					std::string filename = data.name_instance() + "_allocation_post.txt";
					solfile.open(filename);

					solfile << "Instance: " << data.name_instance() << "\n";
					if (_scenario == Scenario::FIXED_WEEK_SAME_DAY)
						solfile << "\nScenario: fixed week & same day";
					else if (_scenario == Scenario::FIXED_WEEK_FREE_DAY)
						solfile << "\nScenario: fixed week & free day";
					else if (_scenario == Scenario::FREE_WEEK_FREE_DAY)
						solfile << "\nScenario: free week & free day";
					solfile << "\nMax computation time: " << _max_computation_time;

					solfile << "\n\nObjective value = " << objval;

					solfile << "\n\nx_rdw (routes toegewezen aan dagen en weken)";
					for (int r = 0; r < nb_routes; ++r) {
						for (int d = 0; d < nb_days; ++d) {
							for (int w = 0; w < nb_weeks; ++w) {
								size_t index_var = index_x_rdw(r, d, w);
								double val = solution_problem[index_var];
								if (val > 0.000001) {
									solfile << "\nRoute [";
									for (int ii = 0; ii < data.route(r)._pickups.size(); ++ii) {
										solfile << data.route(r)._pickups[ii];
										if (ii < data.route(r)._pickups.size() - 1)
											solfile << ",";
									}
									solfile << "], afvaltype = " << data.route(r)._waste_type <<
										", aantal keer = " << data.route(r)._nb_times_used << ", dag = " << data.day_name(d) << ", week " << w + 1;
								}
							}
						}
					}

					solfile << "\n\ny_tmdw  (wel of niet opgehaald)";
					for (int t = 0; t < nb_types; ++t) {
						for (int m = 0; m < nb_zones; ++m) {
							for (int d = 0; d < nb_days; ++d) {
								for (int w = 0; w < nb_weeks; ++w) {
									size_t index_var = index_y_tmdw(t, m, d, w);
									double val = solution_problem[index_var];
									if (val > 0.00001) {
										solfile << "\n" << data.waste_type(t) << ", " << data.zone_name(m) << ", " << data.day_name(d) << ", week " << w + 1 << ": y = " << val;
									}
								}
							}
						}
					}
					solfile << "\n\nz_tmdw  (andere ophaaldag dan huidig)";
					for (int t = 0; t < nb_types; ++t) {
						for (int m = 0; m < nb_zones; ++m) {
							for (int d = 0; d < nb_days; ++d) {
								for (int w = 0; w < nb_weeks; ++w) {
									size_t index_var = index_z_tmdw(t, m, d, w);
									double val = solution_problem[index_var];
									if (val > 0) {
										solfile << "\n" << data.waste_type(t) << ", " << data.zone_name(m) << ", " << data.day_name(d) << ", week " << w + 1 << ": z = " << val;
									}
								}
							}
						}
					}
					if (_penalty_on_route_assignment)
					{
						solfile << "\n\ntheta_r";
						for (int r = 0; r < nb_routes; ++r) {
							size_t index_var = index_theta_r(r);
							double val = solution_problem[index_var];
							if (val > 0.000001) {
								solfile << "\n";
								for (auto&& dest : data.route(r)._pickups)
									solfile << dest << ", ";
								solfile << "aantal_keer = " << data.route_nb_times_used(r);
							}
						}
					}
					else
					{
						solfile << "\n\ntheta_tm";
						for (int t = 0; t < nb_types; ++t) {
							for (int m = 0; m < nb_zones; ++m) {
								size_t index_var = index_theta_tm(t, m);
								double val = solution_problem[index_var];
								if (val > 0.000001) {
									solfile << "\n" << data.waste_type(t) << ", " << data.zone_name(m);
								}
							}
						}
					}
					{
						double val = solution_problem[startindex_beta];
						solfile << "\n\nbeta\n" << val;
					}

					// other layout
					solfile << "\n\n\n\nKalender\nZone\tMaandag\tDinsdag\tWoensdag\tDonderdag\tVrijdag\tMaandag\tDinsdag\tWoensdag\tDonderdag\tVrijdag";
					for (int m = 0; m < nb_zones; ++m) {
						solfile << "\n" << data.zone_name(m);
						for (int w = 0; w < nb_weeks; ++w) {
							for (int d = 0; d < nb_days; ++d) {
								solfile << "\t";
								for (int t = 0; t < nb_types; ++t) {
									size_t index_var = index_y_tmdw(t, m, d, w);
									double val = solution_problem[index_var];
									if (val > 0.000001) {
										solfile << data.waste_type(t);
									}
								}
							}
						}
					}

					// table routes & days
					solfile << "\n\n\nRoutes-dagen\nWeek\tDag\tAfval\tRoute\tAantal_keer";
					for (int w = 0; w < nb_weeks; ++w) {
						for (int d = 0; d < nb_days; ++d) {
							for (int r = 0; r < nb_routes; ++r) {
								size_t index_var = index_x_rdw(r, d, w);
								double val = solution_problem[index_var];
								if (val > 0.000001) {
									solfile << "\n" << w + 1 << "\t" << d + 1 << "\t";
									solfile << data.route(r)._waste_type << "\t";
									for (int ii = 0; ii < data.route(r)._pickups.size(); ++ii) {
										solfile << data.route(r)._pickups[ii];
										if (ii < data.route(r)._pickups.size() - 1)
											solfile << ", ";
									}
									solfile << "\t" << data.route(r)._nb_times_used;
								}
							}
						}
					}

					solfile.flush();
				}
			}
		}
	}

	void IP_model_allocation_post::clear_cplex()
	{
		int status = 0;
		char error_text[CPXMESSAGEBUFSIZE];

		// Free the problem
		status = CPXfreeprob(env, &problem);
		if (status != 0)
		{
			CPXgeterrorstring(env, status, error_text);
			throw std::runtime_error("Error in function IP_model_allocation_post::clear_cplex(). \nCouldn't free problem. \nReason: " + std::string(error_text));
		}

		// Close the cplex environment
		status = CPXcloseCPLEX(&env);
		if (status != 0)
		{
			CPXgeterrorstring(env, status, error_text);
			throw std::runtime_error("Error in function IP_model_allocation_post::clear_cplex(). \nCouldn't close cplex environment. \nReason: " + std::string(error_text));
		}
	}

	void IP_model_allocation_post::run(const Instance& data)
	{
		initialize_cplex();
		build_problem(data);
		solve_problem(data);
		clear_cplex();
	}

	///////////////////////////////////////////////////////////////////////////////////////////////
}
