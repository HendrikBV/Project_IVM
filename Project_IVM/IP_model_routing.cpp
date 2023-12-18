/*
	Copyright (c) 2023 Hendrik Vermuyten

	Licensed under the Apache License, Version 2.0 (the "License");
	you may not use this file except in compliance with the License.
	You may obtain a copy of the License at

	http://www.apache.org/licenses/LICENSE-2.0
*/



#include "models.h"
#include "data.h"
#include <stdexcept>
#include <memory>
#include <iostream>
#include <chrono>
#include <fstream>
#include <cassert>



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

	size_t IP_model_routing::_index_x_qvijk(const Instance& data, int q, int v, int i, int j, int k) const
	{
		const size_t nb_locations = data.nb_zones() + 1 + data.nb_collection_points();

		assert(q < data.nb_truck_types());
		assert(v < _max_nb_trucks);
		assert(i < nb_locations);
		assert(j < nb_locations);
		assert(k < _max_nb_segments);

		return q * _max_nb_trucks * nb_locations * nb_locations * _max_nb_segments + v * nb_locations * nb_locations * _max_nb_segments
			+ i * nb_locations * _max_nb_segments + j * _max_nb_segments + k;
	}

	size_t IP_model_routing::_index_w_tqvik(const Instance& data, int t, int q, int v, int i_zone, int k) const
	{
		const size_t nb_locations = data.nb_zones() + 1 + data.nb_collection_points();
		const size_t startindex = data.nb_truck_types() * _max_nb_trucks * nb_locations * nb_locations * _max_nb_segments;

		assert(t < data.nb_waste_types());
		assert(q < data.nb_truck_types());
		assert(v < _max_nb_trucks);
		assert(i_zone < data.nb_zones());
		assert(k < _max_nb_segments);

		return startindex + t * data.nb_truck_types() * _max_nb_trucks * data.nb_zones() * _max_nb_segments + q * _max_nb_trucks * data.nb_zones() * _max_nb_segments
			+ v * data.nb_zones() * _max_nb_segments + i_zone * _max_nb_segments + k;
	}

	size_t IP_model_routing::_index_y_qv(const Instance& data, int q, int v) const
	{
		const size_t nb_locations = data.nb_zones() + 1 + data.nb_collection_points();
		const size_t startindex = (data.nb_truck_types() * _max_nb_trucks * nb_locations * nb_locations * _max_nb_segments)
			+ (data.nb_waste_types() * data.nb_truck_types() * _max_nb_trucks * data.nb_zones() * _max_nb_segments);

		assert(q < data.nb_truck_types());
		assert(v < _max_nb_trucks);

		return startindex + q * _max_nb_trucks + v;
	}

	size_t IP_model_routing::_index_beta_qv(const Instance& data, int q, int v) const
	{
		const size_t nb_locations = data.nb_zones() + 1 + data.nb_collection_points();
		const size_t startindex = (data.nb_truck_types() * _max_nb_trucks * nb_locations * nb_locations * _max_nb_segments)
			+ (data.nb_waste_types() * data.nb_truck_types() * _max_nb_trucks * data.nb_zones() * _max_nb_segments)
			+ (data.nb_truck_types() * _max_nb_trucks);

		assert(q < data.nb_truck_types());
		assert(v < _max_nb_trucks);

		return startindex + q * _max_nb_trucks + v;
	}

	void IP_model_routing::build_problem(const Instance& data, size_t day)
	{
		std::cout << "\n\n\nStart building the routing model for day " << day + 1;
		auto start_time = std::chrono::system_clock::now();

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



		// add variables
		int nb_variables = -1;

		// variable x_qvijk   day is given
		for (int q = 0; q < nb_truck_types; ++q)
		{
			for (int v = 0; v < _max_nb_trucks; ++v)
			{
				for (int i = 0; i < nb_locations; ++i)
				{
					for (int j = 0; j < nb_locations; ++j)
					{
						for (int k = 0; k < _max_nb_segments; ++k)
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
							std::string varname = "x_" + std::to_string(q + 1) + "_" + std::to_string(v + 1) + "_"
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

		// variable w_tqvik   day is given
		for (int t = 0; t < nb_waste_types; ++t)
		{
			for (int q = 0; q < nb_truck_types; ++q)
			{
				for (int v = 0; v < _max_nb_trucks; ++v)
				{
					for (int m = 0; m < nb_zones; ++m) // enkel voor zones
					{
						for (int k = 0; k < _max_nb_segments; ++k)
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
							std::string varname = "w_" + std::to_string(t + 1) + "_" + std::to_string(q + 1) + "_"
								+ std::to_string(v + 1) + "_" + std::to_string(m + 1) + "_" + std::to_string(k + 1);
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

		// variable y_qv   day is given
		for (int q = 0; q < nb_truck_types; ++q)
		{
			for (int v = 0; v < _max_nb_trucks; ++v)
			{
				++nb_variables;

				if (_include_nb_truck_objective)
					obj[0] = data.fixed_costs(q);
				else
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
				std::string varname = "y_" + std::to_string(q + 1) + "_" + std::to_string(v + 1);
				status = CPXchgname(env, problem, 'c', nb_variables, varname.c_str());
				if (status != 0)
				{
					CPXgeterrorstring(env, status, error_text);
					throw std::runtime_error("Error in function IP_model_routing::build_problem(). \nCouldn't change variable name. \nReason: " + std::string(error_text));
				}
			}
		}

		// variable beta_qv   day is given
		for (int q = 0; q < nb_truck_types; ++q)
		{
			for (int v = 0; v < _max_nb_trucks; ++v)
			{
				++nb_variables;

				obj[0] = data.operating_costs(q);
				lb[0] = 0;

				status = CPXnewcols(env, problem, 1, obj, lb, NULL, NULL, NULL);
				if (status != 0)
				{
					CPXgeterrorstring(env, status, error_text);
					throw std::runtime_error("Error in function IP_model_routing::build_problem(). \nCouldn't add variable. \nReason: " + std::string(error_text));
				}

				// change variable name
				std::string varname = "beta_" + std::to_string(q + 1) + "_" + std::to_string(v + 1);
				status = CPXchgname(env, problem, 'c', nb_variables, varname.c_str());
				if (status != 0)
				{
					CPXgeterrorstring(env, status, error_text);
					throw std::runtime_error("Error in function IP_model_routing::build_problem(). \nCouldn't change variable name. \nReason: " + std::string(error_text));
				}
			}
		}

		nb_variables = CPXgetnumcols(env, problem);



		// add constraints
		int nb_constraints = -1;

		// 1: beta_qv - sum(i,j,k) tau_D_ij*x_qvijk - sum(t,i,k) tau_P_ti*w_tqvik - sum(i,j,k) tau_U*x_qvijk == 0   forall q,v
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
				{
					const size_t index = _index_beta_qv(data, q, v);
					if (index >= nb_variables)
						throw std::runtime_error("Error in function IP_model_routing::build_problem(). Index variable exceeds range");

					matind[nonzeroes] = index;
					matval[nonzeroes] = 1;
					++nonzeroes;
				}

				// -sum(i,j,k) tau_D_ij* x_qvijk
				for (int i = 0; i < nb_locations; ++i)
				{
					for (int j = 0; j < nb_locations; ++j)
					{
						for (int k = 0; k < _max_nb_segments; ++k)
						{
							const size_t index = _index_x_qvijk(data, q, v, i, j, k);
							if (index >= nb_variables)
								throw std::runtime_error("Error in function IP_model_routing::build_problem(). Index variable exceeds range");

							if (i < nb_zones && j == nb_zones) { // i == zone, j == depot
								matind[nonzeroes] = index;
								matval[nonzeroes] = -data.time_driving_zone_depot(i);
								++nonzeroes;
							}
							else if (i < nb_zones && j > nb_zones) { // i == zone, j == collection point
								matind[nonzeroes] = index;
								const std::string& cp = data.collection_point(j - nb_zones - 1);
								matval[nonzeroes] = -data.time_driving_zone_collectionpoint(i, cp);
								++nonzeroes;
							}
							else if (i == nb_zones && j < nb_zones) { // i == depot, j == zone
								matind[nonzeroes] = index;
								matval[nonzeroes] = -data.time_driving_zone_depot(j);
								++nonzeroes;
							}
							else if (i > nb_zones && j < nb_zones) { // i == collection point, j == zone
								matind[nonzeroes] = index;
								const std::string& cp = data.collection_point(i - nb_zones - 1);
								matval[nonzeroes] = -data.time_driving_zone_collectionpoint(j, cp);
								++nonzeroes;
							}
						}
					}
				}

				// -sum(t,m,k) tau_P_ti*w_tqvik
				for (int t = 0; t < nb_waste_types; ++t)
				{
					for (int m = 0; m < nb_zones; ++m) // enkel zones
					{
						for (int k = 0; k < _max_nb_segments; ++k)
						{
							const size_t index = _index_w_tqvik(data, t, q, v, m, k);
							if (index >= nb_variables)
								throw std::runtime_error("Error in function IP_model_routing::build_problem(). Index variable exceeds range");

							matind[nonzeroes] = index;
							const std::string& waste_type = data.waste_type(t);
							matval[nonzeroes] = -data.time_pickup(m, waste_type);
							++nonzeroes;
						}
					}
				}

				// sum(i,j,k) -tau_U * x_qvijk
				for (int i = 0; i < nb_locations; ++i)
				{
					for (int j = 0; j < nb_locations; ++j)
					{
						for (int k = 0; k < _max_nb_segments; ++k)
						{
							const size_t index = _index_x_qvijk(data, q, v, i, j, k);
							if (index >= nb_variables)
								throw std::runtime_error("Error in function IP_model_routing::build_problem(). Index variable exceeds range");


							matind[nonzeroes] = index;
							const std::string& waste_type = data.waste_type(0); // MOMENTEEL ZELFDE VERONDERSTELD
							matval[nonzeroes] = -data.time_unloading(waste_type);
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
				std::string conname = "c1_" + std::to_string(q + 1) + "_" + std::to_string(v + 1);
				status = CPXchgname(env, problem, 'r', nb_constraints, conname.c_str());
				if (status != 0)
				{
					CPXgeterrorstring(env, status, error_text);
					throw std::runtime_error("Error in function IP_model_routing::build_problem(). \nCouldn't change constraint name. \nReason: " + std::string(error_text));
				}
			}
		}

		// 2: beta_qv <= T_q  forall q,v
		for (int q = 0; q < nb_truck_types; ++q)
		{
			for (int v = 0; v < _max_nb_trucks; ++v)
			{
				++nb_constraints;

				rhs[0] = data.max_driving_time(q);
				sense[0] = 'L';
				matbeg[0] = 0;

				nonzeroes = 0;

				// beta_qv
				{
					const size_t index = _index_beta_qv(data, q, v);
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
				std::string conname = "c2_" + std::to_string(q + 1) + "_" + std::to_string(v + 1);
				status = CPXchgname(env, problem, 'r', nb_constraints, conname.c_str());
				if (status != 0)
				{
					CPXgeterrorstring(env, status, error_text);
					throw std::runtime_error("Error in function IP_model_routing::build_problem(). \nCouldn't change constraint name. \nReason: " + std::string(error_text));
				}
			}
		}

		// 3: w_tqvik <= L_tq sum(j) x_qvjik   forall t,q,v,i,k
		for (int t = 0; t < nb_waste_types; ++t)
		{
			for (int q = 0; q < nb_truck_types; ++q)
			{
				for (int v = 0; v < _max_nb_trucks; ++v)
				{
					for (int m = 0; m < nb_zones; ++m) // enkel zones
					{
						for (int k = 0; k < _max_nb_segments; ++k)
						{
							++nb_constraints;

							rhs[0] = 0;
							sense[0] = 'L';
							matbeg[0] = 0;

							nonzeroes = 0;

							// w_tqvik
							{
								const size_t index = _index_w_tqvik(data, t, q, v, m, k);
								if (index >= nb_variables)
									throw std::runtime_error("Error in function IP_model_routing::build_problem(). Index variable exceeds range");

								matind[nonzeroes] = index;
								matval[nonzeroes] = 1;
								++nonzeroes;
							}

							// - L_tq sum(j) x_qvjik (aankomen bij i)
							for(int j = 0; j < nb_locations; ++j)
							{
								const size_t index = _index_x_qvijk(data, q, v, j, m, k);
								if (index >= nb_variables)
									throw std::runtime_error("Error in function IP_model_routing::build_problem(). Index variable exceeds range");

								matind[nonzeroes] = index;
								const std::string& waste_type = data.waste_type(t);
								matval[nonzeroes] = -data.capacity(q, waste_type);
								++nonzeroes;
							}

							// - L_tq y_qv
							/*
							OUD
							{
								const size_t index = _index_y_qv(data, q, v);
								if (index >= nb_variables)
									throw std::runtime_error("Error in function IP_model_routing::build_problem(). Index variable exceeds range");

								matind[nonzeroes] = index;
								const std::string& waste_type = data.waste_type(t);
								matval[nonzeroes] = -data.capacity(q, waste_type);
								++nonzeroes;
							}�*/


							if (nonzeroes >= maxnonzeroes)
								throw std::runtime_error("Error in function IP_model_routing::build_problem(). Nonzeroes exceeds size of maxnonzeroes (matind and matval)");

							status = CPXaddrows(env, problem, 0, 1, nonzeroes, rhs, sense, matbeg, matind.get(), matval.get(), NULL, NULL);
							if (status != 0)
							{
								CPXgeterrorstring(env, status, error_text);
								throw std::runtime_error("Error in function IP_model_routing::build_problem(). \nCouldn't add constraint. \nReason: " + std::string(error_text));
							}

							// change name of constraint
							std::string conname = "c3_" + std::to_string(t + 1) + "_" + std::to_string(q + 1) + "_" + std::to_string(v + 1)
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
		}

		// 4. sum(q,v,k) w_tqvik == alpha_tid   forall t,i
		for (int t = 0; t < nb_waste_types; ++t)
		{
			for (int m = 0; m < nb_zones; ++m)
			{
				++nb_constraints;

				int dayweek = day % nb_days;
				int week = day / nb_days; // integer division

				rhs[0] = data.x_tmdw(t, m, dayweek, week);
				sense[0] = 'E';
				matbeg[0] = 0;

				nonzeroes = 0;

				// sum(q,v,k) w_tqvik
				for (int q = 0; q < nb_truck_types; ++q)
				{
					for (int v = 0; v < _max_nb_trucks; ++v)
					{
						for (int k = 0; k < _max_nb_segments; ++k)
						{
							const size_t index = _index_w_tqvik(data, t, q, v, m, k);
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
				std::string conname = "c4_" + std::to_string(t + 1) + "_" + std::to_string(m + 1);
				status = CPXchgname(env, problem, 'r', nb_constraints, conname.c_str());
				if (status != 0)
				{
					CPXgeterrorstring(env, status, error_text);
					throw std::runtime_error("Error in function IP_model_routing::build_problem(). \nCouldn't change constraint name. \nReason: " + std::string(error_text));
				}
			}
		}

		// 5. sum(j) x_qv,depot,j,1 - y_qv == 0   forall q,v
		for (int q = 0; q < nb_truck_types; ++q)
		{
			for (int v = 0; v < _max_nb_trucks; ++v)
			{
				++nb_constraints;

				rhs[0] = 0;
				sense[0] = 'E';
				matbeg[0] = 0;

				nonzeroes = 0;

				// sum(j) x_qv,depot,j,1
				for (int j = 0; j < nb_locations; ++j)
				{
					const int index_depot = nb_zones;
					const int index_k = 0;

					const size_t index = _index_x_qvijk(data, q, v, index_depot, j, index_k);
					if (index >= nb_variables)
						throw std::runtime_error("Error in function IP_model_routing::build_problem(). Index variable exceeds range");

					matind[nonzeroes] = index;
					matval[nonzeroes] = 1;
					++nonzeroes;
				}

				// y_qv
				{
					const int index = _index_y_qv(data, q, v);
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
				std::string conname = "c5_" + std::to_string(q + 1) + "_" + std::to_string(v + 1);
				status = CPXchgname(env, problem, 'r', nb_constraints, conname.c_str());
				if (status != 0)
				{
					CPXgeterrorstring(env, status, error_text);
					throw std::runtime_error("Error in function IP_model_routing::build_problem(). \nCouldn't change constraint name. \nReason: " + std::string(error_text));
				}
			}
		}

		// 6. sum(j,k) x_qvi,depot,k - y_qv == 0   forall q,v
		for (int q = 0; q < nb_truck_types; ++q)
		{
			for (int v = 0; v < _max_nb_trucks; ++v)
			{
				++nb_constraints;

				rhs[0] = 0;
				sense[0] = 'E';
				matbeg[0] = 0;

				nonzeroes = 0;

				// sum(j,k) x_qvi,depot,k
				for (int i = 0; i < nb_locations; ++i)
				{
					for (int k = 0; k < _max_nb_segments; ++k)
					{
						const int index_depot = nb_zones;

						const size_t index = _index_x_qvijk(data, q, v, i, index_depot, k);
						if (index >= nb_variables)
							throw std::runtime_error("Error in function IP_model_routing::build_problem(). Index variable exceeds range");

						matind[nonzeroes] = index;
						matval[nonzeroes] = 1;
						++nonzeroes;
					}
				}

				// y_qv
				{
					const size_t index = _index_y_qv(data, q, v);
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
				std::string conname = "c6_" + std::to_string(q + 1) + "_" + std::to_string(v + 1);
				status = CPXchgname(env, problem, 'r', nb_constraints, conname.c_str());
				if (status != 0)
				{
					CPXgeterrorstring(env, status, error_text);
					throw std::runtime_error("Error in function IP_model_routing::build_problem(). \nCouldn't change constraint name. \nReason: " + std::string(error_text));
				}
			}
		}

		// 7. x_qvijk == 0   for zone-zone or dropoff-dropoff
		for (int q = 0; q < nb_truck_types; ++q)
		{
			for (int v = 0; v < _max_nb_trucks; ++v)
			{
				for (int i = 0; i < nb_locations; ++i)
				{
					for (int j = 0; j < nb_locations; ++j)
					{
						for (int k = 0; k < _max_nb_segments; ++k)
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

								// x_qvijk
								{
									const size_t index = _index_x_qvijk(data, q, v, i, j, k);
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
								std::string conname = "c7_" + std::to_string(q + 1) + "_" + std::to_string(v + 1) + "_" + std::to_string(i + 1) + "_"
									+ std::to_string(j + 1) + "_" + std::to_string(k + 1);
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
		}

		// 8. sum(j) x_qvij,k+1 - sum(j) x_qvjik == 0   forall q,v,i!=depot,k
		for (int q = 0; q < nb_truck_types; ++q)
		{
			for (int v = 0; v < _max_nb_trucks; ++v)
			{
				for (int i = 0; i < nb_locations; ++i)
				{
					if (i != nb_zones) // niet depot
					{
						for (int k = 0; k < _max_nb_segments - 1; ++k)
						{
							++nb_constraints;

							rhs[0] = 0;
							sense[0] = 'E';
							matbeg[0] = 0;

							nonzeroes = 0;

							// sum(j) x_qvij,k+1
							for (int j = 0; j < nb_locations; ++j)
							{
								const size_t index = _index_x_qvijk(data, q, v, i, j, k + 1);
								if (index >= nb_variables)
									throw std::runtime_error("Error in function IP_model_routing::build_problem(). Index variable exceeds range");

								matind[nonzeroes] = index;
								matval[nonzeroes] = 1;
								++nonzeroes;
							}

							// sum(j) x_qvjik
							for (int j = 0; j < nb_locations; ++j)
							{
								const size_t index = _index_x_qvijk(data, q, v, j, i, k);
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
							std::string conname = "c8_" + std::to_string(q + 1) + "_" + std::to_string(v + 1) + "_" + std::to_string(i + 1) + "_" + std::to_string(k + 1);
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

		// 9. x_qvijk - y_qv <= 0   forall q,v,i,j,k
		for (int q = 0; q < nb_truck_types; ++q)
		{
			for (int v = 0; v < _max_nb_trucks; ++v)
			{
				for (int i = 0; i < nb_locations; ++i)
				{
					for (int j = 0; j < nb_locations; ++j)
					{
						for (int k = 0; k < _max_nb_segments; ++k)
						{
							++nb_constraints;

							rhs[0] = 0;
							sense[0] = 'L';
							matbeg[0] = 0;

							nonzeroes = 0;

							// x_qvijk
							{
								const size_t index = _index_x_qvijk(data, q, v, i, j, k);
								if (index >= nb_variables)
									throw std::runtime_error("Error in function IP_model_routing::build_problem(). Index variable exceeds range");

								matind[nonzeroes] = index;
								matval[nonzeroes] = 1;
								++nonzeroes;
							}

							// - y_qv
							{
								const size_t index = _index_y_qv(data, q, v);
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
							std::string conname = "c9_" + std::to_string(q + 1) + "_" + std::to_string(v + 1) + "_" + std::to_string(i + 1)
								+ "_" + std::to_string(j + 1) +"_" + std::to_string(k + 1);
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



		// write to file
		status = CPXwriteprob(env, problem, "IP_model_routing.lp", NULL);
		if (status != 0)
		{
			CPXgeterrorstring(env, status, error_text);
			throw std::runtime_error("Error in function IP_model_routing::build_problem(). \nCouldn't write problem to lp-file. \nReason: " + std::string(error_text));
		}

		std::chrono::duration<double, std::ratio<1, 1>> elapsed_time = std::chrono::system_clock::now() - start_time;
		std::cout << "\nElapsed time building problem (s): " << elapsed_time.count();
	}

	void IP_model_routing::solve_problem(const Instance& data, size_t day)
	{
		char error_text[CPXMESSAGEBUFSIZE];
		int status = 0;
		int solstat = 0;
		std::unique_ptr<double[]> solution_problem;
		double objval;

		status = CPXreadcopyprob(env, problem, "IP_model_routing.lp", NULL); /// Dit werkt wel, maar probleem rechtstreeks geeft error
		if (status != 0)
		{
			CPXgeterrorstring(env, status, error_text);
			throw std::runtime_error("Error in function IP_model_routing::solve_problem(). \nCouldn't read problem from lp-file. \nReason: " + std::string(error_text));
		}

		// Set allowed computation time
		status = CPXsetdblparam(env, CPXPARAM_TimeLimit, _max_computation_time);
		if (status != 0)
		{
			CPXgeterrorstring(env, status, error_text);
			throw std::runtime_error("Error in function IP_model_allocation::solve_problem(). \nCouldn't set time limit. \nReason: " + std::string(error_text));
		}

		// Assign memory for solution
		const int numvar = CPXgetnumcols(env, problem);
		solution_problem = std::make_unique<double[]>(numvar);

		// Optimize the problem
		std::cout << "\n\nIP_model_routing: CPLEX is solving the problem ...\n\n";
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
			std::cout << "\n\nDone solving ... \n\nSolution status: " << solstat_text;

			if (solstat == CPXMIP_OPTIMAL || solstat == CPXMIP_OPTIMAL_TOL || solstat == CPXMIP_TIME_LIM_FEAS)
			{
				std::cout << "\nObjval = " << objval;
				std::cout << "\nElapsed time (s): " << elapsed_time_IP.count();

				const size_t nb_waste_types = data.nb_waste_types();
				const size_t nb_truck_types = data.nb_truck_types();
				const size_t nb_zones = data.nb_zones();
				const size_t nb_days = data.nb_days();
				const size_t nb_weeks = data.nb_weeks();
				const size_t nb_collection_points = data.nb_collection_points();
				const size_t nb_locations = nb_zones + 1 + nb_collection_points; // Order: Z1...Zn, depot, CP1...CPk

				// Obtain values for variables
				std::vector<int> y_qv, x_qvijk;
				std::vector<double> w_tqvik, beta_qv;

				y_qv.reserve(nb_truck_types * _max_nb_trucks);
				x_qvijk.reserve(nb_truck_types * _max_nb_trucks * nb_locations * nb_locations * _max_nb_segments);
				w_tqvik.reserve(nb_waste_types * nb_truck_types * _max_nb_trucks * nb_zones * _max_nb_segments);
				beta_qv.reserve(nb_truck_types * _max_nb_trucks);

				// x variables
				for (int q = 0; q < nb_truck_types; ++q) {
					for (int v = 0; v < _max_nb_trucks; ++v) {
						for (int i = 0; i < nb_locations; ++i) {
							for (int j = 0; j < nb_locations; ++j) {
								for (int k = 0; k < _max_nb_segments; ++k) {

									std::string varname = "x_" + std::to_string(q + 1) + "_" + std::to_string(v + 1) + "_"
										+ std::to_string(i + 1) + "_" + std::to_string(j + 1) + "_" + std::to_string(k + 1);

									int index;
									status = CPXgetcolindex(env, problem, varname.c_str(), &index);
									if (status != 0)
									{
										CPXgeterrorstring(env, status, error_text);
										throw std::runtime_error("Error in function IP_model_routing::solve_problem(). \nCouldn't access variable. \nReason: " + std::string(error_text));
									}

									int x = static_cast<int>(solution_problem[index] + 0.1);
									x_qvijk.push_back(x);
								}
							}
						}
					}
				}

				// w variables
				for (int t = 0; t < nb_waste_types; ++t) {
					for (int q = 0; q < nb_truck_types; ++q) {
						for (int v = 0; v < _max_nb_trucks; ++v) {
							for (int m = 0; m < nb_zones; ++m) { // enkel zones
								for (int k = 0; k < _max_nb_segments; ++k) {

									std::string varname = "w_" + std::to_string(t + 1) + "_" + std::to_string(q + 1) + "_"
										+ std::to_string(v + 1) + "_" + std::to_string(m + 1) + "_" + std::to_string(k + 1);

									int index;
									status = CPXgetcolindex(env, problem, varname.c_str(), &index);
									if (status != 0)
									{
										CPXgeterrorstring(env, status, error_text);
										throw std::runtime_error("Error in function IP_model_routing::solve_problem(). \nCouldn't access variable. \nReason: " + std::string(error_text));
									}

									double w = solution_problem[index];
									w_tqvik.push_back(w);
								}
							}
						}
					}
				}

				// y variables and beta variables
				for (int q = 0; q < nb_truck_types; ++q) {
					for (int v = 0; v < _max_nb_trucks; ++v) {

						std::string varname_y = "y_" + std::to_string(q + 1) + "_" + std::to_string(v + 1);
						int index_y;
						status = CPXgetcolindex(env, problem, varname_y.c_str(), &index_y);
						if (status != 0)
						{
							CPXgeterrorstring(env, status, error_text);
							throw std::runtime_error("Error in function IP_model_routing::solve_problem(). \nCouldn't access variable. \nReason: " + std::string(error_text));
						}
						double y = solution_problem[index_y];
						y_qv.push_back(y);


						std::string varname_beta = "beta_" + std::to_string(q + 1) + "_" + std::to_string(v + 1);
						int index_beta;
						status = CPXgetcolindex(env, problem, varname_beta.c_str(), &index_beta);
						if (status != 0)
						{
							CPXgeterrorstring(env, status, error_text);
							throw std::runtime_error("Error in function IP_model_routing::solve_problem(). \nCouldn't access variable. \nReason: " + std::string(error_text));
						}
						double beta = solution_problem[index_beta];
						beta_qv.push_back(beta);
					}
				}



				// Write solution to file
				{
					std::ofstream solfile;
					std::string filename = "solution_IP_model_routing_" + std::to_string(day + 1) + ".txt";
					solfile.open(filename);

					solfile << "\nObjective value = " << objval << "\n\n";
					
					for (int q = 0; q < nb_truck_types; ++q) {
						for (int v = 0; v < _max_nb_trucks; ++v) {
							if (y_qv[q * _max_nb_trucks + v] > 0) {
								solfile << "\n\nVrachtwagen type " << q + 1 << ", nummer " << v + 1; //<< ": y = " << y_qv[q * _max_nb_trucks + v];
								solfile << "\nRijtijd: " << beta_qv[q * _max_nb_trucks + v];
								solfile << "\nOphalingen:";
								for (int t = 0; t < nb_waste_types; ++t) {
									for (int m = 0; m < nb_zones; ++m) {
										for (int k = 0; k < _max_nb_segments; ++k) {
											double wval = w_tqvik[t * nb_truck_types * _max_nb_trucks * nb_zones * _max_nb_segments + q * _max_nb_trucks * nb_zones * _max_nb_segments + v * nb_zones * _max_nb_segments + m * _max_nb_segments + k];
											if (wval > 0) {
												solfile << "\n\t" << data.zone_name(m) << ", " << data.waste_type(t)
													<< ", segment " << k + 1 << ", hoeveelheid = " << wval;
											}
										}
									}
								}
								solfile << "\nRoute:";
								for (int k = 0; k < _max_nb_segments; ++k) {
									for (int i = 0; i < nb_locations; ++i) {
										for (int j = 0; j < nb_locations; ++j) {
											int xval = x_qvijk[q * _max_nb_trucks * nb_locations * nb_locations * _max_nb_segments + v * nb_locations * nb_locations * _max_nb_segments + i * nb_locations * _max_nb_segments + j * _max_nb_segments + k];
											if (xval > 0) {
												std::string origin, destination;
												if (i < nb_zones)
													origin = data.zone_name(i);
												else if (i == nb_zones)
													origin = "depot";
												else
													origin = data.collection_point(i - nb_zones - 1);
												if (j < nb_zones)
													destination = data.zone_name(j);
												else if (j == nb_zones)
													destination = "depot";
												else
													destination = data.collection_point(j - nb_zones - 1);

												solfile << "\n\tsegment " << k + 1 << ": van " << origin << " naar " << destination;
											}
										}
									}
								}
							}
						}
					}

					solfile.flush();
				}
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

	void IP_model_routing::run(const Instance& data, size_t day)
	{
		initialize_cplex();
		build_problem(data, day);
		solve_problem(data, day);
		clear_cplex();
	}

	///////////////////////////////////////////////////////////////////////////////////////////////
}