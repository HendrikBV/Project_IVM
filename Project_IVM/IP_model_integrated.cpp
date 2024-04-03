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
#include <random>



namespace IVM
{
	///////////////////////////////////////////
	///			 IP Model Integrated		///
	///////////////////////////////////////////

	void IP_model_integrated::initialize_cplex()
	{
		int status = 0;
		char error_text[CPXMESSAGEBUFSIZE];

		// open the CPLEX environment
		env = CPXopenCPLEX(&status);
		if (status != 0)
		{
			CPXgeterrorstring(env, status, error_text);
			throw std::runtime_error("Error in function IP_model_integrated::initialize_cplex(). \nCouldn't open CPLEX. \nReason: " + std::string(error_text));
		}

		// turn output to screen on/off
		if (_output_solver)
			status = CPXsetintparam(env, CPXPARAM_ScreenOutput, CPX_ON);
		else
			status = CPXsetintparam(env, CPXPARAM_ScreenOutput, CPX_OFF);
		if (status != 0)
		{
			CPXgeterrorstring(env, status, error_text);
			throw std::runtime_error("Error in function IP_model_integrated::initialize_cplex(). \nCouldn't change param SCRIND. \nReason: " + std::string(error_text));
		}
	}

	void IP_model_integrated::build_problem(const Instance& data)
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
		problem = CPXcreateprob(env, &status, "IP_model_integrated");
		if (status != 0)
		{
			CPXgeterrorstring(env, status, error_text);
			throw std::runtime_error("Error in function IP_model_integrated::build_problem(). \nCouldn't create problem. \nReason: " + std::string(error_text));
		}

		// problem is minimization
		status = CPXchgobjsen(env, problem, CPX_MIN);
		if (status != 0)
		{
			CPXgeterrorstring(env, status, error_text);
			throw std::runtime_error("Error in function IP_model_integrated::build_problem(). \nCouldn't change objective to minimization. \nReason: " + std::string(error_text));
		}


		// data 
		const size_t nb_days_total = data.nb_days() * data.nb_weeks();
		const size_t nb_trucks = _max_nb_trucks;
		const size_t nb_segments = _max_nb_segments;
		const size_t nb_zones = data.nb_zones();
		const size_t nb_collection_points = data.nb_collection_points();
		const size_t nb_locations = nb_zones + 1 + nb_collection_points; // Order: Z1...Zn, depot, CP1...CPk
		const size_t max_visits = _max_visits;


		// add variables
		int nb_variables = -1;

		// variable x_dvijk   
		const int startindex_x_dvijk = 0;
		for (int d = 0; d < nb_days_total; ++d)
		{
			for (int v = 0; v < nb_trucks; ++v)
			{
				for (int i = 0; i < nb_locations; ++i)
				{
					for (int j = 0; j < nb_locations; ++j)
					{
						for (int k = 0; k < nb_segments; ++k)
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
								throw std::runtime_error("Error in function IP_model_routing::build_problem(). \nCouldn't add variable. \nReason: " + std::string(error_text));
							}

							// change variable name
							std::string varname = "x_" + std::to_string(d + 1) + "_" + std::to_string(v + 1) + "_"
								+ std::to_string(i + 1) + "_" + std::to_string(j + 1) + "_" + std::to_string(k + 1);
							status = CPXchgname(env, problem, 'c', nb_variables, varname.c_str());
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

		// variable w_dvik   
		const int startindex_w_dvik = startindex_x_dvijk + nb_days_total * nb_trucks * nb_locations * nb_locations * nb_segments;
		for (int d = 0; d < nb_days_total; ++d)
		{
			for (int v = 0; v < nb_trucks; ++v)
			{
				for (int m = 0; m < nb_zones; ++m) // enkel voor zones
				{
					for (int k = 0; k < nb_segments; ++k)
					{
						++nb_variables;

						obj[0] = 0;
						lb[0] = 0;

						status = CPXnewcols(env, problem, 1, obj, lb, NULL, NULL, NULL);
						if (status != 0)
						{
							CPXgeterrorstring(env, status, error_text);
							throw std::runtime_error("Error in function IP_model_routing::build_problem(). \nCouldn't add variable. \nReason: " + std::string(error_text));
						}

						// change variable name
						std::string varname = "w_" + std::to_string(d + 1) + "_" + std::to_string(v + 1) + "_" + std::to_string(m + 1) + "_" + std::to_string(k + 1);
						status = CPXchgname(env, problem, 'c', nb_variables, varname.c_str());
						if (status != 0)
						{
							CPXgeterrorstring(env, status, error_text);
							throw std::runtime_error("Error in function IP_model_routing::build_problem(). \nCouldn't change variable name. \nReason: " + std::string(error_text));
						}
					}
				}
			}
		}

		// variable y_dv   
		const int startindex_y_dv = startindex_w_dvik + nb_days_total * nb_trucks * nb_zones * nb_segments;
		for (int d = 0; d < nb_days_total; ++d)
		{
			for (int v = 0; v < nb_trucks; ++v)
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
					throw std::runtime_error("Error in function IP_model_routing::build_problem(). \nCouldn't add variable. \nReason: " + std::string(error_text));
				}

				// change variable name
				std::string varname = "y_" + std::to_string(d + 1) + "_" + std::to_string(v + 1);
				status = CPXchgname(env, problem, 'c', nb_variables, varname.c_str());
				if (status != 0)
				{
					CPXgeterrorstring(env, status, error_text);
					throw std::runtime_error("Error in function IP_model_routing::build_problem(). \nCouldn't change variable name. \nReason: " + std::string(error_text));
				}
			}
		}

		// variable beta_dv   
		const int startindex_beta_dv = startindex_y_dv + nb_days_total * nb_trucks;
		for (int d = 0; d < nb_days_total; ++d)
		{
			for (int v = 0; v < nb_trucks; ++v)
			{
				++nb_variables;

				obj[0] = data.operating_costs(0); // assume one truck type
				lb[0] = 0;

				status = CPXnewcols(env, problem, 1, obj, lb, NULL, NULL, NULL);
				if (status != 0)
				{
					CPXgeterrorstring(env, status, error_text);
					throw std::runtime_error("Error in function IP_model_routing::build_problem(). \nCouldn't add variable. \nReason: " + std::string(error_text));
				}

				// change variable name
				std::string varname = "beta_" + std::to_string(d + 1) + "_" + std::to_string(v + 1);
				status = CPXchgname(env, problem, 'c', nb_variables, varname.c_str());
				if (status != 0)
				{
					CPXgeterrorstring(env, status, error_text);
					throw std::runtime_error("Error in function IP_model_routing::build_problem(). \nCouldn't change variable name. \nReason: " + std::string(error_text));
				}
			}
		}

		// variable z
		const int startindex_z = startindex_beta_dv + nb_days_total * nb_trucks;
		{
			++nb_variables;

			obj[0] = nb_days_total * data.fixed_costs(0); // assume one truck type
			lb[0] = 0;
			type[0] = 'I';

			status = CPXnewcols(env, problem, 1, obj, lb, NULL, type, NULL);
			if (status != 0)
			{
				CPXgeterrorstring(env, status, error_text);
				throw std::runtime_error("Error in function IP_model_routing::build_problem(). \nCouldn't add variable. \nReason: " + std::string(error_text));
			}

			// change variable name
			std::string varname = "z";
			status = CPXchgname(env, problem, 'c', nb_variables, varname.c_str());
			if (status != 0)
			{
				CPXgeterrorstring(env, status, error_text);
				throw std::runtime_error("Error in function IP_model_routing::build_problem(). \nCouldn't change variable name. \nReason: " + std::string(error_text));
			}
		}

		// variable ksi_di
		const int startindex_ksi_di = startindex_z + 1;
		for (int d = 0; d < nb_days_total; ++d)
		{
			for (int m = 0; m < nb_zones; ++m)  // enkel voor zones
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
					throw std::runtime_error("Error in function IP_model_routing::build_problem(). \nCouldn't add variable. \nReason: " + std::string(error_text));
				}

				// change variable name
				std::string varname = "ksi_" + std::to_string(d + 1) + std::to_string(m + 1);
				status = CPXchgname(env, problem, 'c', nb_variables, varname.c_str());
				if (status != 0)
				{
					CPXgeterrorstring(env, status, error_text);
					throw std::runtime_error("Error in function IP_model_routing::build_problem(). \nCouldn't change variable name. \nReason: " + std::string(error_text));
				}
			}
		}


		// lambdas to get variable indices
		auto index_x_dvijk = [startindex_x_dvijk, nb_trucks, nb_locations, nb_segments](int d, int v, int i, int j, int k) -> int {
			return startindex_x_dvijk + d * nb_trucks * nb_locations * nb_locations * nb_segments + v * nb_locations * nb_locations * nb_segments
				+ i * nb_locations * nb_segments + j * nb_segments + k;
			};
		auto index_w_dvik = [startindex_w_dvik, nb_trucks, nb_zones, nb_segments](int d, int v, int i_zone, int k) -> int {
			return startindex_w_dvik + d * nb_trucks * nb_zones * nb_segments + v * nb_zones * nb_segments + i_zone * nb_segments + k;
			};
		auto index_y_dv = [startindex_y_dv, nb_trucks](int d, int v) -> int {
			return startindex_y_dv + d * nb_trucks + v;
			};
		auto index_beta_dv = [startindex_beta_dv, nb_trucks](int d, int v) -> int {
			return startindex_beta_dv + d * nb_trucks + v;
			};
		auto index_ksi_di = [startindex_ksi_di, nb_zones](int d, int m) -> int {
			return startindex_ksi_di + d * nb_zones + m;
			};


		nb_variables = CPXgetnumcols(env, problem);



		// add constraints
		int nb_constraints = -1;

		// 1: beta_dv - sum(i,j,k) tau_D_ij*x_dvijk - sum(i,k) tau_P_i*w_dvik - sum(i,j,k) tau_U*x_dvijk == 0   forall d,v
		for (int d = 0; d < nb_days_total; ++d)
		{
			for (int v = 0; v < nb_trucks; ++v)
			{
				++nb_constraints;

				rhs[0] = 0;
				sense[0] = 'E';
				matbeg[0] = 0;

				nonzeroes = 0;

				// beta_dv
				{
					const size_t index = index_beta_dv(d, v);
					if (index >= nb_variables)
						throw std::runtime_error("Error in function IP_model_routing::build_problem(). Index variable exceeds range");

					matind[nonzeroes] = index;
					matval[nonzeroes] = 1;
					++nonzeroes;
				}

				// -sum(i,j,k) (tau_D_ij + tau_U) * x_dvijk
				for (int i = 0; i < nb_locations; ++i)
				{
					for (int j = 0; j < nb_locations; ++j)
					{
						for (int k = 0; k < nb_segments; ++k)
						{
							double coeff = 0;
							if (i < nb_zones && j == nb_zones) { // i == zone, j == depot
								coeff = -data.time_driving_zone_depot(i);
							}
							else if (i < nb_zones && j > nb_zones) { // i == zone, j == collection point
								const std::string& cp = data.collection_point_name(j - nb_zones - 1);
								coeff += -data.time_driving_zone_collectionpoint(i, cp);

								const std::string& waste_type = data.waste_type(0); // MOMENTEEL ZELFDE VERONDERSTELD
								coeff += -data.time_unloading(waste_type);
							}
							else if (i == nb_zones && j < nb_zones) { // i == depot, j == zone
								coeff += -data.time_driving_zone_depot(j);
							}
							else if (i > nb_zones && j < nb_zones) { // i == collection point, j == zone
								const std::string& cp = data.collection_point_name(i - nb_zones - 1);
								coeff += -data.time_driving_zone_collectionpoint(j, cp);
							}
							else if (i > nb_zones && j == nb_zones) { // i == collection point, j == depot
								coeff += -data.time_driving_collectionpoint_depot(i - nb_zones - 1);
							}

							const size_t index = index_x_dvijk(d, v, i, j, k);
							if (index >= nb_variables)
								throw std::runtime_error("Error in function IP_model_routing::build_problem(). Index variable exceeds range");

							matind[nonzeroes] = index;
							matval[nonzeroes] = coeff;
							++nonzeroes;
						}
					}
				}

				// -sum(m,k) tau_P_i*w_dvik
				for (int m = 0; m < nb_zones; ++m) // enkel zones
				{
					for (int k = 0; k < nb_segments; ++k)
					{
						const size_t index = index_w_dvik(d, v, m, k);
						if (index >= nb_variables)
							throw std::runtime_error("Error in function IP_model_routing::build_problem(). Index variable exceeds range");

						matind[nonzeroes] = index;
						const std::string& waste_type = data.waste_type(0); // assume only one waste type
						matval[nonzeroes] = -data.time_pickup(m, waste_type);
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
				std::string conname = "c1_" + std::to_string(d + 1) + "_" + std::to_string(v + 1);
				status = CPXchgname(env, problem, 'r', nb_constraints, conname.c_str());
				if (status != 0)
				{
					CPXgeterrorstring(env, status, error_text);
					throw std::runtime_error("Error in function IP_model_routing::build_problem(). \nCouldn't change constraint name. \nReason: " + std::string(error_text));
				}
			}
		}

		// 2: beta_dv <= T  forall d,v
		for (int d = 0; d < nb_days_total; ++d)
		{
			for (int v = 0; v < nb_trucks; ++v)
			{
				++nb_constraints;

				rhs[0] = data.max_driving_time(0); // assume one truck type
				sense[0] = 'L';
				matbeg[0] = 0;

				nonzeroes = 0;

				// beta_qv
				{
					const size_t index = index_beta_dv(d, v);
					if (index >= nb_variables)
						throw std::runtime_error("Error in function IP_model_routing::build_problem(). Index variable exceeds range");

					matind[nonzeroes] = index;
					matval[nonzeroes] = 1;
					++nonzeroes;
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
				std::string conname = "c2_" + std::to_string(d + 1) + "_" + std::to_string(v + 1);
				status = CPXchgname(env, problem, 'r', nb_constraints, conname.c_str());
				if (status != 0)
				{
					CPXgeterrorstring(env, status, error_text);
					throw std::runtime_error("Error in function IP_model_routing::build_problem(). \nCouldn't change constraint name. \nReason: " + std::string(error_text));
				}
			}
		}

		// 3: w_dvik <= L sum(j) x_djik   forall t,q,v,i,k
		for (int d = 0; d < nb_days_total; ++d)
		{
			for (int v = 0; v < nb_trucks; ++v)
			{
				for (int m = 0; m < nb_zones; ++m) // enkel zones
				{
					for (int k = 0; k < nb_segments; ++k)
					{
						++nb_constraints;

						rhs[0] = 0;
						sense[0] = 'L';
						matbeg[0] = 0;

						nonzeroes = 0;

						// w_tqvik
						{
							const size_t index = index_w_dvik(d, v, m, k);
							if (index >= nb_variables)
								throw std::runtime_error("Error in function IP_model_routing::build_problem(). Index variable exceeds range");

							matind[nonzeroes] = index;
							matval[nonzeroes] = 1;
							++nonzeroes;
						}

						// - L_tq sum(j) x_qvjik (aankomen bij i)
						for (int j = 0; j < nb_locations; ++j)
						{
							const size_t index = index_x_dvijk(d, v, j, m, k);
							if (index >= nb_variables)
								throw std::runtime_error("Error in function IP_model_routing::build_problem(). Index variable exceeds range");

							matind[nonzeroes] = index;
							const std::string& waste_type = data.waste_type(0); // assume one waste type
							matval[nonzeroes] = -data.capacity(0, waste_type); // assume one truck type
							++nonzeroes;
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
						std::string conname = "c3_" + std::to_string(d + 1) + "_" + std::to_string(v + 1)
							+ "_" + std::to_string(m + 1) + "_" + std::to_string(k + 1);
						status = CPXchgname(env, problem, 'r', nb_constraints, conname.c_str());
						if (status != 0)
						{
							CPXgeterrorstring(env, status, error_text);
							throw std::runtime_error("Error in function IP_model_routing::build_problem(). \nCouldn't change constraint name. \nReason: " + std::string(error_text));
						}
					}
				}
			}
		}

		// 4. sum(d,v,k) w_dvik == alpha_i   forall i
		for (int m = 0; m < nb_zones; ++m)
		{
			++nb_constraints;

			const std::string& waste_type = data.waste_type(0); // assume one waste type
			rhs[0] = data.demand(m, waste_type);

			sense[0] = 'E';
			matbeg[0] = 0;

			nonzeroes = 0;

			// sum(d,v,k) w_dvik
			for (int d = 0; d < nb_days_total; ++d)
			{
				for (int v = 0; v < nb_trucks; ++v)
				{
					for (int k = 0; k < nb_segments; ++k)
					{
						const size_t index = index_w_dvik(d, v, m, k);
						if (index >= nb_variables)
							throw std::runtime_error("Error in function IP_model_routing::build_problem(). Index variable exceeds range");

						matind[nonzeroes] = index;
						matval[nonzeroes] = 1;
						++nonzeroes;
					}
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
			std::string conname = "c4_" + std::to_string(m + 1);
			status = CPXchgname(env, problem, 'r', nb_constraints, conname.c_str());
			if (status != 0)
			{
				CPXgeterrorstring(env, status, error_text);
				throw std::runtime_error("Error in function IP_model_routing::build_problem(). \nCouldn't change constraint name. \nReason: " + std::string(error_text));
			}
		}

		// 5. sum(j) x_dv,depot,j,1 - y_dv == 0   forall d,v
		for (int d = 0; d < nb_days_total; ++d)
		{
			for (int v = 0; v < nb_trucks; ++v)
			{
				++nb_constraints;

				rhs[0] = 0;
				sense[0] = 'E';
				matbeg[0] = 0;

				nonzeroes = 0;

				// sum(j) x_dv,depot,j,1
				for (int j = 0; j < nb_locations; ++j)
				{
					const int index_depot = nb_zones;
					const int index_k = 0;

					const size_t index = index_x_dvijk(d, v, index_depot, j, index_k);
					if (index >= nb_variables)
						throw std::runtime_error("Error in function IP_model_routing::build_problem(). Index variable exceeds range");

					matind[nonzeroes] = index;
					matval[nonzeroes] = 1;
					++nonzeroes;
				}

				// y_dv
				{
					const int index = index_y_dv(d, v);
					if (index >= nb_variables)
						throw std::runtime_error("Error in function IP_model_routing::build_problem(). Index variable exceeds range");

					matind[nonzeroes] = index;
					matval[nonzeroes] = -1;
					++nonzeroes;
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
				std::string conname = "c5_" + std::to_string(d + 1) + "_" + std::to_string(v + 1);
				status = CPXchgname(env, problem, 'r', nb_constraints, conname.c_str());
				if (status != 0)
				{
					CPXgeterrorstring(env, status, error_text);
					throw std::runtime_error("Error in function IP_model_routing::build_problem(). \nCouldn't change constraint name. \nReason: " + std::string(error_text));
				}
			}
		}

		// 6. sum(j,k) x_dvi,depot,k - y_dv == 0   forall d,v
		for (int d = 0; d < nb_days_total; ++d)
		{
			for (int v = 0; v < nb_trucks; ++v)
			{
				++nb_constraints;

				rhs[0] = 0;
				sense[0] = 'E';
				matbeg[0] = 0;

				nonzeroes = 0;

				// sum(j,k) x_dvi,depot,k
				for (int i = 0; i < nb_locations; ++i)
				{
					for (int k = 0; k < _max_nb_segments; ++k)
					{
						const int index_depot = nb_zones;

						const size_t index = index_x_dvijk(d, v, i, index_depot, k);
						if (index >= nb_variables)
							throw std::runtime_error("Error in function IP_model_routing::build_problem(). Index variable exceeds range");

						matind[nonzeroes] = index;
						matval[nonzeroes] = 1;
						++nonzeroes;
					}
				}

				// y_dv
				{
					const size_t index = index_y_dv(d, v);
					if (index >= nb_variables)
						throw std::runtime_error("Error in function IP_model_routing::build_problem(). Index variable exceeds range");

					matind[nonzeroes] = index;
					matval[nonzeroes] = -1;
					++nonzeroes;
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
				std::string conname = "c6_" + std::to_string(d + 1) + "_" + std::to_string(v + 1);
				status = CPXchgname(env, problem, 'r', nb_constraints, conname.c_str());
				if (status != 0)
				{
					CPXgeterrorstring(env, status, error_text);
					throw std::runtime_error("Error in function IP_model_routing::build_problem(). \nCouldn't change constraint name. \nReason: " + std::string(error_text));
				}
			}
		}

		// 7. x_dvijk == 0   for zone-zone or dropoff-dropoff
		for (int d = 0; d < nb_days_total; ++d)
		{
			for (int v = 0; v < nb_trucks; ++v)
			{
				for (int i = 0; i < nb_locations; ++i)
				{
					for (int j = 0; j < nb_locations; ++j)
					{
						for (int k = 0; k < nb_segments; ++k)
						{
							// forbidden combinations
							if ((i < nb_zones && j < nb_zones)       // zone - zone
								|| (i > nb_zones && j > nb_zones)    // dropoff - dropoff
								|| (i == nb_zones && j == nb_zones)  // depot - depot
								|| (i == nb_zones && j > nb_zones)   // depot - dropoff (dropoff-depot however is possible)
								|| (i < nb_zones && j == nb_zones)   // zone - depot (always first to dropoff)
								|| (i == nb_zones && k > 0)) // depot-zone if not first segment
							{
								++nb_constraints;

								rhs[0] = 0;
								sense[0] = 'E';
								matbeg[0] = 0;

								nonzeroes = 0;

								// x_dvijk
								{
									const size_t index = index_x_dvijk(d, v, i, j, k);
									if (index >= nb_variables)
										throw std::runtime_error("Error in function IP_model_routing::build_problem(). Index variable exceeds range");

									matind[nonzeroes] = index;
									matval[nonzeroes] = 1;
									++nonzeroes;
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
								std::string conname = "c7_" + std::to_string(d + 1) + "_" + std::to_string(v + 1) + "_" + std::to_string(i + 1) + "_"
									+ std::to_string(j + 1) + "_" + std::to_string(k + 1);
								status = CPXchgname(env, problem, 'r', nb_constraints, conname.c_str());
								if (status != 0)
								{
									CPXgeterrorstring(env, status, error_text);
									throw std::runtime_error("Error in function IP_model_routing::build_problem(). \nCouldn't change constraint name. \nReason: " + std::string(error_text));
								}
							}

							// forbidden dropoffs at collection points
							// hier niet van toepassing
							/*if (j > nb_zones)
							{
								int index_cp = j - nb_zones - 1;
								auto& truck_name = data.truck_type(0);
								if ((truck_name == "truck_GFT" && !data.collection_point_waste_type_allowed(index_cp, "GFT"))
									|| (truck_name == "truck_restafval" && !data.collection_point_waste_type_allowed(index_cp, "restafval")))
								{
									++nb_constraints;

									rhs[0] = 0;
									sense[0] = 'E';
									matbeg[0] = 0;

									nonzeroes = 0;

									// x_qvijk
									{
										const size_t index = index_x_qvijk(q, v, i, j, k);
										if (index >= nb_variables)
											throw std::runtime_error("Error in function IP_model_routing::build_problem(). Index variable exceeds range");

										matind[nonzeroes] = index;
										matval[nonzeroes] = 1;
										++nonzeroes;
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
									std::string conname = "c7_dropoffs_" + std::to_string(q + 1) + "_" + std::to_string(v + 1) + "_" + std::to_string(i + 1) + "_"
										+ std::to_string(j + 1) + "_" + std::to_string(k + 1);
									status = CPXchgname(env, problem, 'r', nb_constraints, conname.c_str());
									if (status != 0)
									{
										CPXgeterrorstring(env, status, error_text);
										throw std::runtime_error("Error in function IP_model_routing::build_problem(). \nCouldn't change constraint name. \nReason: " + std::string(error_text));
									}
								}
							}*/
						}
					}
				}
			}
		}

		// 8. sum(j) x_dvij,k+1 - sum(j) x_dvjik == 0   forall d,v,i!=depot,k
		for (int d = 0; d < nb_days_total; ++d)
		{
			for (int v = 0; v < nb_trucks; ++v)
			{
				for (int i = 0; i < nb_locations; ++i)
				{
					if (i != nb_zones) // niet depot
					{
						for (int k = 0; k < nb_segments - 1; ++k)
						{
							++nb_constraints;

							rhs[0] = 0;
							sense[0] = 'E';
							matbeg[0] = 0;

							nonzeroes = 0;

							// sum(j) x_dvij,k+1
							for (int j = 0; j < nb_locations; ++j)
							{
								const size_t index = index_x_dvijk(d, v, i, j, k + 1);
								if (index >= nb_variables)
									throw std::runtime_error("Error in function IP_model_routing::build_problem(). Index variable exceeds range");

								matind[nonzeroes] = index;
								matval[nonzeroes] = 1;
								++nonzeroes;
							}

							// sum(j) x_dvjik
							for (int j = 0; j < nb_locations; ++j)
							{
								const size_t index = index_x_dvijk(d, v, j, i, k);
								if (index >= nb_variables)
									throw std::runtime_error("Error in function IP_model_routing::build_problem(). Index variable exceeds range");

								matind[nonzeroes] = index;
								matval[nonzeroes] = -1;
								++nonzeroes;
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
							std::string conname = "c8_" + std::to_string(d + 1) + "_" + std::to_string(v + 1) + "_" + std::to_string(i + 1) + "_" + std::to_string(k + 1);
							status = CPXchgname(env, problem, 'r', nb_constraints, conname.c_str());
							if (status != 0)
							{
								CPXgeterrorstring(env, status, error_text);
								throw std::runtime_error("Error in function IP_model_routing::build_problem(). \nCouldn't change constraint name. \nReason: " + std::string(error_text));
							}
						}
					}
				}
			}
		}

		// 9. sum(i,j) x_dvijk <= 1   forall d,v,k
		for (int d = 0; d < nb_days_total; ++d)
		{
			for (int v = 0; v < _max_nb_trucks; ++v)
			{
				for (int k = 0; k < _max_nb_segments; ++k)
				{
					++nb_constraints;

					rhs[0] = 1;
					sense[0] = 'L';
					matbeg[0] = 0;

					nonzeroes = 0;

					// sum(i,j) x_qvijk
					for (int i = 0; i < nb_locations; ++i)
					{
						for (int j = 0; j < nb_locations; ++j)
						{
							const size_t index = index_x_dvijk(d, v, i, j, k);
							if (index >= nb_variables)
								throw std::runtime_error("Error in function IP_model_routing::build_problem(). Index variable exceeds range");

							matind[nonzeroes] = index;
							matval[nonzeroes] = 1;
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
					std::string conname = "c9_" + std::to_string(d + 1) + "_" + std::to_string(v + 1) + "_" + std::to_string(k + 1);
					status = CPXchgname(env, problem, 'r', nb_constraints, conname.c_str());
					if (status != 0)
					{
						CPXgeterrorstring(env, status, error_text);
						throw std::runtime_error("Error in function IP_model_routing::build_problem(). \nCouldn't change constraint name. \nReason: " + std::string(error_text));
					}
				}
			}
		}

		// 10. x_dvijk - y_dv <= 0   forall d,v,i,j,k
		for (int d = 0; d < nb_days_total; ++d)
		{
			for (int v = 0; v < nb_trucks; ++v)
			{
				for (int i = 0; i < nb_locations; ++i)
				{
					for (int j = 0; j < nb_locations; ++j)
					{
						for (int k = 0; k < nb_segments; ++k)
						{
							++nb_constraints;

							rhs[0] = 0;
							sense[0] = 'L';
							matbeg[0] = 0;

							nonzeroes = 0;

							// x_qvijk
							{
								const size_t index = index_x_dvijk(d, v, i, j, k);
								if (index >= nb_variables)
									throw std::runtime_error("Error in function IP_model_routing::build_problem(). Index variable exceeds range");

								matind[nonzeroes] = index;
								matval[nonzeroes] = 1;
								++nonzeroes;
							}

							// - y_qv
							{
								const size_t index = index_y_dv(d, v);
								if (index >= nb_variables)
									throw std::runtime_error("Error in function IP_model_routing::build_problem(). Index variable exceeds range");

								matind[nonzeroes] = index;
								matval[nonzeroes] = -1;
								++nonzeroes;
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
							std::string conname = "c10_" + std::to_string(d + 1) + "_" + std::to_string(v + 1) + "_" + std::to_string(i + 1)
								+ "_" + std::to_string(j + 1) + "_" + std::to_string(k + 1);
							status = CPXchgname(env, problem, 'r', nb_constraints, conname.c_str());
							if (status != 0)
							{
								CPXgeterrorstring(env, status, error_text);
								throw std::runtime_error("Error in function IP_model_routing::build_problem(). \nCouldn't change constraint name. \nReason: " + std::string(error_text));
							}
						}
					}
				}
			}
		}

		// 11. sum(v) y_dv - z <= 0   forall d
		for (int d = 0; d < nb_days_total; ++d)
		{
			++nb_constraints;

			rhs[0] = 0;
			sense[0] = 'L';
			matbeg[0] = 0;

			nonzeroes = 0;

			// y_dv 
			for(int v = 0; v < nb_trucks; ++v)
			{
				const size_t index = index_y_dv(d, v);
				if (index >= nb_variables)
					throw std::runtime_error("Error in function IP_model_routing::build_problem(). Index variable exceeds range");

				matind[nonzeroes] = index;
				matval[nonzeroes] = 1;
				++nonzeroes;
			}

			// - z
			{
				const size_t index = startindex_z;
				if (index >= nb_variables)
					throw std::runtime_error("Error in function IP_model_routing::build_problem(). Index variable exceeds range");

				matind[nonzeroes] = index;
				matval[nonzeroes] = -1;
				++nonzeroes;
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
			std::string conname = "c11_" + std::to_string(d + 1);
			status = CPXchgname(env, problem, 'r', nb_constraints, conname.c_str());
			if (status != 0)
			{
				CPXgeterrorstring(env, status, error_text);
				throw std::runtime_error("Error in function IP_model_routing::build_problem(). \nCouldn't change constraint name. \nReason: " + std::string(error_text));
			}
		}

		// 12. x_dvijk - ksi_di <= 0   forall d,v,i,j,k
		for (int d = 0; d < nb_days_total; ++d)
		{
			for (int v = 0; v < nb_trucks; ++v)
			{
				for (int i = 0; i < nb_zones; ++i) // enkel zones
				{
					for (int j = 0; j < nb_locations; ++j)
					{
						for (int k = 0; k < nb_segments; ++k)
						{
							++nb_constraints;

							rhs[0] = 0;
							sense[0] = 'L';
							matbeg[0] = 0;

							nonzeroes = 0;

							// x_dvijk
							{
								const size_t index = index_x_dvijk(d, v, i, j, k);
								if (index >= nb_variables)
									throw std::runtime_error("Error in function IP_model_routing::build_problem(). Index variable exceeds range");

								matind[nonzeroes] = index;
								matval[nonzeroes] = 1;
								++nonzeroes;
							}

							// - ksi_di
							{
								const size_t index = index_ksi_di(d, i);
								if (index >= nb_variables)
									throw std::runtime_error("Error in function IP_model_routing::build_problem(). Index variable exceeds range");

								matind[nonzeroes] = index;
								matval[nonzeroes] = -1;
								++nonzeroes;
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
							std::string conname = "c12_" + std::to_string(d + 1) + "_" + std::to_string(v + 1) + "_" + std::to_string(i + 1)
								+ "_" + std::to_string(j + 1) + "_" + std::to_string(k + 1);
							status = CPXchgname(env, problem, 'r', nb_constraints, conname.c_str());
							if (status != 0)
							{
								CPXgeterrorstring(env, status, error_text);
								throw std::runtime_error("Error in function IP_model_routing::build_problem(). \nCouldn't change constraint name. \nReason: " + std::string(error_text));
							}
						}
					}
				}
			}
		}

		// 13. sum(d) ksi_di <= max_visits   forall i (zones)
		for (int i = 0; i < nb_zones; ++i)
		{
			++nb_constraints;

			rhs[0] = max_visits;
			sense[0] = 'L'; 
			matbeg[0] = 0;

			nonzeroes = 0;

			// sum(d) ksi_di
			for(int d = 0; d < nb_days_total; ++d)
			{
				const size_t index = index_ksi_di(d, i);
				if (index >= nb_variables)
					throw std::runtime_error("Error in function IP_model_routing::build_problem(). Index variable exceeds range");

				matind[nonzeroes] = index;
				matval[nonzeroes] = 1;
				++nonzeroes;
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
			std::string conname = "c13_" + std::to_string(i + 1);
			status = CPXchgname(env, problem, 'r', nb_constraints, conname.c_str());
			if (status != 0)
			{
				CPXgeterrorstring(env, status, error_text);
				throw std::runtime_error("Error in function IP_model_routing::build_problem(). \nCouldn't change constraint name. \nReason: " + std::string(error_text));
			}
		}

		// 14. ksi_di = 0   on forbidden days d
		for (int d = 0; d < nb_days_total; ++d)
		{
			for (int i = 0; i < nb_zones; ++i)
			{
				int day = d % 5; // day of week, week does not matter
				if (data.zone_forbidden_day(i, day))
				{
					++nb_constraints;

					rhs[0] = 0;
					sense[0] = 'E';
					matbeg[0] = 0;

					nonzeroes = 0;

					// ksi_di
					const size_t index = index_ksi_di(d, i);
					if (index >= nb_variables)
						throw std::runtime_error("Error in function IP_model_routing::build_problem(). Index variable exceeds range");

					matind[nonzeroes] = index;
					matval[nonzeroes] = 1;
					++nonzeroes;

					if (nonzeroes >= maxnonzeroes)
						throw std::runtime_error("Error in function IP_model_routing::build_problem(). Nonzeroes exceeds size of maxnonzeroes (matind and matval)");

					status = CPXaddrows(env, problem, 0, 1, nonzeroes, rhs, sense, matbeg, matind.get(), matval.get(), NULL, NULL);
					if (status != 0)
					{
						CPXgeterrorstring(env, status, error_text);
						throw std::runtime_error("Error in function IP_model_routing::build_problem(). \nCouldn't add constraint. \nReason: " + std::string(error_text));
					}

					// change name of constraint
					std::string conname = "c14_" + std::to_string(d + 1) + "_" + std::to_string(i + 1);
					status = CPXchgname(env, problem, 'r', nb_constraints, conname.c_str());
					if (status != 0)
					{
						CPXgeterrorstring(env, status, error_text);
						throw std::runtime_error("Error in function IP_model_routing::build_problem(). \nCouldn't change constraint name. \nReason: " + std::string(error_text));
					}
				}
			}
		}

		// Symmetry breaking
		/*for (int d = 0; d < nb_days_total; ++d)
		{
			for (int v = 0; v < nb_trucks - 1; ++v) // niet laatste
			{
				++nb_constraints;

				rhs[0] = 0;
				sense[0] = 'L';
				matbeg[0] = 0;

				nonzeroes = 0;

				// y_d,v+1
				{
					const size_t index = index_y_dv(d, v + 1);
					if (index >= nb_variables)
						throw std::runtime_error("Error in function IP_model_routing::build_problem(). Index variable exceeds range");

					matind[nonzeroes] = index;
					matval[nonzeroes] = 1;
					++nonzeroes;
				}

				// -y_dv
				{
					const size_t index = index_y_dv(d, v);
					if (index >= nb_variables)
						throw std::runtime_error("Error in function IP_model_routing::build_problem(). Index variable exceeds range");

					matind[nonzeroes] = index;
					matval[nonzeroes] = -1;
					++nonzeroes;
				}

				status = CPXaddrows(env, problem, 0, 1, nonzeroes, rhs, sense, matbeg, matind.get(), matval.get(), NULL, NULL);
				if (status != 0)
				{
					CPXgeterrorstring(env, status, error_text);
					throw std::runtime_error("Error in function IP_model_routing::build_problem(). \nCouldn't add constraint. \nReason: " + std::string(error_text));
				}

				// change name of constraint
				std::string conname = "symmetry_breaking_c1_" + std::to_string(d + 1) + "_" + std::to_string(v + 1);
				status = CPXchgname(env, problem, 'r', nb_constraints, conname.c_str());
				if (status != 0)
				{
					CPXgeterrorstring(env, status, error_text);
					throw std::runtime_error("Error in function IP_model_routing::build_problem(). \nCouldn't change constraint name. \nReason: " + std::string(error_text));
				}
			}
		}
		*/


		// write to file
		status = CPXwriteprob(env, problem, "IP_model_integrated.lp", NULL);
		if (status != 0)
		{
			CPXgeterrorstring(env, status, error_text);
			throw std::runtime_error("Error in function IP_model_integrated::build_problem(). \nCouldn't write problem to lp-file. \nReason: " + std::string(error_text));
		}
	}

	void IP_model_integrated::solve_problem(const Instance & data)
	{
		char error_text[CPXMESSAGEBUFSIZE];
		int status = 0;
		int solstat = 0;
		std::unique_ptr<double[]> solution_problem;
		double objval;

		/*status = CPXreadcopyprob(env, problem, "IP_model_integrated.lp", NULL); /// Dit werkt wel, maar probleem rechtstreeks geeft error
		if (status != 0)
		{
			CPXgeterrorstring(env, status, error_text);
			throw std::runtime_error("Error in function IP_model_routing::solve_problem(). \nCouldn't read problem from lp-file. \nReason: " + std::string(error_text));
		}*/

		// Set allowed computation time
		status = CPXsetdblparam(env, CPXPARAM_TimeLimit, _max_computation_time);
		if (status != 0)
		{
			CPXgeterrorstring(env, status, error_text);
			throw std::runtime_error("Error in function IP_model_allocation::solve_problem(). \nCouldn't set time limit. \nReason: " + std::string(error_text));
		}

		// Set tolerance gap
		status = CPXsetdblparam(env, CPXPARAM_MIP_Tolerances_MIPGap, _optimality_tolerance);
		if (status != 0)
		{
			CPXgeterrorstring(env, status, error_text);
			throw std::runtime_error("Error in function IP_model_allocation::solve_problem(). \nCouldn't set optimality tolerance. \nReason: " + std::string(error_text));
		}

		// Set emphasis feasibility
		status = CPXsetintparam(env, CPXPARAM_Emphasis_MIP, CPX_MIPEMPHASIS_FEASIBILITY);
		if (status != 0)
		{
			CPXgeterrorstring(env, status, error_text);
			throw std::runtime_error("Error in function IP_model_allocation::solve_problem(). \nCouldn't set search strategy. \nReason: " + std::string(error_text));
		}

		// Assign memory for solution
		const int numvar = CPXgetnumcols(env, problem);
		solution_problem = std::make_unique<double[]>(numvar);

		// Optimize the problem
		std::cout << "\n\nSolving the integrated problem ...";
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
			std::cout << "\nResult solve: " << solstat_text;

			if (solstat == CPXMIP_OPTIMAL || solstat == CPXMIP_OPTIMAL_TOL || solstat == CPXMIP_TIME_LIM_FEAS)
			{
				_objective_value = objval;
				std::cout << "\nObjective value = " << objval;
				std::cout << "\nElapsed time (s): " << elapsed_time_IP.count();

				const size_t nb_days_total = data.nb_days() * data.nb_weeks();
				const size_t nb_trucks = _max_nb_trucks;
				const size_t nb_segments = _max_nb_segments;
				const size_t nb_zones = data.nb_zones();
				const size_t nb_collection_points = data.nb_collection_points();
				const size_t nb_locations = nb_zones + 1 + nb_collection_points; // Order: Z1...Zn, depot, CP1...CPk
				const size_t max_visits = _max_visits;
 
				const int startindex_x_dvijk = 0;
				const int startindex_w_dvik = startindex_x_dvijk + nb_days_total * nb_trucks * nb_locations * nb_locations * nb_segments;
				const int startindex_y_dv = startindex_w_dvik + nb_days_total * nb_trucks * nb_zones * nb_segments;
				const int startindex_beta_dv = startindex_y_dv + nb_days_total * nb_trucks;
				const int startindex_z = startindex_beta_dv + nb_days_total * nb_trucks;
				const int startindex_ksi_di = startindex_z + 1;


				// lambdas to get variable indices
				auto index_x_dvijk = [startindex_x_dvijk, nb_trucks, nb_locations, nb_segments](int d, int v, int i, int j, int k) -> int {
					return startindex_x_dvijk + d * nb_trucks * nb_locations * nb_locations * nb_segments + v * nb_locations * nb_locations * nb_segments
						+ i * nb_locations * nb_segments + j * nb_segments + k;
					};
				auto index_w_dvik = [startindex_w_dvik, nb_trucks, nb_zones, nb_segments](int d, int v, int i_zone, int k) -> int {
					return startindex_w_dvik + d * nb_trucks * nb_zones * nb_segments + v * nb_zones * nb_segments + i_zone * nb_segments + k;
					};
				auto index_y_dv = [startindex_y_dv, nb_trucks](int d, int v) -> int {
					return startindex_y_dv + d * nb_trucks + v;
					};
				auto index_beta_dv = [startindex_beta_dv, nb_trucks](int d, int v) -> int {
					return startindex_beta_dv + d * nb_trucks + v;
					};
				auto index_ksi_di = [startindex_ksi_di, nb_zones](int d, int m) -> int {
					return startindex_ksi_di + d * nb_zones + m;
					};
				


				// Solution to file
				{
					try
					{
						std::ofstream solfile;
						std::string filename = data.name_instance() + "_solution.txt";
						solfile.open(filename);

						solfile << "Instance: " << data.name_instance();
						solfile << "\n\nMax computation time (s): " << _max_computation_time;
						solfile << "\nOptimality tolerance: " << _optimality_tolerance;
						solfile << "\nMax nb trucks: " << nb_trucks;
						solfile << "\nMax nb segments per route: " << nb_segments;

						struct Route
						{
							int day = 0;
							double hours = 0;
							std::vector<std::string> destinations; // e.g. depot, zoneA, CP1, zoneB, CP1, depot
							std::vector<int> amounts;	// amounts picked up at respective zones
							int nb_times_used = 1;
						};
						std::vector<Route> routes;


						// calculate routes
						for (int d = 0; d < nb_days_total; ++d) {
							for (int v = 0; v < nb_trucks; ++v) {
								if(solution_problem[index_y_dv(d,v)] > 0) {
									// route
									Route newroute;
									newroute.day = d;
									newroute.hours = solution_problem[index_beta_dv(d, v)];

									for (int m = 0; m < nb_zones; ++m) {
										for (int k = 0; k < nb_segments; ++k) {
											double wval = solution_problem[index_w_dvik(d, v, m, k)];
											if (wval > 0.001) {
												int wvalkg = static_cast<int>(wval * 1000 + 0.001);
												newroute.amounts.push_back(wvalkg);
											}
										}
									}

									for (int k = 0; k < nb_segments; ++k) {
										for (int i = 0; i < nb_locations; ++i) {
											for (int j = 0; j < nb_locations; ++j) {
												int xval = static_cast<int>(solution_problem[index_x_dvijk(d, v, i, j, k)] + 0.001);
												if (xval > 0) {
													std::string destination;
													if (j < nb_zones)
														destination = data.zone_name(j);
													else if (j == nb_zones)
														// destination = "depot";
														continue; // depot niet bijvoegen
													else
														destination = data.collection_point_name(j - nb_zones - 1);

													newroute.destinations.push_back(destination);
												}
											}
										}
									}

									// check if route already exists
									bool already_exists = false;
									for (auto&& er : routes) {
										if (er.destinations == newroute.destinations && er.amounts == newroute.amounts && er.day == newroute.day) {
											already_exists = true;
											++er.nb_times_used;
											break;
										}
									}
									if (!already_exists) {
										routes.push_back(newroute);
									}
								}
							}
						}


						// costs and trucks per day
						solfile << "\n\n\nKosten: " << objval;
						solfile << "\nz = " << solution_problem[startindex_z];
						solfile << "\n\nDag\tTrucks";
						for (int d = 0; d < nb_days_total; ++d) {
							int trucksday = 0;
							for (auto& r : routes) {
								if (r.day == d) {
									trucksday += r.nb_times_used;
								}
							}
							solfile << "\n" << d + 1 << "\t" << trucksday;
						}


						// write routes to file
						solfile << "\n\n\n\n\n\nDag\tRoute\tHoeveelheden\tAantal_keer_gebruikt";
						for (auto&& r : routes) {
							solfile << "\n" << r.day + 1 << "\t";
							for (size_t ii = 0; ii < r.destinations.size(); ++ii) {
								solfile << r.destinations[ii];
								if (ii < r.destinations.size() - 1)
									solfile << ";";
							}
							solfile << "\t";
							for (size_t ii = 0; ii < r.amounts.size(); ++ii) {
								solfile << r.amounts[ii];
								if (ii < r.amounts.size() - 1)
									solfile << ";";
							}
							solfile << "\t" << r.nb_times_used;
						}


						// calculate calendar
						solfile << "\n\n\n\n\n\nKalender\nZone\tMa\tDi\tWo\tDo\tVr\tMa\tDi\tWo\tDo\tVr";
						for (int m = 0; m < nb_zones; ++m) {
							solfile << "\n" << data.zone_name(m) << "\t";
							for (int d = 0; d < nb_days_total; ++d) {
								int ksi_di = static_cast<int>(solution_problem[index_ksi_di(d, m)] + 0.001);
								if (ksi_di > 0)
									solfile << "gft";
								solfile << "\t";
							}
						}


						solfile.flush();
					}
					catch (const std::exception& e)
					{
						std::cout << "\n\n\nError in function IP_model_routing::solve_problem()."
							<< "\nProblem with writing solution representation to file.\n"
							<< e.what()
							<< "\n\n\n";
					}
				}
			}
		}

	}

	void IP_model_integrated::clear_cplex()
	{
		int status = 0;
		char error_text[CPXMESSAGEBUFSIZE];

		// Free the problem
		status = CPXfreeprob(env, &problem);
		if (status != 0)
		{
			CPXgeterrorstring(env, status, error_text);
			throw std::runtime_error("Error in function IP_model_integrated::clear_cplex(). \nCouldn't free problem. \nReason: " + std::string(error_text));
		}

		// Close the cplex environment
		status = CPXcloseCPLEX(&env);
		if (status != 0)
		{
			CPXgeterrorstring(env, status, error_text);
			throw std::runtime_error("Error in function IP_model_integrated::clear_cplex(). \nCouldn't close cplex environment. \nReason: " + std::string(error_text));
		}
	}

	void IP_model_integrated::run(const Instance& data)
	{
		initialize_cplex();
		build_problem(data);
		solve_problem(data);
		clear_cplex();
	}

	///////////////////////////////////////////////////////////////////////////////////////////////

	void IP_model_integrated::run_fix_and_optimize(const Instance& data)
	{
		initialize_cplex();
		build_problem(data);

		fix_and_optimize(data);

		clear_cplex();
	}

	void IP_model_integrated::fix_and_optimize(const Instance& data)
	{
		// Data
		const size_t nb_days_total = data.nb_days() * data.nb_weeks();
		const size_t nb_trucks = _max_nb_trucks;
		const size_t nb_segments = _max_nb_segments;
		const size_t nb_zones = data.nb_zones();
		const size_t nb_collection_points = data.nb_collection_points();
		const size_t nb_locations = nb_zones + 1 + nb_collection_points; // Order: Z1...Zn, depot, CP1...CPk
		const size_t max_visits = _max_visits;

		std::random_device randdev;
		std::seed_seq seedseq{ randdev(),randdev(),randdev(),randdev(),randdev(),randdev(),randdev() };
		std::mt19937_64 engine(seedseq);

		std::chrono::duration<double, std::ratio<1, 1>> elapsed_time;
		auto start_time = std::chrono::system_clock::now();



		// Construct an initial feasible solution
		// Simply use CPLEX to find initial solution
		double best_objval = fao_initial_solution_cplex();
		std::cout << "\nInitial objective value: " << best_objval;
		


		// Choose a neighborhood, fix all variables to current solution except those in the neighborhood and reoptimize until time out
		// Set allowed computation time
		int status = CPXsetdblparam(env, CPXPARAM_TimeLimit, _fao_max_comptime_subproblem);
		if (status != 0)
		{
			char error_text[CPXMESSAGEBUFSIZE];
			CPXgeterrorstring(env, status, error_text);
			throw std::runtime_error("Error in function IP_model_integrated::fix_and_optimize(). \nCouldn't set time limit. \nReason: " + std::string(error_text));
		}
		// Set tolerance gap
		status = CPXsetdblparam(env, CPXPARAM_MIP_Tolerances_MIPGap, _optimality_tolerance);
		if (status != 0)
		{
			char error_text[CPXMESSAGEBUFSIZE];
			CPXgeterrorstring(env, status, error_text);
			throw std::runtime_error("Error in function IP_model_allocation::solve_problem(). \nCouldn't set optimality tolerance. \nReason: " + std::string(error_text));
		}



		// Parameters fix-and-optimize
		std::uniform_int_distribution<> dist_neighborhood(0, 2); // days / zones / vehicles
		std::uniform_int_distribution<> neighborhood_days(0, nb_days_total - 1);
		std::uniform_int_distribution<> neighborhood_zones(0, nb_zones - 1);
		std::uniform_int_distribution<> neighborhood_vehicles(0, nb_trucks - 1);
		size_t size_neighborhood_days = 2;
		size_t size_neighborhood_zones = 3;
		size_t size_neighborhood_vehicles = 1;
		int iterations_without_improvement_days = 0;
		int iterations_without_improvement_zones = 0;
		int iterations_without_improvement_vehicles = 0;
		int max_iterations_without_improvement = 1;



		// Start local search
		// First iterate over zones
		for (int i = 0; i < nb_zones; i += size_neighborhood_zones)
		{
			// Time check
			elapsed_time = std::chrono::system_clock::now() - start_time;
			std::cout << "\n\nElapsed time (s): " << elapsed_time.count();
			if (elapsed_time.count() > _max_computation_time)
				break;

			// Select zones and solve
			std::vector<int> zones_free;
			for (int ii = i; ii < i + size_neighborhood_zones; ++ii)
				zones_free.push_back(ii);
			double current_objval = fao_search_neighborhood_zones(data, zones_free);

			if (current_objval < best_objval)
			{
				best_objval = current_objval;
				for (int j = 0; j < CPXgetnumcols(env, problem); ++j)
					_fao_best_solution_cplex[j] = _fao_current_solution_cplex[j];
			}
		}

		// Then switch between neighborhoods
		int neighborhood_previous = -1;
		int neighborhood = 0;
		while (true)
		{
			// Time check
			elapsed_time = std::chrono::system_clock::now() - start_time;
			std::cout << "\n\nElapsed time (s): " << elapsed_time.count();
			if (elapsed_time.count() > _max_computation_time)
				break;

			// Choose neighborhood
			do
			{
				neighborhood = dist_neighborhood(engine);
			} while (neighborhood == neighborhood_previous);
			neighborhood_previous = neighborhood;

			// Search neighborhood
			if (neighborhood <= 0) // days
			{
				// Select days
				std::vector<int> days_free;
				{
					int added = 0;
					while (added < size_neighborhood_days)
					{
						int dayfree = neighborhood_days(engine);
						if (std::find(days_free.begin(), days_free.end(), dayfree) == days_free.end()) // not twice same day
						{
							days_free.push_back(dayfree);
							++added;
						}
					}
				}

				double current_objval = fao_search_neighborhood_days(data, days_free);
				++iterations_without_improvement_days;

				if (current_objval < best_objval)
				{
					best_objval = current_objval;
					for (int j = 0; j < CPXgetnumcols(env, problem); ++j)
						_fao_best_solution_cplex[j] = _fao_current_solution_cplex[j];

					iterations_without_improvement_days = 0;
				}

				if (iterations_without_improvement_days >= max_iterations_without_improvement)
				{
					if (size_neighborhood_days < nb_days_total)
					{
						++size_neighborhood_days;
						std::cout << "\nIncrease size neighborhood days to " << size_neighborhood_days;
						iterations_without_improvement_days = 0;
					}
				}
			}
			else if(neighborhood <= 1) // vehicles
			{
				// Select vehicles
				std::vector<int> vehicles_free;
				{
					int added = 0;
					while (added < size_neighborhood_vehicles)
					{
						int vehiclefree = neighborhood_vehicles(engine);
						if (std::find(vehicles_free.begin(), vehicles_free.end(), vehiclefree) == vehicles_free.end()) // not twice same vehicle
						{
							vehicles_free.push_back(vehiclefree);
							++added;
						}
					}
				}

				double current_objval = fao_search_neighborhood_vehicles(data, vehicles_free);
				++iterations_without_improvement_vehicles;

				if (current_objval < best_objval)
				{
					best_objval = current_objval;
					for (int j = 0; j < CPXgetnumcols(env, problem); ++j)
						_fao_best_solution_cplex[j] = _fao_current_solution_cplex[j];

					iterations_without_improvement_vehicles = 0;
				}

				if (iterations_without_improvement_vehicles >= max_iterations_without_improvement)
				{
					if (size_neighborhood_vehicles < nb_trucks)
					{
						++size_neighborhood_vehicles;
						std::cout << "\nIncrease size neighborhood vehicles to " << size_neighborhood_vehicles;
						iterations_without_improvement_vehicles = 0;
					}
				}
			}
			else if (neighborhood <= 2) // zones
			{
				// Select zones
				std::vector<int> zones_free;
				{
					int added = 0;
					while (added < size_neighborhood_zones)
					{
						int zonefree = neighborhood_zones(engine);
						if (std::find(zones_free.begin(), zones_free.end(), zonefree) == zones_free.end()) // not twice same zone
						{
							zones_free.push_back(zonefree);
							++added;
						}
					}
				}

				double current_objval = fao_search_neighborhood_zones(data, zones_free);
				++iterations_without_improvement_zones;

				if (current_objval < best_objval)
				{
					best_objval = current_objval;
					for (int j = 0; j < CPXgetnumcols(env, problem); ++j)
						_fao_best_solution_cplex[j] = _fao_current_solution_cplex[j];

					iterations_without_improvement_zones = 0;
				}

				if (iterations_without_improvement_zones >= max_iterations_without_improvement)
				{
					if (size_neighborhood_zones < nb_zones)
					{
						++size_neighborhood_zones;
						std::cout << "\nIncrease size neighborhood zones to " << size_neighborhood_zones;
						iterations_without_improvement_zones = 0;
					}
				}
			}
		}

		std::cout << "\n\nFixe-and-optimize: time limit reached ... ";
		std::cout << "\nBest objective value: " << best_objval;
		_objective_value = best_objval;

		// store solution
		fao_write_solution_file(data);
	}

	double IP_model_integrated::fao_initial_solution_cplex()
	{
		char error_text[CPXMESSAGEBUFSIZE];
		int status = 0;
		int solstat = 0;
		double objval;

		// Set emphasis feasibility
		status = CPXsetintparam(env, CPXPARAM_Emphasis_MIP, CPX_MIPEMPHASIS_FEASIBILITY);
		if (status != 0)
		{
			CPXgeterrorstring(env, status, error_text);
			throw std::runtime_error("Error in function IP_model_integrated::fao_initial_solution_cplex(). \nCouldn't set search strategy. \nReason: " + std::string(error_text));
		}

		// CPLEX stops after first solution
		status = CPXsetintparam(env, CPXPARAM_MIP_Limits_Solutions, 1);
		if (status != 0)
		{
			CPXgeterrorstring(env, status, error_text);
			throw std::runtime_error("Error in function IP_model_integrated::fao_initial_solution_cplex(). \nCouldn't change param MIP_Limits_Solutions. \nReason: " + std::string(error_text));
		}

		// Obtain solution 
		// Assign memory for solution
		const int numvar = CPXgetnumcols(env, problem);
		_fao_current_solution_cplex = std::make_unique<double[]>(numvar);
		_fao_best_solution_cplex = std::make_unique<double[]>(numvar);

		// Optimize the problem
		std::cout << "\n\nFix-and-optimize: finding initial solution using CPLEX ...";
		auto start_time = std::chrono::system_clock::now();

		status = CPXmipopt(env, problem);
		if (status != 0)
		{
			CPXgeterrorstring(env, status, error_text);
			throw std::runtime_error("Error in function IP_model_integrated::fao_initial_solution_cplex(). \nCPXmipopt failed. \nReason: " + std::string(error_text));
		}

		std::chrono::duration<double, std::ratio<1, 1>> elapsed_time_IP = std::chrono::system_clock::now() - start_time;


		// Get the solution
		status = CPXsolution(env, problem, &solstat, &objval, _fao_current_solution_cplex.get(), NULL, NULL, NULL);
		if (status != 0)
		{
			CPXgeterrorstring(env, status, error_text);
			throw std::runtime_error("Error in function IP_model_integrated::fao_initial_solution_cplex(). \nCPXsolution failed. \nReason: " + std::string(error_text));
		}

		// Reset solution limit parameter to default value
		status = CPXsetintparam(env, CPXPARAM_MIP_Limits_Solutions, CPXINT_MAX);
		if (status != 0)
		{
			CPXgeterrorstring(env, status, error_text);
			throw std::runtime_error("Error in function IP_model_integrated::fao_initial_solution_cplex(). \nCouldn't change param MIP_Limits_Solutions. \nReason: " + std::string(error_text));
		}

		if (solstat != CPXMIP_SOL_LIM)
			throw std::runtime_error("Fix-and-optimize: Did not find a feasible start solution.");

		// save best solution
		for (int i = 0; i < CPXgetnumcols(env, problem); ++i)
			_fao_best_solution_cplex[i] = _fao_current_solution_cplex[i];

		return objval;
	}

	double IP_model_integrated::fao_search_neighborhood_days(const Instance& data, const std::vector<int>& days_free)
	{
		// Number of constraints before fixing variables
		const int nb_rows_default = CPXgetnumrows(env, problem);

		// Data
		const size_t nb_days_total = data.nb_days() * data.nb_weeks();
		const size_t nb_trucks = _max_nb_trucks;
		const size_t nb_segments = _max_nb_segments;
		const size_t nb_zones = data.nb_zones();
		const size_t nb_collection_points = data.nb_collection_points();
		const size_t nb_locations = nb_zones + 1 + nb_collection_points; // Order: Z1...Zn, depot, CP1...CPk

		const int startindex_x_dvijk = 0;
		const int startindex_w_dvik = startindex_x_dvijk + nb_days_total * nb_trucks * nb_locations * nb_locations * nb_segments;
		const int startindex_y_dv = startindex_w_dvik + nb_days_total * nb_trucks * nb_zones * nb_segments;
		const int startindex_beta_dv = startindex_y_dv + nb_days_total * nb_trucks;
		const int startindex_z = startindex_beta_dv + nb_days_total * nb_trucks;
		const int startindex_ksi_di = startindex_z + 1;

		// Lambdas to get variable indices
		auto index_y_dv = [startindex_y_dv, nb_trucks](int d, int v) -> int {
			return startindex_y_dv + d * nb_trucks + v;
			};
		auto index_ksi_di = [startindex_ksi_di, nb_zones](int d, int m) -> int {
			return startindex_ksi_di + d * nb_zones + m;
			};

		// Fix current solution (except neighborhood)
		for (int d = 0; d < nb_days_total; ++d)
		{
			if (std::find(days_free.begin(), days_free.end(), d) == days_free.end()) // not days in neighborhood
			{
				for (int v = 0; v < nb_trucks; ++v)
				{
					// y_dv
					{
						const int index = index_y_dv(d, v);
						const double value = _fao_best_solution_cplex[index];
						fao_fix_variable(index, value);
					}
				}

				// ksi_di
				for (int i = 0; i < nb_zones; ++i) // enkel zones
				{
					const int index = index_ksi_di(d, i);
					const double value = _fao_best_solution_cplex[index];
					fao_fix_variable(index, value);
				}
			}
		}
		
#if _DEBUG
		// Write to file
		{
			int status = CPXwriteprob(env, problem, "IP_model_integrated.lp", NULL);
			if (status != 0)
			{
				char error_text[CPXMESSAGEBUFSIZE];
				CPXgeterrorstring(env, status, error_text);
				throw std::runtime_error("Error in function IP_model_integrated::fao_search_neighborhood_days(). \nCouldn't write problem to lp-file. \nReason: " + std::string(error_text));
			}
		}
#endif

		// Solve the problem and get the solution
		std::cout << "\nSolving neighborhood days ...";

		int status = CPXmipopt(env, problem);
		if (status != 0)
		{
			char error_text[CPXMESSAGEBUFSIZE];
			CPXgeterrorstring(env, status, error_text);
			throw std::runtime_error("Error in function IP_model_integrated::fao_search_neighborhood_days(). \nCPXmipopt failed. \nReason: " + std::string(error_text));
		}

		int solstat;
		double objval;
		status = CPXsolution(env, problem, &solstat, &objval, _fao_current_solution_cplex.get(), NULL, NULL, NULL);
		if (status != 0)
		{
			std::cout << "\nNo solution exists ... ";
			objval = 1e20;
		}
		else
		{
			char solstat_text[CPXMESSAGEBUFSIZE];
			auto p = CPXgetstatstring(env, solstat, solstat_text);
			std::cout << "\nSolstat: " << solstat_text;
			std::cout << "\nObjective value: " << objval;

			if (solstat != CPXMIP_OPTIMAL && solstat != CPXMIP_OPTIMAL_TOL && solstat != CPXMIP_TIME_LIM_FEAS)
				objval = 1e20;
		}

		// Delete added constraints
		if (CPXgetnumrows(env, problem) > nb_rows_default) // enkel indien constraints toegevoegd (als neighborhood heel probleem is, wordt er niets toegevoegd)
		{
			status = CPXdelrows(env, problem, nb_rows_default, CPXgetnumrows(env, problem) - 1);
			if (status != 0)
			{
				char error_text[CPXMESSAGEBUFSIZE];
				CPXgeterrorstring(env, status, error_text);
				throw std::runtime_error("Error in function IP_model_integrated::fao_search_neighborhood_days(). \nCoudln't delete added constraints fixed variables. \nReason: " + std::string(error_text));
			}
		}

		return objval;
	}

	double IP_model_integrated::fao_search_neighborhood_zones(const Instance& data, const std::vector<int>& zones_free)
	{
		// Number of constraints before fixing variables
		const int nb_rows_default = CPXgetnumrows(env, problem);

		// Data
		const size_t nb_days_total = data.nb_days() * data.nb_weeks();
		const size_t nb_trucks = _max_nb_trucks;
		const size_t nb_segments = _max_nb_segments;
		const size_t nb_zones = data.nb_zones();
		const size_t nb_collection_points = data.nb_collection_points();
		const size_t nb_locations = nb_zones + 1 + nb_collection_points; // Order: Z1...Zn, depot, CP1...CPk

		const int startindex_x_dvijk = 0;
		const int startindex_w_dvik = startindex_x_dvijk + nb_days_total * nb_trucks * nb_locations * nb_locations * nb_segments;
		const int startindex_y_dv = startindex_w_dvik + nb_days_total * nb_trucks * nb_zones * nb_segments;
		const int startindex_beta_dv = startindex_y_dv + nb_days_total * nb_trucks;
		const int startindex_z = startindex_beta_dv + nb_days_total * nb_trucks;
		const int startindex_ksi_di = startindex_z + 1;

		// Lambdas to get variable indices
		auto index_ksi_di = [startindex_ksi_di, nb_zones](int d, int m) -> int {
			return startindex_ksi_di + d * nb_zones + m;
			};

		// Fix current solution (except neighborhood)
		for (int d = 0; d < nb_days_total; ++d)
		{
			// ksi_di
			for (int i = 0; i < nb_zones; ++i) // enkel zones
			{
				if (std::find(zones_free.begin(), zones_free.end(), i) == zones_free.end()) // not zones in neighborhood
				{
					const int index = index_ksi_di(d, i);
					const double value = _fao_best_solution_cplex[index];
					fao_fix_variable(index, value);
				}
			}
		}

#if _DEBUG
		// Write to file
		{
			int status = CPXwriteprob(env, problem, "IP_model_integrated.lp", NULL);
			if (status != 0)
			{
				char error_text[CPXMESSAGEBUFSIZE];
				CPXgeterrorstring(env, status, error_text);
				throw std::runtime_error("Error in function IP_model_integrated::fao_search_neighborhood_zones(). \nCouldn't write problem to lp-file. \nReason: " + std::string(error_text));
			}
		}
#endif

		// Solve the problem and get the solution
		std::cout << "\nSolving neighborhood zones ...";

		int status = CPXmipopt(env, problem);
		if (status != 0)
		{
			char error_text[CPXMESSAGEBUFSIZE];
			CPXgeterrorstring(env, status, error_text);
			throw std::runtime_error("Error in function IP_model_integrated::fao_search_neighborhood_zones(). \nCPXmipopt failed. \nReason: " + std::string(error_text));
		}

		int solstat;
		double objval;
		status = CPXsolution(env, problem, &solstat, &objval, _fao_current_solution_cplex.get(), NULL, NULL, NULL);
		if (status != 0)
		{
			std::cout << "\nNo solution exists ... ";
			objval = 1e20;
		}
		else
		{
			char solstat_text[CPXMESSAGEBUFSIZE];
			auto p = CPXgetstatstring(env, solstat, solstat_text);
			std::cout << "\nSolstat: " << solstat_text;
			std::cout << "\nObjective value: " << objval;

			if (solstat != CPXMIP_OPTIMAL && solstat != CPXMIP_OPTIMAL_TOL && solstat != CPXMIP_TIME_LIM_FEAS)
				objval = 1e20;
		}

		// Delete added constraints
		if (CPXgetnumrows(env, problem) > nb_rows_default) // enkel indien constraints toegevoegd (als neighborhood heel probleem is, wordt er niets toegevoegd)
		{
			status = CPXdelrows(env, problem, nb_rows_default, CPXgetnumrows(env, problem) - 1);
			if (status != 0)
			{
				char error_text[CPXMESSAGEBUFSIZE];
				CPXgeterrorstring(env, status, error_text);
				throw std::runtime_error("Error in function IP_model_integrated::fao_search_neighborhood_zones(). \nCoudln't delete added constraints fixed variables. \nReason: " + std::string(error_text));
			}
		}

		return objval;
	}

	double IP_model_integrated::fao_search_neighborhood_vehicles(const Instance& data, const std::vector<int>& vehicles_free)
	{
		// Number of constraints before fixing variables
		const int nb_rows_default = CPXgetnumrows(env, problem);

		// Data
		const size_t nb_days_total = data.nb_days() * data.nb_weeks();
		const size_t nb_trucks = _max_nb_trucks;
		const size_t nb_segments = _max_nb_segments;
		const size_t nb_zones = data.nb_zones();
		const size_t nb_collection_points = data.nb_collection_points();
		const size_t nb_locations = nb_zones + 1 + nb_collection_points; // Order: Z1...Zn, depot, CP1...CPk

		const int startindex_x_dvijk = 0;
		const int startindex_w_dvik = startindex_x_dvijk + nb_days_total * nb_trucks * nb_locations * nb_locations * nb_segments;
		const int startindex_y_dv = startindex_w_dvik + nb_days_total * nb_trucks * nb_zones * nb_segments;
		const int startindex_beta_dv = startindex_y_dv + nb_days_total * nb_trucks;
		const int startindex_z = startindex_beta_dv + nb_days_total * nb_trucks;
		const int startindex_ksi_di = startindex_z + 1;

		// Lambdas to get variable indices
		auto index_y_dv = [startindex_y_dv, nb_trucks](int d, int v) -> int {
			return startindex_y_dv + d * nb_trucks + v;
			};

		// Fix current solution (except neighborhood)
		for (int d = 0; d < nb_days_total; ++d)
		{
			for (int v = 0; v < nb_trucks; ++v)
			{
				if (std::find(vehicles_free.begin(), vehicles_free.end(), v) == vehicles_free.end()) // not vehicles in neighborhood
				{
					const int index = index_y_dv(d, v);
					const double value = _fao_best_solution_cplex[index];
					fao_fix_variable(index, value);
				}
			}
		}

#if _DEBUG
		// Write to file
		{
			int status = CPXwriteprob(env, problem, "IP_model_integrated.lp", NULL);
			if (status != 0)
			{
				char error_text[CPXMESSAGEBUFSIZE];
				CPXgeterrorstring(env, status, error_text);
				throw std::runtime_error("Error in function IP_model_integrated::fao_search_neighborhood_vehicles(). \nCouldn't write problem to lp-file. \nReason: " + std::string(error_text));
			}
		}
#endif

		// Solve the problem and get the solution
		std::cout << "\nSolving neighborhood vehicles ...";

		int status = CPXmipopt(env, problem);
		if (status != 0)
		{
			char error_text[CPXMESSAGEBUFSIZE];
			CPXgeterrorstring(env, status, error_text);
			throw std::runtime_error("Error in function IP_model_integrated::fao_search_neighborhood_vehicles(). \nCPXmipopt failed. \nReason: " + std::string(error_text));
		}

		int solstat;
		double objval;
		status = CPXsolution(env, problem, &solstat, &objval, _fao_current_solution_cplex.get(), NULL, NULL, NULL);
		if (status != 0)
		{
			std::cout << "\nNo solution exists ... ";
			objval = 1e20;
		}
		else
		{
			char solstat_text[CPXMESSAGEBUFSIZE];
			auto p = CPXgetstatstring(env, solstat, solstat_text);
			std::cout << "\nSolstat: " << solstat_text;
			std::cout << "\nObjective value: " << objval;

			if (solstat != CPXMIP_OPTIMAL && solstat != CPXMIP_OPTIMAL_TOL && solstat != CPXMIP_TIME_LIM_FEAS)
				objval = 1e20;
		}

		// Delete added constraints
		if (CPXgetnumrows(env, problem) > nb_rows_default) // enkel indien constraints toegevoegd (als neighborhood heel probleem is, wordt er niets toegevoegd)
		{
			status = CPXdelrows(env, problem, nb_rows_default, CPXgetnumrows(env, problem) - 1);
			if (status != 0)
			{
				char error_text[CPXMESSAGEBUFSIZE];
				CPXgeterrorstring(env, status, error_text);
				throw std::runtime_error("Error in function IP_model_integrated::fao_search_neighborhood_vehicles(). \nCoudln't delete added constraints fixed variables. \nReason: " + std::string(error_text));
			}
		}

		return objval;
	}

	void IP_model_integrated::fao_fix_variable(size_t index_variable, int value)
	{
		char error_text[CPXMESSAGEBUFSIZE];
		int status = 0;
		double rhs[1];			// Right-hand side constraints
		char sense[1];			// Sign of constraint
		int nonzeroes = 0;		// To calculate number of nonzero coefficients in each constraint
		int matbeg[1];			// Begin position of the constraint
		int matind[1];			// Position of each element in constraint matrix
		double matval[1];		// Value of each element in constraint matrix

		matbeg[0] = 0;

		rhs[0] = value;
		sense[0] = 'E';

		nonzeroes = 0;
		matind[nonzeroes] = index_variable;
		matval[nonzeroes] = 1;
		++nonzeroes;

		status = CPXaddrows(env, problem, 0, 1, nonzeroes, rhs, sense, matbeg, matind, matval, NULL, NULL);
		if (status != 0)
		{
			CPXgeterrorstring(env, status, error_text);
			throw std::runtime_error("Error in function IP_model_routing::fao_fix_variable(). \nCouldn't add constraint. \nReason: " + std::string(error_text));
		}

		// change name of constraint
		int numrows = CPXgetnumrows(env, problem);
		std::string conname = "fix_variable_c" + std::to_string(numrows + 1);
		status = CPXchgname(env, problem, 'r', numrows - 1, conname.c_str());
		if (status != 0)
		{
			CPXgeterrorstring(env, status, error_text);
			throw std::runtime_error("Error in function IP_model_routing::fao_fix_variable(). \nCouldn't change constraint name. \nReason: " + std::string(error_text));
		}
	}

	void IP_model_integrated::fao_write_solution_file(const Instance& data)
	{
		const size_t nb_days_total = data.nb_days() * data.nb_weeks();
		const size_t nb_trucks = _max_nb_trucks;
		const size_t nb_segments = _max_nb_segments;
		const size_t nb_zones = data.nb_zones();
		const size_t nb_collection_points = data.nb_collection_points();
		const size_t nb_locations = nb_zones + 1 + nb_collection_points; // Order: Z1...Zn, depot, CP1...CPk
		const size_t max_visits = _max_visits;

		const int startindex_x_dvijk = 0;
		const int startindex_w_dvik = startindex_x_dvijk + nb_days_total * nb_trucks * nb_locations * nb_locations * nb_segments;
		const int startindex_y_dv = startindex_w_dvik + nb_days_total * nb_trucks * nb_zones * nb_segments;
		const int startindex_beta_dv = startindex_y_dv + nb_days_total * nb_trucks;
		const int startindex_z = startindex_beta_dv + nb_days_total * nb_trucks;
		const int startindex_ksi_di = startindex_z + 1;


		// lambdas to get variable indices
		auto index_x_dvijk = [startindex_x_dvijk, nb_trucks, nb_locations, nb_segments](int d, int v, int i, int j, int k) -> int {
			return startindex_x_dvijk + d * nb_trucks * nb_locations * nb_locations * nb_segments + v * nb_locations * nb_locations * nb_segments
				+ i * nb_locations * nb_segments + j * nb_segments + k;
			};
		auto index_w_dvik = [startindex_w_dvik, nb_trucks, nb_zones, nb_segments](int d, int v, int i_zone, int k) -> int {
			return startindex_w_dvik + d * nb_trucks * nb_zones * nb_segments + v * nb_zones * nb_segments + i_zone * nb_segments + k;
			};
		auto index_y_dv = [startindex_y_dv, nb_trucks](int d, int v) -> int {
			return startindex_y_dv + d * nb_trucks + v;
			};
		auto index_beta_dv = [startindex_beta_dv, nb_trucks](int d, int v) -> int {
			return startindex_beta_dv + d * nb_trucks + v;
			};
		auto index_ksi_di = [startindex_ksi_di, nb_zones](int d, int m) -> int {
			return startindex_ksi_di + d * nb_zones + m;
			};



		// Solution to file
		{
			try
			{
				std::ofstream solfile;
				std::string filename = data.name_instance() + "_solution.txt";
				solfile.open(filename);

				solfile << "Instance: " << data.name_instance();
				solfile << "\n\nFix-and-optimize";
				solfile << "\nMax computation time (s): " << _max_computation_time;
				solfile << "\nMax computation time subproblem (s): " << _fao_max_comptime_subproblem;
				solfile << "\nOptimality tolerance: " << _optimality_tolerance;
				solfile << "\nMax nb trucks: " << nb_trucks;
				solfile << "\nMax nb segments per route: " << nb_segments;
				solfile << "\nMax nb visits: " << _max_visits;

				struct Route
				{
					int day = 0;
					double hours = 0;
					std::vector<std::string> destinations; // e.g. depot, zoneA, CP1, zoneB, CP1, depot
					std::vector<int> amounts;	// amounts picked up at respective zones
					int nb_times_used = 1;
				};
				std::vector<Route> routes;


				// calculate routes
				for (int d = 0; d < nb_days_total; ++d) {
					for (int v = 0; v < nb_trucks; ++v) {
						if (_fao_best_solution_cplex[index_y_dv(d, v)] > 0) {
							// route
							Route newroute;
							newroute.day = d;
							newroute.hours = _fao_best_solution_cplex[index_beta_dv(d, v)];

							for (int m = 0; m < nb_zones; ++m) {
								for (int k = 0; k < nb_segments; ++k) {
									double wval = _fao_best_solution_cplex[index_w_dvik(d, v, m, k)];
									if (wval > 0.001) {
										int wvalkg = static_cast<int>(wval * 1000 + 0.001);
										newroute.amounts.push_back(wvalkg);
									}
								}
							}

							for (int k = 0; k < nb_segments; ++k) {
								for (int i = 0; i < nb_locations; ++i) {
									for (int j = 0; j < nb_locations; ++j) {
										int xval = static_cast<int>(_fao_best_solution_cplex[index_x_dvijk(d, v, i, j, k)] + 0.001);
										if (xval > 0) {
											std::string destination;
											if (j < nb_zones)
												destination = data.zone_name(j);
											else if (j == nb_zones)
												// destination = "depot";
												continue; // depot niet bijvoegen
											else
												destination = data.collection_point_name(j - nb_zones - 1);

											newroute.destinations.push_back(destination);
										}
									}
								}
							}

							// check if route already exists
							bool already_exists = false;
							for (auto&& er : routes) {
								if (er.destinations == newroute.destinations && er.amounts == newroute.amounts && er.day == newroute.day) {
									already_exists = true;
									++er.nb_times_used;
									break;
								}
							}
							if (!already_exists) {
								routes.push_back(newroute);
							}
						}
					}
				}


				// costs and trucks per day
				solfile << "\n\n\nKosten: " << _objective_value;
				solfile << "\nz = " << _fao_best_solution_cplex[startindex_z];
				solfile << "\n\nDag\tTrucks";
				for (int d = 0; d < nb_days_total; ++d) {
					int trucksday = 0;
					for (auto& r : routes) {
						if (r.day == d) {
							trucksday += r.nb_times_used;
						}
					}
					solfile << "\n" << d + 1 << "\t" << trucksday;
				}


				// write routes to file
				solfile << "\n\n\n\n\n\nDag\tRoute\tHoeveelheden\tAantal_keer_gebruikt";
				for (auto&& r : routes) {
					solfile << "\n" << r.day + 1 << "\t";
					for (size_t ii = 0; ii < r.destinations.size(); ++ii) {
						solfile << r.destinations[ii];
						if (ii < r.destinations.size() - 1)
							solfile << ";";
					}
					solfile << "\t";
					for (size_t ii = 0; ii < r.amounts.size(); ++ii) {
						solfile << r.amounts[ii];
						if (ii < r.amounts.size() - 1)
							solfile << ";";
					}
					solfile << "\t" << r.nb_times_used;
				}


				// calculate calendar
				solfile << "\n\n\n\n\n\nKalender\nZone\tMa\tDi\tWo\tDo\tVr\tMa\tDi\tWo\tDo\tVr";
				for (int m = 0; m < nb_zones; ++m) {
					solfile << "\n" << data.zone_name(m) << "\t";
					for (int d = 0; d < nb_days_total; ++d) {
						int ksi_di = static_cast<int>(_fao_best_solution_cplex[index_ksi_di(d, m)] + 0.001);
						if (ksi_di > 0)
							solfile << "gft";
						solfile << "\t";
					}
				}


				solfile.flush();
			}
			catch (const std::exception& e)
			{
				std::cout << "\n\n\nError in function IP_model_routing::solve_problem()."
					<< "\nProblem with writing solution representation to file.\n"
					<< e.what()
					<< "\n\n\n";
			}
		}
	}
}