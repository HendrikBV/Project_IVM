#include "models.h"
#include "data.h"
#include <stdexcept>
#include <memory>
#include <iostream>
#include <chrono>


namespace IVM
{
	///////////////////////////////////////////
	///			IP Model Monolithic			///
	///////////////////////////////////////////

	void IP_model_monolithic::initialize_cplex()
	{
		int status = 0;
		char error_text[CPXMESSAGEBUFSIZE];

		// open the CPLEX environment
		env = CPXopenCPLEX(&status);
		if (status != 0)
		{
			CPXgeterrorstring(env, status, error_text);
			throw std::runtime_error("Error in function IP_model_monolithic::initialize_cplex(). \nCouldn't open CPLEX. \nReason: " + std::string(error_text));
		}

		// turn output to screen on/off
		status = CPXsetintparam(env, CPX_PARAM_SCRIND, CPX_ON);
		if (status != 0)
		{
			CPXgeterrorstring(env, status, error_text);
			throw std::runtime_error("Error in function IP_model_monolithic::initialize_cplex(). \nCouldn't change param SCRIND. \nReason: " + std::string(error_text));
		}
	}

	void IP_model_monolithic::build_problem(const Data& data)
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

		const double big_M = 50;

		// allocate memory
		const size_t maxnonzeroes = 100000;
		matind = std::make_unique<int[]>(maxnonzeroes);
		matval = std::make_unique<double[]>(maxnonzeroes);


		// create the problem
		problem = CPXcreateprob(env, &status, "IP_model_monolithic");

		// problem is minimization
		status = CPXchgobjsen(env, problem, CPX_MIN);

		// add variables
		// variable y_1_vmd
		const int startindex_y1_vmd = 0;
		for (int v = 0; v < data.vehicles(); ++v)
		{
			for (int m = 0; m < data.nb_customers(); ++m)
			{
				for (int d = 0; d < data.days(); ++d)
				{
					obj[0] = data.cost_hour() * data.t_trip1(m);
					lb[0] = 0;
					ub[0] = 1;
					type[0] = 'B';

					status = CPXnewcols(env, problem, 1, obj, lb, ub, type, NULL);
					if (status != 0)
					{
						CPXgeterrorstring(env, status, error_text);
						throw std::runtime_error("Error in function IP_model_monolithic::build_problem(). \nCouldn't add variable. \nReason: " + std::string(error_text));
					}

					// change variable name
					std::string varname = "y1_" + std::to_string(v + 1) + "_" + std::to_string(m + 1) + "_" + std::to_string(d + 1);
					status = CPXchgname(env, problem, 'c', v * data.nb_customers() * data.days() + m * data.days() + d, varname.c_str());
					if (status != 0)
					{
						CPXgeterrorstring(env, status, error_text);
						throw std::runtime_error("Error in function IP_model_monolithic::build_problem(). \nCouldn't change variable name. \nReason: " + std::string(error_text));
					}
				}
			}
		}

		// variable y_2_vmd
		const int startindex_y2_vmd = data.vehicles()*data.nb_customers()*data.days();
		for (int v = 0; v < data.vehicles(); ++v)
		{
			for (int m = 0; m < data.nb_customers(); ++m)
			{
				for (int d = 0; d < data.days(); ++d)
				{
					obj[0] = data.cost_hour() * data.t_trip2(m);
					lb[0] = 0;
					type[0] = 'I';

					status = CPXnewcols(env, problem, 1, obj, lb, NULL, type, NULL);
					if (status != 0)
					{
						CPXgeterrorstring(env, status, error_text);
						throw std::runtime_error("Error in function IP_model_monolithic::build_problem(). \nCouldn't add variable. \nReason: " + std::string(error_text));
					}

					// change variable name
					std::string varname = "y2_" + std::to_string(v + 1) + "_" + std::to_string(m + 1) + "_" + std::to_string(d + 1);
					status = CPXchgname(env, problem, 'c', startindex_y2_vmd + v * data.nb_customers() * data.days() + m * data.days() + d, varname.c_str());
					if (status != 0)
					{
						CPXgeterrorstring(env, status, error_text);
						throw std::runtime_error("Error in function IP_model_monolithic::build_problem(). \nCouldn't change variable name. \nReason: " + std::string(error_text));
					}
				}
			}
		}

		// variable x_1_vmd
		const int startindex_x1_vmd = startindex_y2_vmd + data.vehicles() * data.nb_customers() * data.days();
		for (int v = 0; v < data.vehicles(); ++v)
		{
			for (int m = 0; m < data.nb_customers(); ++m)
			{
				for (int d = 0; d < data.days(); ++d)
				{
					obj[0] = 0;
					lb[0] = 0;

					status = CPXnewcols(env, problem, 1, obj, lb, NULL, NULL, NULL);
					if (status != 0)
					{
						CPXgeterrorstring(env, status, error_text);
						throw std::runtime_error("Error in function IP_model_monolithic::build_problem(). \nCouldn't add variable. \nReason: " + std::string(error_text));
					}

					// change variable name
					std::string varname = "x1_" + std::to_string(v + 1) + "_" + std::to_string(m + 1) + "_" + std::to_string(d + 1);
					status = CPXchgname(env, problem, 'c', startindex_x1_vmd + v * data.nb_customers() * data.days() + m * data.days() + d, varname.c_str());
					if (status != 0)
					{
						CPXgeterrorstring(env, status, error_text);
						throw std::runtime_error("Error in function IP_model_monolithic::build_problem(). \nCouldn't change variable name. \nReason: " + std::string(error_text));
					}
				}
			}
		}

		// variable x_2_vmd
		const int startindex_x2_vmd = startindex_x1_vmd + data.vehicles() * data.nb_customers() * data.days();
		for (int v = 0; v < data.vehicles(); ++v)
		{
			for (int m = 0; m < data.nb_customers(); ++m)
			{
				for (int d = 0; d < data.days(); ++d)
				{
					obj[0] = 0;
					lb[0] = 0;

					status = CPXnewcols(env, problem, 1, obj, lb, NULL, NULL, NULL);
					if (status != 0)
					{
						CPXgeterrorstring(env, status, error_text);
						throw std::runtime_error("Error in function IP_model_monolithic::build_problem(). \nCouldn't add variable. \nReason: " + std::string(error_text));
					}

					// change variable name
					std::string varname = "x2_" + std::to_string(v + 1) + "_" + std::to_string(m + 1) + "_" + std::to_string(d + 1);
					status = CPXchgname(env, problem, 'c', startindex_x2_vmd + v * data.nb_customers() * data.days() + m * data.days() + d, varname.c_str());
					if (status != 0)
					{
						CPXgeterrorstring(env, status, error_text);
						throw std::runtime_error("Error in function IP_model_monolithic::build_problem(). \nCouldn't change variable name. \nReason: " + std::string(error_text));
					}
				}
			}
		}

		// variable w_md
		const int startindex_w_md = startindex_x2_vmd + data.vehicles() * data.nb_customers() * data.days();
		for (int m = 0; m < data.nb_customers(); ++m)
		{
			for (int d = 0; d < data.days(); ++d)
			{
				obj[0] = 0;
				lb[0] = 0;
				ub[0] = 1;
				type[0] = 'B';

				status = CPXnewcols(env, problem, 1, obj, lb, ub, type, NULL);
				if (status != 0)
				{
					CPXgeterrorstring(env, status, error_text);
					throw std::runtime_error("Error in function IP_model_monolithic::build_problem(). \nCouldn't add variable. \nReason: " + std::string(error_text));
				}

				// change variable name
				std::string varname = "w_" + std::to_string(m + 1) + "_" + std::to_string(d + 1);
				status = CPXchgname(env, problem, 'c', startindex_w_md + m * data.days() + d, varname.c_str());
				if (status != 0)
				{
					CPXgeterrorstring(env, status, error_text);
					throw std::runtime_error("Error in function IP_model_monolithic::build_problem(). \nCouldn't change variable name. \nReason: " + std::string(error_text));
				}
			}
		}

		// variable z
		const int startindex_z = startindex_w_md + data.nb_customers() * data.days();
		{
			obj[0] = data.cost_vehicle();
			lb[0] = 0;
			type[0] = 'I';

			status = CPXnewcols(env, problem, 1, obj, lb, NULL, type, NULL);
			if (status != 0)
			{
				CPXgeterrorstring(env, status, error_text);
				throw std::runtime_error("Error in function IP_model_monolithic::build_problem(). \nCouldn't add variable. \nReason: " + std::string(error_text));
			}

			// change variable name
			std::string varname = "z";
			status = CPXchgname(env, problem, 'c', startindex_z, varname.c_str());
			if (status != 0)
			{
				CPXgeterrorstring(env, status, error_text);
				throw std::runtime_error("Error in function IP_model_monolithic::build_problem(). \nCouldn't change variable name. \nReason: " + std::string(error_text));
			}
		}


		// add constraints
		int nb_constraints = -1;

		// 1: x1_vmd - L * y1_vmd <= 0     forall v,m,d
		for (int v = 0; v < data.vehicles(); ++v)
		{
			for (int m = 0; m < data.nb_customers(); ++m)
			{
				for (int d = 0; d < data.days(); ++d)
				{
					++nb_constraints;

					rhs[0] = 0;
					sense[0] = 'L';
					matbeg[0] = 0;

					nonzeroes = 0;

					// x1_vmd
					matind[nonzeroes] = startindex_x1_vmd + v * data.nb_customers() * data.days() + m * data.days() + d;
					matval[nonzeroes] = 1;
					++nonzeroes;

					// y1_vmd
					matind[nonzeroes] = startindex_y1_vmd + v * data.nb_customers() * data.days() + m * data.days() + d;
					matval[nonzeroes] = -data.max_load();
					++nonzeroes;

					if (nonzeroes >= maxnonzeroes)
						throw std::runtime_error("Error in function IP_model_monolithic::build_problem(). Nonzeroes exceeds size of maxnonzeroes (matind and matval)");

					status = CPXaddrows(env, problem, 0, 1, nonzeroes, rhs, sense, matbeg, matind.get(), matval.get(), NULL, NULL);
					if (status != 0)
					{
						CPXgeterrorstring(env, status, error_text);
						throw std::runtime_error("Error in function IP_model_monolithic::build_problem(). \nCouldn't add constraint. \nReason: " + std::string(error_text));
					}

					// change name of constraint
					std::string conname = "c1_" + std::to_string(v + 1) + "_" + std::to_string(m + 1) + "_" + std::to_string(d + 1);
					status = CPXchgname(env, problem, 'r', nb_constraints, conname.c_str());
					if (status != 0)
					{
						CPXgeterrorstring(env, status, error_text);
						throw std::runtime_error("Error in function IP_model_monolithic::build_problem(). \nCouldn't change constraint name. \nReason: " + std::string(error_text));
					}
				}
			}
		}

		// 2: x2_vmd - L * y2_vmd <= 0     forall v,m,d
		for (int v = 0; v < data.vehicles(); ++v)
		{
			for (int m = 0; m < data.nb_customers(); ++m)
			{
				for (int d = 0; d < data.days(); ++d)
				{
					++nb_constraints;

					rhs[0] = 0;
					sense[0] = 'L';
					matbeg[0] = 0;

					nonzeroes = 0;

					// x2_vmd
					matind[nonzeroes] = startindex_x2_vmd + v * data.nb_customers() * data.days() + m * data.days() + d;
					matval[nonzeroes] = 1;
					++nonzeroes;

					// y2_vmd
					matind[nonzeroes] = startindex_y2_vmd + v * data.nb_customers() * data.days() + m * data.days() + d;
					matval[nonzeroes] = -data.max_load();
					++nonzeroes;

					if (nonzeroes >= maxnonzeroes)
						throw std::runtime_error("Error in function IP_model_monolithic::build_problem(). Nonzeroes exceeds size of maxnonzeroes (matind and matval)");

					status = CPXaddrows(env, problem, 0, 1, nonzeroes, rhs, sense, matbeg, matind.get(), matval.get(), NULL, NULL);
					if (status != 0)
					{
						CPXgeterrorstring(env, status, error_text);
						throw std::runtime_error("Error in function IP_model_monolithic::build_problem(). \nCouldn't add constraint. \nReason: " + std::string(error_text));
					}

					// change name of constraint
					std::string conname = "c2_" + std::to_string(v + 1) + "_" + std::to_string(m + 1) + "_" + std::to_string(d + 1);
					status = CPXchgname(env, problem, 'r', nb_constraints, conname.c_str());
					if (status != 0)
					{
						CPXgeterrorstring(env, status, error_text);
						throw std::runtime_error("Error in function IP_model_monolithic::build_problem(). \nCouldn't change constraint name. \nReason: " + std::string(error_text));
					}
				}
			}
		}

		// 3: sum(v,d) (x1_vmd + x2_vmd) == Qm     forall m
		for (int m = 0; m < data.nb_customers(); ++m)
		{
			++nb_constraints;

			rhs[0] = data.demand(m);
			sense[0] = 'E';
			matbeg[0] = 0;

			nonzeroes = 0;

			// x1_vmd
			for (int v = 0; v < data.vehicles(); ++v)
			{
				for (int d = 0; d < data.days(); ++d)
				{
					matind[nonzeroes] = startindex_x1_vmd + v * data.nb_customers() * data.days() + m * data.days() + d;
					matval[nonzeroes] = 1;
					++nonzeroes;
				}
			}

			// x2_vmd
			for (int v = 0; v < data.vehicles(); ++v)
			{
				for (int d = 0; d < data.days(); ++d)
				{
					matind[nonzeroes] = startindex_x2_vmd + v * data.nb_customers() * data.days() + m * data.days() + d;
					matval[nonzeroes] = 1;
					++nonzeroes;
				}
			}

			if (nonzeroes >= maxnonzeroes)
				throw std::runtime_error("Error in function IP_model_monolithic::build_problem(). Nonzeroes exceeds size of maxnonzeroes (matind and matval)");

			status = CPXaddrows(env, problem, 0, 1, nonzeroes, rhs, sense, matbeg, matind.get(), matval.get(), NULL, NULL);
			if (status != 0)
			{
				CPXgeterrorstring(env, status, error_text);
				throw std::runtime_error("Error in function IP_model_monolithic::build_problem(). \nCouldn't add constraint. \nReason: " + std::string(error_text));
			}

			// change name of constraint
			std::string conname = "c3_" + std::to_string(m + 1);
			status = CPXchgname(env, problem, 'r', nb_constraints, conname.c_str());
			if (status != 0)
			{
				CPXgeterrorstring(env, status, error_text);
				throw std::runtime_error("Error in function IP_model_monolithic::build_problem(). \nCouldn't change constraint name. \nReason: " + std::string(error_text));
			}
		}

		// 4: sum(m) (s_m*(x1_vmd + x2_vmd) + t1_m*y1_vmd + t2_m*y2_vmd) <= T   forall v,d
		for (int v = 0; v < data.vehicles(); ++v)
		{
			for (int d = 0; d < data.days(); ++d)
			{
				++nb_constraints;

				rhs[0] = data.max_hours();
				sense[0] = 'L';
				matbeg[0] = 0;

				nonzeroes = 0;

				// x1_vmd
				for (int m = 0; m < data.nb_customers(); ++m)
				{
					matind[nonzeroes] = startindex_x1_vmd + v * data.nb_customers() * data.days() + m * data.days() + d;
					matval[nonzeroes] = data.collection_speed(m);
					++nonzeroes;
				}

				// x2_vmd
				for (int m = 0; m < data.nb_customers(); ++m)
				{
					matind[nonzeroes] = startindex_x2_vmd + v * data.nb_customers() * data.days() + m * data.days() + d;
					matval[nonzeroes] = data.collection_speed(m);
					++nonzeroes;
				}

				// y1_vmd
				for (int m = 0; m < data.nb_customers(); ++m)
				{
					matind[nonzeroes] = startindex_y1_vmd + v * data.nb_customers() * data.days() + m * data.days() + d;
					matval[nonzeroes] = data.t_trip1(m);
					++nonzeroes;
				}

				// y1_vmd
				for (int m = 0; m < data.nb_customers(); ++m)
				{
					matind[nonzeroes] = startindex_y2_vmd + v * data.nb_customers() * data.days() + m * data.days() + d;
					matval[nonzeroes] = data.t_trip2(m);
					++nonzeroes;
				}

				if (nonzeroes >= maxnonzeroes)
					throw std::runtime_error("Error in function IP_model_monolithic::build_problem(). Nonzeroes exceeds size of maxnonzeroes (matind and matval)");

				status = CPXaddrows(env, problem, 0, 1, nonzeroes, rhs, sense, matbeg, matind.get(), matval.get(), NULL, NULL);
				if (status != 0)
				{
					CPXgeterrorstring(env, status, error_text);
					throw std::runtime_error("Error in function IP_model_monolithic::build_problem(). \nCouldn't add constraint. \nReason: " + std::string(error_text));
				}

				// change name of constraint
				std::string conname = "c4_" + std::to_string(v + 1) + "_" + std::to_string(d + 1);
				status = CPXchgname(env, problem, 'r', nb_constraints, conname.c_str());
				if (status != 0)
				{
					CPXgeterrorstring(env, status, error_text);
					throw std::runtime_error("Error in function IP_model_monolithic::build_problem(). \nCouldn't change constraint name. \nReason: " + std::string(error_text));
				}
			}
		}

		// 5: sum(m) y1_vmd <= 1   forall v,d
		for (int v = 0; v < data.vehicles(); ++v)
		{
			for (int d = 0; d < data.days(); ++d)
			{
				++nb_constraints;

				rhs[0] = 1;
				sense[0] = 'L';
				matbeg[0] = 0;

				nonzeroes = 0;

				// y1_vmd
				for (int m = 0; m < data.nb_customers(); ++m)
				{
					matind[nonzeroes] = startindex_y1_vmd + v * data.nb_customers() * data.days() + m * data.days() + d;
					matval[nonzeroes] = 1;
					++nonzeroes;
				}

				if (nonzeroes >= maxnonzeroes)
					throw std::runtime_error("Error in function IP_model_monolithic::build_problem(). Nonzeroes exceeds size of maxnonzeroes (matind and matval)");

				status = CPXaddrows(env, problem, 0, 1, nonzeroes, rhs, sense, matbeg, matind.get(), matval.get(), NULL, NULL);
				if (status != 0)
				{
					CPXgeterrorstring(env, status, error_text);
					throw std::runtime_error("Error in function IP_model_monolithic::build_problem(). \nCouldn't add constraint. \nReason: " + std::string(error_text));
				}

				// change name of constraint
				std::string conname = "c5_" + std::to_string(v + 1) + "_" + std::to_string(d + 1);
				status = CPXchgname(env, problem, 'r', nb_constraints, conname.c_str());
				if (status != 0)
				{
					CPXgeterrorstring(env, status, error_text);
					throw std::runtime_error("Error in function IP_model_monolithic::build_problem(). \nCouldn't change constraint name. \nReason: " + std::string(error_text));
				}
			}
		}

		// 6: N*sum(m') y1_vm'd - y2_vmd >= 0     forall v,m,d
		for (int v = 0; v < data.vehicles(); ++v)
		{
			for (int m = 0; m < data.nb_customers(); ++m)
			{
				for (int d = 0; d < data.days(); ++d)
				{
					++nb_constraints;

					rhs[0] = 0;
					sense[0] = 'G';
					matbeg[0] = 0;

					nonzeroes = 0;

					// y1_vm'd
					for (int macc = 0; macc < data.nb_customers(); ++macc)
					{
						matind[nonzeroes] = startindex_y1_vmd + v * data.nb_customers() * data.days() + macc * data.days() + d;
						matval[nonzeroes] = big_M;
						++nonzeroes;
					}

					// y2_vmd
					matind[nonzeroes] = startindex_y2_vmd + v * data.nb_customers() * data.days() + m * data.days() + d;
					matval[nonzeroes] = -1;
					++nonzeroes;

					if (nonzeroes >= maxnonzeroes)
						throw std::runtime_error("Error in function IP_model_monolithic::build_problem(). Nonzeroes exceeds size of maxnonzeroes (matind and matval)");

					status = CPXaddrows(env, problem, 0, 1, nonzeroes, rhs, sense, matbeg, matind.get(), matval.get(), NULL, NULL);
					if (status != 0)
					{
						CPXgeterrorstring(env, status, error_text);
						throw std::runtime_error("Error in function IP_model_monolithic::build_problem(). \nCouldn't add constraint. \nReason: " + std::string(error_text));
					}

					// change name of constraint
					std::string conname = "c6_" + std::to_string(v + 1) + "_" + std::to_string(m + 1) + "_" + std::to_string(d + 1);
					status = CPXchgname(env, problem, 'r', nb_constraints, conname.c_str());
					if (status != 0)
					{
						CPXgeterrorstring(env, status, error_text);
						throw std::runtime_error("Error in function IP_model_monolithic::build_problem(). \nCouldn't change constraint name. \nReason: " + std::string(error_text));
					}
				}
			}
		}

		// 7: z - sum(v,m) y1_vmd >= 0       forall d
		for (int d = 0; d < data.days(); ++d)
		{
			++nb_constraints;

			rhs[0] = 0;
			sense[0] = 'G';
			matbeg[0] = 0;

			nonzeroes = 0;

			// y1_vmd
			for (int v = 0; v < data.vehicles(); ++v)
			{
				for (int m = 0; m < data.nb_customers(); ++m)
				{
					matind[nonzeroes] = startindex_y1_vmd + v * data.nb_customers() * data.days() + m * data.days() + d;
					matval[nonzeroes] = -1;
					++nonzeroes;
				}
			}

			// z
			matind[nonzeroes] = startindex_z;
			matval[nonzeroes] = 1;
			++nonzeroes;

			if (nonzeroes >= maxnonzeroes)
				throw std::runtime_error("Error in function IP_model_monolithic::build_problem(). Nonzeroes exceeds size of maxnonzeroes (matind and matval)");

			status = CPXaddrows(env, problem, 0, 1, nonzeroes, rhs, sense, matbeg, matind.get(), matval.get(), NULL, NULL);
			if (status != 0)
			{
				CPXgeterrorstring(env, status, error_text);
				throw std::runtime_error("Error in function IP_model_monolithic::build_problem(). \nCouldn't add constraint. \nReason: " + std::string(error_text));
			}

			// change name of constraint
			std::string conname = "c7_" + std::to_string(d + 1);
			status = CPXchgname(env, problem, 'r', nb_constraints, conname.c_str());
			if (status != 0)
			{
				CPXgeterrorstring(env, status, error_text);
				throw std::runtime_error("Error in function IP_model_monolithic::build_problem(). \nCouldn't change constraint name. \nReason: " + std::string(error_text));
			}
		}

		// 8: w_md - y1_vmd >= 0    forall v,m,d
		for (int v = 0; v < data.vehicles(); ++v)
		{
			for (int m = 0; m < data.nb_customers(); ++m)
			{
				for (int d = 0; d < data.days(); ++d)
				{
					++nb_constraints;

					rhs[0] = 0;
					sense[0] = 'G';
					matbeg[0] = 0;

					nonzeroes = 0;

					// w_md
					matind[nonzeroes] = startindex_w_md + m * data.days() + d;
					matval[nonzeroes] = 1;
					++nonzeroes;

					// y1_vmd
					matind[nonzeroes] = startindex_y1_vmd + v * data.nb_customers() * data.days() + m * data.days() + d;
					matval[nonzeroes] = -1;
					++nonzeroes;

					if (nonzeroes >= maxnonzeroes)
						throw std::runtime_error("Error in function IP_model_monolithic::build_problem(). Nonzeroes exceeds size of maxnonzeroes (matind and matval)");

					status = CPXaddrows(env, problem, 0, 1, nonzeroes, rhs, sense, matbeg, matind.get(), matval.get(), NULL, NULL);
					if (status != 0)
					{
						CPXgeterrorstring(env, status, error_text);
						throw std::runtime_error("Error in function IP_model_monolithic::build_problem(). \nCouldn't add constraint. \nReason: " + std::string(error_text));
					}

					// change name of constraint
					std::string conname = "c8_" + std::to_string(v + 1) + "_" + std::to_string(m + 1) + "_" + std::to_string(d + 1);
					status = CPXchgname(env, problem, 'r', nb_constraints, conname.c_str());
					if (status != 0)
					{
						CPXgeterrorstring(env, status, error_text);
						throw std::runtime_error("Error in function IP_model_monolithic::build_problem(). \nCouldn't change constraint name. \nReason: " + std::string(error_text));
					}
				}
			}
		}

		// 9: bigM*w_md - y2_vmd >= 0    forall v,m,d
		for (int v = 0; v < data.vehicles(); ++v)
		{
			for (int m = 0; m < data.nb_customers(); ++m)
			{
				for (int d = 0; d < data.days(); ++d)
				{
					++nb_constraints;

					rhs[0] = 0;
					sense[0] = 'G';
					matbeg[0] = 0;

					nonzeroes = 0;

					// w_md
					matind[nonzeroes] = startindex_w_md + m * data.days() + d;
					matval[nonzeroes] = big_M;
					++nonzeroes;

					// y2_vmd
					matind[nonzeroes] = startindex_y2_vmd + v * data.nb_customers() * data.days() + m * data.days() + d;
					matval[nonzeroes] = -1;
					++nonzeroes;

					if (nonzeroes >= maxnonzeroes)
						throw std::runtime_error("Error in function IP_model_monolithic::build_problem(). Nonzeroes exceeds size of maxnonzeroes (matind and matval)");

					status = CPXaddrows(env, problem, 0, 1, nonzeroes, rhs, sense, matbeg, matind.get(), matval.get(), NULL, NULL);
					if (status != 0)
					{
						CPXgeterrorstring(env, status, error_text);
						throw std::runtime_error("Error in function IP_model_monolithic::build_problem(). \nCouldn't add constraint. \nReason: " + std::string(error_text));
					}

					// change name of constraint
					std::string conname = "c9_" + std::to_string(v + 1) + "_" + std::to_string(m + 1) + "_" + std::to_string(d + 1);
					status = CPXchgname(env, problem, 'r', nb_constraints, conname.c_str());
					if (status != 0)
					{
						CPXgeterrorstring(env, status, error_text);
						throw std::runtime_error("Error in function IP_model_monolithic::build_problem(). \nCouldn't change constraint name. \nReason: " + std::string(error_text));
					}
				}
			}
		}

		// 10: sum(d) w_md <= W_m    forall m
		for (int m = 0; m < data.nb_customers(); ++m)
		{
			++nb_constraints;

			rhs[0] = data.max_visits();
			sense[0] = 'L';
			matbeg[0] = 0;

			nonzeroes = 0;

			// w_md
			for (int d = 0; d < data.days(); ++d)
			{
				matind[nonzeroes] = startindex_w_md + m * data.days() + d;
				matval[nonzeroes] = 1;
				++nonzeroes;
			}

			if (nonzeroes >= maxnonzeroes)
				throw std::runtime_error("Error in function IP_model_monolithic::build_problem(). Nonzeroes exceeds size of maxnonzeroes (matind and matval)");

			status = CPXaddrows(env, problem, 0, 1, nonzeroes, rhs, sense, matbeg, matind.get(), matval.get(), NULL, NULL);
			if (status != 0)
			{
				CPXgeterrorstring(env, status, error_text);
				throw std::runtime_error("Error in function IP_model_monolithic::build_problem(). \nCouldn't add constraint. \nReason: " + std::string(error_text));
			}

			// change name of constraint
			std::string conname = "c10_" + std::to_string(m + 1);
			status = CPXchgname(env, problem, 'r', nb_constraints, conname.c_str());
			if (status != 0)
			{
				CPXgeterrorstring(env, status, error_text);
				throw std::runtime_error("Error in function IP_model_monolithic::build_problem(). \nCouldn't change constraint name. \nReason: " + std::string(error_text));
			}
		}

		// write to file
		status = CPXwriteprob(env, problem, "IP_model_monolithic.lp", NULL);
		if (status != 0)
		{
			CPXgeterrorstring(env, status, error_text);
			throw std::runtime_error("Error in function IP_model_monolithic::build_problem(). \nCouldn't write problem to lp-file. \nReason: " + std::string(error_text));
		}
	}

	void IP_model_monolithic::solve_problem(const Data& data)
	{
		char error_text[CPXMESSAGEBUFSIZE];
		int status = 0;
		int solstat = 0;
		std::unique_ptr<double[]> solution_problem;
		double objval;

		// Set time limit (in seconds)
		status = CPXsetdblparam(env, CPXPARAM_TimeLimit, _time_limit);
		if (status != 0)
		{
			CPXgeterrorstring(env, status, error_text);
			throw std::runtime_error("Error in function IP_model_monolithic::solve_problem(). \nCouldn't set time limit. \nReason: " + std::string(error_text));
		}

		// Assign memory for solution
		int numcols = CPXgetnumcols(env, problem);
		solution_problem = std::make_unique<double[]>(CPXgetnumcols(env, problem));


		// Optimize the problem
		std::cout << "\n\n\nIP_model_monolithic: CPLEX is solving the problem ...\n";
		auto start_time = std::chrono::system_clock::now();

		status = CPXmipopt(env, problem);
		if (status != 0)
		{
			CPXgeterrorstring(env, status, error_text);
			throw std::runtime_error("Error in function IP_model_monolithic::solve_problem(). \nCPXmipopt failed. \nReason: " + std::string(error_text));
		}

		std::chrono::duration<double, std::ratio<1, 1>> elapsed_time_IP = std::chrono::system_clock::now() - start_time;


		// Get the solution
		status = CPXsolution(env, problem, &solstat, &objval, solution_problem.get(), NULL, NULL, NULL);
		if (status != 0)
		{
			CPXgeterrorstring(env, status, error_text);
			throw std::runtime_error("Error in function IP_model_monolithic::solve_problem(). \nCPXsolution failed. \nReason: " + std::string(error_text));
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

	void IP_model_monolithic::clear_cplex()
	{
		int status = 0;
		char error_text[CPXMESSAGEBUFSIZE];

		// Free the problem
		status = CPXfreeprob(env, &problem);
		if (status != 0)
		{
			CPXgeterrorstring(env, status, error_text);
			throw std::runtime_error("Error in function IP_model_monolithic::clear_cplex(). \nCouldn't free problem. \nReason: " + std::string(error_text));
		}

		// Close the cplex environment
		status = CPXcloseCPLEX(&env);
		if (status != 0)
		{
			CPXgeterrorstring(env, status, error_text);
			throw std::runtime_error("Error in function IP_model_monolithic::clear_cplex(). \nCouldn't close cplex environment. \nReason: " + std::string(error_text));
		}
	}

	void IP_model_monolithic::run(const Data& data)
	{
		initialize_cplex();
		build_problem(data);
		solve_problem(data);
		clear_cplex();
	}

	///////////////////////////////////////////////////////////////////////////////////////////////

}