#include "models.h"
#include "data.h"
#include <stdexcept>
#include <memory>
#include <iostream>
#include <chrono>
#include <random>
#include <vector>
#include <list>
#include <algorithm>



namespace Random
{
	std::random_device randdev;
	std::seed_seq seed{ randdev(), randdev(), randdev(), randdev(), randdev(), randdev(), randdev() };
	std::mt19937_64 generator(seed);
}



namespace IVM
{
	///////////////////////////////////////////
	///			Fix-and-optimize			///
	///////////////////////////////////////////

	void IP_VNDS::initialize_cplex()
	{
		int status = 0;
		char error_text[CPXMESSAGEBUFSIZE];

		// open the CPLEX environment
		env = CPXopenCPLEX(&status);
		if (status != 0)
		{
			CPXgeterrorstring(env, status, error_text);
			throw std::runtime_error("Error in function IP_VNDS::initialize_cplex(). \nCouldn't open CPLEX. \nReason: " + std::string(error_text));
		}

		// turn output to screen on/off
		status = CPXsetintparam(env, CPX_PARAM_SCRIND, CPX_OFF);
		if (status != 0)
		{
			CPXgeterrorstring(env, status, error_text);
			throw std::runtime_error("Error in function IP_VNDS::initialize_cplex(). \nCouldn't change param SCRIND. \nReason: " + std::string(error_text));
		}
	}

	void IP_VNDS::build_problem(const Data& data)
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

		const double big_M = 1; // good value???

		// allocate memory
		const size_t maxnonzeroes = 100000;
		matind = std::make_unique<int[]>(maxnonzeroes);
		matval = std::make_unique<double[]>(maxnonzeroes);


		// create the problem
		problem = CPXcreateprob(env, &status, "IP_VNDS");

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
						throw std::runtime_error("Error in function IP_VNDS::build_problem(). \nCouldn't add variable. \nReason: " + std::string(error_text));
					}

					// change variable name
					std::string varname = "y1_" + std::to_string(v + 1) + "_" + std::to_string(m + 1) + "_" + std::to_string(d + 1);
					status = CPXchgname(env, problem, 'c', v * data.nb_customers() * data.days() + m * data.days() + d, varname.c_str());
					if (status != 0)
					{
						CPXgeterrorstring(env, status, error_text);
						throw std::runtime_error("Error in function IP_VNDS::build_problem(). \nCouldn't change variable name. \nReason: " + std::string(error_text));
					}
				}
			}
		}

		// variable y_2_vmd
		const int startindex_y2_vmd = data.vehicles() * data.nb_customers() * data.days();
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
						throw std::runtime_error("Error in function IP_VNDS::build_problem(). \nCouldn't add variable. \nReason: " + std::string(error_text));
					}

					// change variable name
					std::string varname = "y2_" + std::to_string(v + 1) + "_" + std::to_string(m + 1) + "_" + std::to_string(d + 1);
					status = CPXchgname(env, problem, 'c', startindex_y2_vmd + v * data.nb_customers() * data.days() + m * data.days() + d, varname.c_str());
					if (status != 0)
					{
						CPXgeterrorstring(env, status, error_text);
						throw std::runtime_error("Error in function IP_VNDS::build_problem(). \nCouldn't change variable name. \nReason: " + std::string(error_text));
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
						throw std::runtime_error("Error in function IP_VNDS::build_problem(). \nCouldn't add variable. \nReason: " + std::string(error_text));
					}

					// change variable name
					std::string varname = "x1_" + std::to_string(v + 1) + "_" + std::to_string(m + 1) + "_" + std::to_string(d + 1);
					status = CPXchgname(env, problem, 'c', startindex_x1_vmd + v * data.nb_customers() * data.days() + m * data.days() + d, varname.c_str());
					if (status != 0)
					{
						CPXgeterrorstring(env, status, error_text);
						throw std::runtime_error("Error in function IP_VNDS::build_problem(). \nCouldn't change variable name. \nReason: " + std::string(error_text));
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
						throw std::runtime_error("Error in function IP_VNDS::build_problem(). \nCouldn't add variable. \nReason: " + std::string(error_text));
					}

					// change variable name
					std::string varname = "x2_" + std::to_string(v + 1) + "_" + std::to_string(m + 1) + "_" + std::to_string(d + 1);
					status = CPXchgname(env, problem, 'c', startindex_x2_vmd + v * data.nb_customers() * data.days() + m * data.days() + d, varname.c_str());
					if (status != 0)
					{
						CPXgeterrorstring(env, status, error_text);
						throw std::runtime_error("Error in function IP_VNDS::build_problem(). \nCouldn't change variable name. \nReason: " + std::string(error_text));
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
					throw std::runtime_error("Error in function IP_VNDS::build_problem(). \nCouldn't add variable. \nReason: " + std::string(error_text));
				}

				// change variable name
				std::string varname = "w_" + std::to_string(m + 1) + "_" + std::to_string(d + 1);
				status = CPXchgname(env, problem, 'c', startindex_w_md + m * data.days() + d, varname.c_str());
				if (status != 0)
				{
					CPXgeterrorstring(env, status, error_text);
					throw std::runtime_error("Error in function IP_VNDS::build_problem(). \nCouldn't change variable name. \nReason: " + std::string(error_text));
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
				throw std::runtime_error("Error in function IP_VNDS::build_problem(). \nCouldn't add variable. \nReason: " + std::string(error_text));
			}

			// change variable name
			std::string varname = "z";
			status = CPXchgname(env, problem, 'c', startindex_z, varname.c_str());
			if (status != 0)
			{
				CPXgeterrorstring(env, status, error_text);
				throw std::runtime_error("Error in function IP_VNDS::build_problem(). \nCouldn't change variable name. \nReason: " + std::string(error_text));
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
						throw std::runtime_error("Error in function IP_VNDS::build_problem(). Nonzeroes exceeds size of maxnonzeroes (matind and matval)");

					status = CPXaddrows(env, problem, 0, 1, nonzeroes, rhs, sense, matbeg, matind.get(), matval.get(), NULL, NULL);
					if (status != 0)
					{
						CPXgeterrorstring(env, status, error_text);
						throw std::runtime_error("Error in function IP_VNDS::build_problem(). \nCouldn't add constraint. \nReason: " + std::string(error_text));
					}

					// change name of constraint
					std::string conname = "c1_" + std::to_string(v + 1) + "_" + std::to_string(m + 1) + "_" + std::to_string(d + 1);
					status = CPXchgname(env, problem, 'r', nb_constraints, conname.c_str());
					if (status != 0)
					{
						CPXgeterrorstring(env, status, error_text);
						throw std::runtime_error("Error in function IP_VNDS::build_problem(). \nCouldn't change constraint name. \nReason: " + std::string(error_text));
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
						throw std::runtime_error("Error in function IP_VNDS::build_problem(). Nonzeroes exceeds size of maxnonzeroes (matind and matval)");

					status = CPXaddrows(env, problem, 0, 1, nonzeroes, rhs, sense, matbeg, matind.get(), matval.get(), NULL, NULL);
					if (status != 0)
					{
						CPXgeterrorstring(env, status, error_text);
						throw std::runtime_error("Error in function IP_VNDS::build_problem(). \nCouldn't add constraint. \nReason: " + std::string(error_text));
					}

					// change name of constraint
					std::string conname = "c2_" + std::to_string(v + 1) + "_" + std::to_string(m + 1) + "_" + std::to_string(d + 1);
					status = CPXchgname(env, problem, 'r', nb_constraints, conname.c_str());
					if (status != 0)
					{
						CPXgeterrorstring(env, status, error_text);
						throw std::runtime_error("Error in function IP_VNDS::build_problem(). \nCouldn't change constraint name. \nReason: " + std::string(error_text));
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
				throw std::runtime_error("Error in function IP_VNDS::build_problem(). Nonzeroes exceeds size of maxnonzeroes (matind and matval)");

			status = CPXaddrows(env, problem, 0, 1, nonzeroes, rhs, sense, matbeg, matind.get(), matval.get(), NULL, NULL);
			if (status != 0)
			{
				CPXgeterrorstring(env, status, error_text);
				throw std::runtime_error("Error in function IP_VNDS::build_problem(). \nCouldn't add constraint. \nReason: " + std::string(error_text));
			}

			// change name of constraint
			std::string conname = "c3_" + std::to_string(m + 1);
			status = CPXchgname(env, problem, 'r', nb_constraints, conname.c_str());
			if (status != 0)
			{
				CPXgeterrorstring(env, status, error_text);
				throw std::runtime_error("Error in function IP_VNDS::build_problem(). \nCouldn't change constraint name. \nReason: " + std::string(error_text));
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
					throw std::runtime_error("Error in function IP_VNDS::build_problem(). Nonzeroes exceeds size of maxnonzeroes (matind and matval)");

				status = CPXaddrows(env, problem, 0, 1, nonzeroes, rhs, sense, matbeg, matind.get(), matval.get(), NULL, NULL);
				if (status != 0)
				{
					CPXgeterrorstring(env, status, error_text);
					throw std::runtime_error("Error in function IP_VNDS::build_problem(). \nCouldn't add constraint. \nReason: " + std::string(error_text));
				}

				// change name of constraint
				std::string conname = "c4_" + std::to_string(v + 1) + "_" + std::to_string(d + 1);
				status = CPXchgname(env, problem, 'r', nb_constraints, conname.c_str());
				if (status != 0)
				{
					CPXgeterrorstring(env, status, error_text);
					throw std::runtime_error("Error in function IP_VNDS::build_problem(). \nCouldn't change constraint name. \nReason: " + std::string(error_text));
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
					throw std::runtime_error("Error in function IP_VNDS::build_problem(). Nonzeroes exceeds size of maxnonzeroes (matind and matval)");

				status = CPXaddrows(env, problem, 0, 1, nonzeroes, rhs, sense, matbeg, matind.get(), matval.get(), NULL, NULL);
				if (status != 0)
				{
					CPXgeterrorstring(env, status, error_text);
					throw std::runtime_error("Error in function IP_VNDS::build_problem(). \nCouldn't add constraint. \nReason: " + std::string(error_text));
				}

				// change name of constraint
				std::string conname = "c5_" + std::to_string(v + 1) + "_" + std::to_string(d + 1);
				status = CPXchgname(env, problem, 'r', nb_constraints, conname.c_str());
				if (status != 0)
				{
					CPXgeterrorstring(env, status, error_text);
					throw std::runtime_error("Error in function IP_VNDS::build_problem(). \nCouldn't change constraint name. \nReason: " + std::string(error_text));
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
						throw std::runtime_error("Error in function IP_VNDS::build_problem(). Nonzeroes exceeds size of maxnonzeroes (matind and matval)");

					status = CPXaddrows(env, problem, 0, 1, nonzeroes, rhs, sense, matbeg, matind.get(), matval.get(), NULL, NULL);
					if (status != 0)
					{
						CPXgeterrorstring(env, status, error_text);
						throw std::runtime_error("Error in function IP_VNDS::build_problem(). \nCouldn't add constraint. \nReason: " + std::string(error_text));
					}

					// change name of constraint
					std::string conname = "c6_" + std::to_string(v + 1) + "_" + std::to_string(m + 1) + "_" + std::to_string(d + 1);
					status = CPXchgname(env, problem, 'r', nb_constraints, conname.c_str());
					if (status != 0)
					{
						CPXgeterrorstring(env, status, error_text);
						throw std::runtime_error("Error in function IP_VNDS::build_problem(). \nCouldn't change constraint name. \nReason: " + std::string(error_text));
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
				throw std::runtime_error("Error in function IP_VNDS::build_problem(). Nonzeroes exceeds size of maxnonzeroes (matind and matval)");

			status = CPXaddrows(env, problem, 0, 1, nonzeroes, rhs, sense, matbeg, matind.get(), matval.get(), NULL, NULL);
			if (status != 0)
			{
				CPXgeterrorstring(env, status, error_text);
				throw std::runtime_error("Error in function IP_VNDS::build_problem(). \nCouldn't add constraint. \nReason: " + std::string(error_text));
			}

			// change name of constraint
			std::string conname = "c7_" + std::to_string(d + 1);
			status = CPXchgname(env, problem, 'r', nb_constraints, conname.c_str());
			if (status != 0)
			{
				CPXgeterrorstring(env, status, error_text);
				throw std::runtime_error("Error in function IP_VNDS::build_problem(). \nCouldn't change constraint name. \nReason: " + std::string(error_text));
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
						throw std::runtime_error("Error in function IP_VNDS::build_problem(). Nonzeroes exceeds size of maxnonzeroes (matind and matval)");

					status = CPXaddrows(env, problem, 0, 1, nonzeroes, rhs, sense, matbeg, matind.get(), matval.get(), NULL, NULL);
					if (status != 0)
					{
						CPXgeterrorstring(env, status, error_text);
						throw std::runtime_error("Error in function IP_VNDS::build_problem(). \nCouldn't add constraint. \nReason: " + std::string(error_text));
					}

					// change name of constraint
					std::string conname = "c8_" + std::to_string(v + 1) + "_" + std::to_string(m + 1) + "_" + std::to_string(d + 1);
					status = CPXchgname(env, problem, 'r', nb_constraints, conname.c_str());
					if (status != 0)
					{
						CPXgeterrorstring(env, status, error_text);
						throw std::runtime_error("Error in function IP_VNDS::build_problem(). \nCouldn't change constraint name. \nReason: " + std::string(error_text));
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
						throw std::runtime_error("Error in function IP_VNDS::build_problem(). Nonzeroes exceeds size of maxnonzeroes (matind and matval)");

					status = CPXaddrows(env, problem, 0, 1, nonzeroes, rhs, sense, matbeg, matind.get(), matval.get(), NULL, NULL);
					if (status != 0)
					{
						CPXgeterrorstring(env, status, error_text);
						throw std::runtime_error("Error in function IP_VNDS::build_problem(). \nCouldn't add constraint. \nReason: " + std::string(error_text));
					}

					// change name of constraint
					std::string conname = "c9_" + std::to_string(v + 1) + "_" + std::to_string(m + 1) + "_" + std::to_string(d + 1);
					status = CPXchgname(env, problem, 'r', nb_constraints, conname.c_str());
					if (status != 0)
					{
						CPXgeterrorstring(env, status, error_text);
						throw std::runtime_error("Error in function IP_VNDS::build_problem(). \nCouldn't change constraint name. \nReason: " + std::string(error_text));
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
				throw std::runtime_error("Error in function IP_VNDS::build_problem(). Nonzeroes exceeds size of maxnonzeroes (matind and matval)");

			status = CPXaddrows(env, problem, 0, 1, nonzeroes, rhs, sense, matbeg, matind.get(), matval.get(), NULL, NULL);
			if (status != 0)
			{
				CPXgeterrorstring(env, status, error_text);
				throw std::runtime_error("Error in function IP_VNDS::build_problem(). \nCouldn't add constraint. \nReason: " + std::string(error_text));
			}

			// change name of constraint
			std::string conname = "c10_" + std::to_string(m + 1);
			status = CPXchgname(env, problem, 'r', nb_constraints, conname.c_str());
			if (status != 0)
			{
				CPXgeterrorstring(env, status, error_text);
				throw std::runtime_error("Error in function IP_VNDS::build_problem(). \nCouldn't change constraint name. \nReason: " + std::string(error_text));
			}
		}

		// write to file
		status = CPXwriteprob(env, problem, "IP_VNDS.lp", NULL);
		if (status != 0)
		{
			CPXgeterrorstring(env, status, error_text);
			throw std::runtime_error("Error in function IP_VNDS::build_problem(). \nCouldn't write problem to lp-file. \nReason: " + std::string(error_text));
		}
	}

	void IP_VNDS::clear_cplex()
	{
		int status = 0;
		char error_text[CPXMESSAGEBUFSIZE];

		// Free the problem
		status = CPXfreeprob(env, &problem);
		if (status != 0)
		{
			CPXgeterrorstring(env, status, error_text);
			throw std::runtime_error("Error in function IP_VNDS::clear_cplex(). \nCouldn't free problem. \nReason: " + std::string(error_text));
		}

		// Close the cplex environment
		status = CPXcloseCPLEX(&env);
		if (status != 0)
		{
			CPXgeterrorstring(env, status, error_text);
			throw std::runtime_error("Error in function IP_VNDS::clear_cplex(). \nCouldn't close cplex environment. \nReason: " + std::string(error_text));
		}
	}


	bool IP_VNDS::solve_subproblem(double time_limit)
	{
		char error_text[CPXMESSAGEBUFSIZE];
		int status = 0;
		int solstat = 0;
		double objval;

		// Set time limit (in seconds)
		status = CPXsetdblparam(env, CPXPARAM_TimeLimit, time_limit);
		if (status != 0)
		{
			CPXgeterrorstring(env, status, error_text);
			throw std::runtime_error("Error in function IP_VNDS::solve_problem(). \nCouldn't set time limit. \nReason: " + std::string(error_text));
		}

		// Optimize the problem
		//std::cout << "\nCPLEX is solving the subproblem ...\n";
		status = CPXmipopt(env, problem);
		if (status != 0)
		{
			CPXgeterrorstring(env, status, error_text);
			throw std::runtime_error("Error in function IP_VNDS::solve_problem(). \nCPXmipopt failed. \nReason: " + std::string(error_text));
		}

		// Get the solution info
		status = CPXsolution(env, problem, &solstat, &objval, NULL, NULL, NULL, NULL);
		if (status != 0)
		{
			CPXgeterrorstring(env, status, error_text);
			std::cout << error_text;
			return false; // no solution exists
		}

		if (solstat == CPXMIP_OPTIMAL || solstat == CPXMIP_OPTIMAL_TOL)
		{
			//std::cout << "\nOptimal solution found";
			return true;
		}
		else if (solstat == CPXMIP_TIME_LIM_FEAS || solstat == CPXMIP_ABORT_FEAS)
		{
			//std::cout << "\nFeasible solution found";
			return true;
		}
		else
		{
			//std::cout << "\nNo feasible solution found";
			return false;
		}
	}

	void IP_VNDS::VNDS_initial_solution(const Data& data)
	{
		const int startindex_y1_vmd = 0;
		const int startindex_y2_vmd = data.vehicles() * data.nb_customers() * data.days();
		const int startindex_x1_vmd = startindex_y2_vmd + data.vehicles() * data.nb_customers() * data.days();
		const int startindex_x2_vmd = startindex_x1_vmd + data.vehicles() * data.nb_customers() * data.days();
		const int startindex_w_md = startindex_x2_vmd + data.vehicles() * data.nb_customers() * data.days();
		const int startindex_z = startindex_w_md + data.nb_customers() * data.days();

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
		int matind[1];			// Position of each element in constraint matrix
		double matval[1];		// Value of each element in constraint matrix

		matbeg[0] = 0;

		int indices[1];
		double values[1];


		// change objective y1_vmd to 0 (y2_vmd not necessary since variables set to 0 anyway)
		for (int v = 0; v < data.vehicles(); ++v)
		{
			for (int m = 0; m < data.nb_customers(); ++m)
			{
				for (int d = 0; d < data.days(); ++d)
				{
					indices[0] = startindex_y1_vmd + v * data.nb_customers() * data.days() + m * data.days() + d;
					values[0] = 0; // set obj coeff = 0
					status = CPXchgobj(env, problem, 1, indices, values);
					if (status != 0)
					{
						CPXgeterrorstring(env, status, error_text);
						throw std::runtime_error("Error in function IP_VNDS::VNDS_initial_solution(). \nCouldn't change objective coefficient y1_vmd. \nReason: " + std::string(error_text));
					}
				}
			}
		}

		// constraints counters
		int original_constraints = CPXgetnumrows(env, problem);
		int nb_constraints = original_constraints - 1; // adjust for indexing

		// set y2_vmd to 0
		for (int v = 0; v < data.vehicles(); ++v)
		{
			for (int m = 0; m < data.nb_customers(); ++m)
			{
				for (int d = 0; d < data.days(); ++d)
				{
					++nb_constraints;

					rhs[0] = 0;
					sense[0] = 'E';
					matbeg[0] = 0;

					nonzeroes = 0;

					// y2_vmd
					matind[nonzeroes] = startindex_y2_vmd + v * data.nb_customers() * data.days() + m * data.days() + d;
					matval[nonzeroes] = 1;
					++nonzeroes;

					status = CPXaddrows(env, problem, 0, 1, nonzeroes, rhs, sense, matbeg, matind, matval, NULL, NULL);
					if (status != 0)
					{
						CPXgeterrorstring(env, status, error_text);
						throw std::runtime_error("Error in function IP_VNDS::VNDS_initial_solution(). \nCouldn't add constraint. \nReason: " + std::string(error_text));
					}

					// change name of constraint
					std::string conname = "initsol_y2_" + std::to_string(v + 1) + "_" + std::to_string(m + 1) + "_" + std::to_string(d + 1);
					status = CPXchgname(env, problem, 'r', nb_constraints, conname.c_str());
					if (status != 0)
					{
						CPXgeterrorstring(env, status, error_text);
						throw std::runtime_error("Error in function IP_VNDS::VNDS_initial_solution(). \nCouldn't change constraint name. \nReason: " + std::string(error_text));
					}
				}
			}
		}

		// set x2_vmd to 0
		for (int v = 0; v < data.vehicles(); ++v)
		{
			for (int m = 0; m < data.nb_customers(); ++m)
			{
				for (int d = 0; d < data.days(); ++d)
				{
					++nb_constraints;

					rhs[0] = 0;
					sense[0] = 'E';
					matbeg[0] = 0;

					nonzeroes = 0;

					// x2_vmd
					matind[nonzeroes] = startindex_x2_vmd + v * data.nb_customers() * data.days() + m * data.days() + d;
					matval[nonzeroes] = 1;
					++nonzeroes;

					status = CPXaddrows(env, problem, 0, 1, nonzeroes, rhs, sense, matbeg, matind, matval, NULL, NULL);
					if (status != 0)
					{
						CPXgeterrorstring(env, status, error_text);
						throw std::runtime_error("Error in function IP_VNDS::VNDS_initial_solution(). \nCouldn't add constraint. \nReason: " + std::string(error_text));
					}

					// change name of constraint
					std::string conname = "initsol_x2_" + std::to_string(v + 1) + "_" + std::to_string(m + 1) + "_" + std::to_string(d + 1);
					status = CPXchgname(env, problem, 'r', nb_constraints, conname.c_str());
					if (status != 0)
					{
						CPXgeterrorstring(env, status, error_text);
						throw std::runtime_error("Error in function IP_VNDS::VNDS_initial_solution(). \nCouldn't change constraint name. \nReason: " + std::string(error_text));
					}
				}
			}
		}

		// write to file
		/*status = CPXwriteprob(env, problem, "IP_VNDS.lp", NULL);
		if (status != 0)
		{
			CPXgeterrorstring(env, status, error_text);
			throw std::runtime_error("Error in function IP_VNDS::VNDS_initial_solution(). \nCouldn't write problem to lp-file. \nReason: " + std::string(error_text));
		}*/

		// solve
		if (solve_subproblem(_time_limit_VNDS))
		{
			// solution found, store it
			double objval;
			_best_solution = std::make_unique<double[]>(CPXgetnumcols(env, problem));
			status = CPXsolution(env, problem, NULL, &objval, _best_solution.get(), NULL, NULL, NULL);
			if (status != 0)
			{
				CPXgeterrorstring(env, status, error_text);
				throw std::runtime_error("Error in function IP_VNDS::VNDS_initial_solution(). \nCouldn't access solution. \nReason: " + std::string(error_text));
			}

			// calculate real objective value and store it
			for (int v = 0; v < data.vehicles(); ++v)
			{
				for (int m = 0; m < data.nb_customers(); ++m)
				{
					for (int d = 0; d < data.days(); ++d)
					{
						// c_h * t1_m * y1_vmd
						objval += data.cost_hour() * data.t_trip1(m) * _best_solution[startindex_y1_vmd + v * data.nb_customers() * data.days() + m * data.days() + d];

						// y2_vmd is 0 forall v,m,d
					}
				}
			}
			_best_objval = objval;


			// change obj coeff back
			for (int v = 0; v < data.vehicles(); ++v)
			{
				for (int m = 0; m < data.nb_customers(); ++m)
				{
					for (int d = 0; d < data.days(); ++d)
					{
						indices[0] = startindex_y1_vmd + v * data.nb_customers() * data.days() + m * data.days() + d;;
						values[0] = data.cost_hour() * data.t_trip1(m); 
						status = CPXchgobj(env, problem, 1, indices, values);
						if (status != 0)
						{
							CPXgeterrorstring(env, status, error_text);
							throw std::runtime_error("Error in function IP_VNDS::VNDS_initial_solution(). \nCouldn't rechange objective coefficient y1_vmd. \nReason: " + std::string(error_text));
						}
					}
				}
			}

			// delete extra constraints
			status = CPXdelrows(env, problem, original_constraints, CPXgetnumrows(env, problem) - 1);

			// write to file
			/*status = CPXwriteprob(env, problem, "IP_VNDS.lp", NULL);
			if (status != 0)
			{
				CPXgeterrorstring(env, status, error_text);
				throw std::runtime_error("Error in function IP_VNDS::VNDS_initial_solution(). \nCouldn't write problem to lp-file. \nReason: " + std::string(error_text));
			}*/

		}
		else
		{
			// otherwise abort
			throw std::runtime_error("Error in IP_VNDS::VNDS_initial_solution().\nCould not find initial feasible solution within time limit.");
		}
	}

	double IP_VNDS::VNDS_neighborhood_days(const Data& data, size_t size)
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
		int matind[1];			// Position of each element in constraint matrix
		double matval[1];		// Value of each element in constraint matrix

		matbeg[0] = 0;

		const int startindex_y1_vmd = 0;
		const int startindex_y2_vmd = data.vehicles() * data.nb_customers() * data.days();

		std::chrono::duration<double, std::ratio<1, 1>> elapsed_time;

		// choose days
		std::uniform_int_distribution<int> distnbdays(3,5);
		//std::cout << "\nNeighborhood Days (" << size << ")";
		//std::cout << "\n\tSize: " << neighborhood_size;

		std::vector<int> days_free;
		std::uniform_int_distribution<int> dist(0, data.days() - 1);
		while (days_free.size() < size)
		{
			int rday = dist(Random::generator);
			if (std::find(days_free.begin(), days_free.end(), rday) == days_free.end())
				days_free.push_back(rday);
		}


		// constraints counters
		int original_constraints = CPXgetnumrows(env, problem);
		int nb_constraints = original_constraints - 1; // adjust for indexing

		// fix current solution, except for chosen days
		for (int y = 0; y < 2; ++y)
		{
			for (int v = 0; v < data.vehicles(); ++v)
			{
				for (int m = 0; m < data.nb_customers(); ++m)
				{
					for (int d = 0; d < data.days(); ++d)
					{
						bool free = false;
						for (auto&& dfree : days_free)
						{
							if (d == dfree)
							{
								free = true;
								break;
							}
						}

						if(!free)
						{
							++nb_constraints;

							rhs[0] = _current_solution[y * startindex_y2_vmd + v * data.nb_customers() * data.days() + m * data.days() + d];
							sense[0] = 'E';
							matbeg[0] = 0;

							nonzeroes = 0;

							// y1of2_vmd
							matind[nonzeroes] = y * startindex_y2_vmd + v * data.nb_customers() * data.days() + m * data.days() + d;
							matval[nonzeroes] = 1;
							++nonzeroes;

							status = CPXaddrows(env, problem, 0, 1, nonzeroes, rhs, sense, matbeg, matind, matval, NULL, NULL);
							if (status != 0)
							{
								CPXgeterrorstring(env, status, error_text);
								throw std::runtime_error("Error in function IP_VNDS::VNDS_neighborhood_days(). \nCouldn't add constraint. \nReason: " + std::string(error_text));
							}

							// change name of constraint
							std::string conname = "N_days_" + std::to_string(y + 1) + "_" + std::to_string(v + 1) + "_"
								+ std::to_string(m + 1) + "_" + std::to_string(d + 1);
							status = CPXchgname(env, problem, 'r', nb_constraints, conname.c_str());
							if (status != 0)
							{
								CPXgeterrorstring(env, status, error_text);
								throw std::runtime_error("Error in function IP_VNDS::VNDS_neighborhood_days(). \nCouldn't change constraint name. \nReason: " + std::string(error_text));
							}
						}
					}
				}
			}
		}

		// write to file
		/*status = CPXwriteprob(env, problem, "IP_VNDS.lp", NULL);
		if (status != 0)
		{
			CPXgeterrorstring(env, status, error_text);
			throw std::runtime_error("Error in function IP_VNDS::VNDS_neighborhood_days(). \nCouldn't write problem to lp-file. \nReason: " + std::string(error_text));
		}*/



		// solve problem
		bool solfound = solve_subproblem(_time_limit_subproblem);
		double objval;
		if (solfound)
		{
			// solution found, store objval
			_new_solution = std::make_unique<double[]>(CPXgetnumcols(env, problem));
			status = CPXsolution(env, problem, NULL, &objval, _new_solution.get(), NULL, NULL, NULL);
			if (status != 0)
			{
				CPXgeterrorstring(env, status, error_text);
				throw std::runtime_error("Error in function IP_VNDS::VNDS_neighborhood_days(). \nCouldn't access solution. \nReason: " + std::string(error_text));
			}
		}

		// delete extra constraints
		status = CPXdelrows(env, problem, original_constraints, CPXgetnumrows(env, problem) - 1);

		// write to file
		/*status = CPXwriteprob(env, problem, "IP_VNDS.lp", NULL);
		if (status != 0)
		{
			CPXgeterrorstring(env, status, error_text);
			throw std::runtime_error("Error in function IP_VNDS::VNDS_neighborhood_days(). \nCouldn't write problem to lp-file. \nReason: " + std::string(error_text));
		}*/

		// return value
		if (solfound)
		{
			return objval;
		}
		else
		{
			// no solution found, so high value
			return 1e100;
		}
	}

	double IP_VNDS::VNDS_neighborhood_customers(const Data& data, size_t size)
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
		int matind[1];			// Position of each element in constraint matrix
		double matval[1];		// Value of each element in constraint matrix

		matbeg[0] = 0;

		const int startindex_y1_vmd = 0;
		const int startindex_y2_vmd = data.vehicles() * data.nb_customers() * data.days();

		std::chrono::duration<double, std::ratio<1, 1>> elapsed_time;

		// PARAM
		std::uniform_int_distribution<int> distnbcust(3,6);
		//std::cout << "\nNeighborhood Customers (" << size << ")";
		//std::cout << "\n\tSize: " << neighborhood_size;

		// choose customers
		std::vector<int> customers_free;
		std::uniform_int_distribution<int> dist(0, data.nb_customers() - 1);
		while (customers_free.size() < size)
		{
			int rcustomer = dist(Random::generator);
			if (std::find(customers_free.begin(), customers_free.end(), rcustomer) == customers_free.end())
				customers_free.push_back(rcustomer);
		}


		// constraints counters
		int original_constraints = CPXgetnumrows(env, problem);
		int nb_constraints = original_constraints - 1; // adjust for indexing

		// fix current solution, except for chosen days
		for (int y = 0; y < 2; ++y)
		{
			for (int v = 0; v < data.vehicles(); ++v)
			{
				for (int m = 0; m < data.nb_customers(); ++m)
				{
					for (int d = 0; d < data.days(); ++d)
					{
						bool free = false;
						for (auto&& mfree : customers_free)
						{
							if (m == mfree)
							{
								free = true;
								break;
							}
						}

						if (!free)
						{
							++nb_constraints;

							rhs[0] = _current_solution[y * startindex_y2_vmd + v * data.nb_customers() * data.days() + m * data.days() + d];
							sense[0] = 'E';
							matbeg[0] = 0;

							nonzeroes = 0;

							// y1of2_vmd
							matind[nonzeroes] = y * startindex_y2_vmd + v * data.nb_customers() * data.days() + m * data.days() + d;
							matval[nonzeroes] = 1;
							++nonzeroes;

							status = CPXaddrows(env, problem, 0, 1, nonzeroes, rhs, sense, matbeg, matind, matval, NULL, NULL);
							if (status != 0)
							{
								CPXgeterrorstring(env, status, error_text);
								throw std::runtime_error("Error in function IP_VNDS::VNDS_neighborhood_days(). \nCouldn't add constraint. \nReason: " + std::string(error_text));
							}

							// change name of constraint
							std::string conname = "N_customers_" + std::to_string(y + 1) + "_" + std::to_string(v + 1) + "_"
								+ std::to_string(m + 1) + "_" + std::to_string(d + 1);
							status = CPXchgname(env, problem, 'r', nb_constraints, conname.c_str());
							if (status != 0)
							{
								CPXgeterrorstring(env, status, error_text);
								throw std::runtime_error("Error in function IP_VNDS::VNDS_neighborhood_days(). \nCouldn't change constraint name. \nReason: " + std::string(error_text));
							}
						}
					}
				}
			}
		}

		// write to file
		/*status = CPXwriteprob(env, problem, "IP_VNDS.lp", NULL);
		if (status != 0)
		{
			CPXgeterrorstring(env, status, error_text);
			throw std::runtime_error("Error in function IP_VNDS::VNDS_initial_solution(). \nCouldn't write problem to lp-file. \nReason: " + std::string(error_text));
		}*/



		// solve problem
		bool solfound = solve_subproblem(_time_limit_subproblem);
		double objval;
		if (solfound)
		{
			// solution found, store objval
			_new_solution = std::make_unique<double[]>(CPXgetnumcols(env, problem));
			status = CPXsolution(env, problem, NULL, &objval, _new_solution.get(), NULL, NULL, NULL);
			if (status != 0)
			{
				CPXgeterrorstring(env, status, error_text);
				throw std::runtime_error("Error in function IP_VNDS::VNDS_initial_solution(). \nCouldn't access solution. \nReason: " + std::string(error_text));
			}
		}

		// delete extra constraints
		status = CPXdelrows(env, problem, original_constraints, CPXgetnumrows(env, problem) - 1);

		// write to file
		/*status = CPXwriteprob(env, problem, "IP_VNDS.lp", NULL);
		if (status != 0)
		{
			CPXgeterrorstring(env, status, error_text);
			throw std::runtime_error("Error in function IP_VNDS::VNDS_initial_solution(). \nCouldn't write problem to lp-file. \nReason: " + std::string(error_text));
		}*/

		// return value
		if (solfound)
		{
			return objval;
		}
		else
		{
			// no solution found, so high value
			return 1e100;
		}
	}

	double IP_VNDS::VNDS_neighborhood_vehicles(const Data& data, size_t size)
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
		int matind[1];			// Position of each element in constraint matrix
		double matval[1];		// Value of each element in constraint matrix

		matbeg[0] = 0;

		const int startindex_y1_vmd = 0;
		const int startindex_y2_vmd = data.vehicles() * data.nb_customers() * data.days();

		std::chrono::duration<double, std::ratio<1, 1>> elapsed_time;

		// PARAM
		std::uniform_int_distribution<int> distnbcust(2, 4);
		//std::cout << "\nNeighborhood Vehicles (" << size << ")";
		//std::cout << "\n\tSize: " << neighborhood_size;

		// choose customers
		std::vector<int> vehicles_free;
		std::uniform_int_distribution<int> dist(0, data.vehicles() - 1);
		while (vehicles_free.size() < size)
		{
			int rvehicle = dist(Random::generator);
			if (std::find(vehicles_free.begin(), vehicles_free.end(), rvehicle) == vehicles_free.end())
				vehicles_free.push_back(rvehicle);
		}


		// constraints counters
		int original_constraints = CPXgetnumrows(env, problem);
		int nb_constraints = original_constraints - 1; // adjust for indexing

		// fix current solution, except for chosen days
		for (int y = 0; y < 2; ++y)
		{
			for (int v = 0; v < data.vehicles(); ++v)
			{
				for (int m = 0; m < data.nb_customers(); ++m)
				{
					for (int d = 0; d < data.days(); ++d)
					{
						bool free = false;
						for (auto&& vfree : vehicles_free)
						{
							if (v == vfree)
							{
								free = true;
								break;
							}
						}

						if (!free)
						{
							++nb_constraints;

							rhs[0] = _current_solution[y * startindex_y2_vmd + v * data.nb_customers() * data.days() + m * data.days() + d];
							sense[0] = 'E';
							matbeg[0] = 0;

							nonzeroes = 0;

							// y1of2_vmd
							matind[nonzeroes] = y * startindex_y2_vmd + v * data.nb_customers() * data.days() + m * data.days() + d;
							matval[nonzeroes] = 1;
							++nonzeroes;

							status = CPXaddrows(env, problem, 0, 1, nonzeroes, rhs, sense, matbeg, matind, matval, NULL, NULL);
							if (status != 0)
							{
								CPXgeterrorstring(env, status, error_text);
								throw std::runtime_error("Error in function IP_VNDS::VNDS_neighborhood_days(). \nCouldn't add constraint. \nReason: " + std::string(error_text));
							}

							// change name of constraint
							std::string conname = "N_vehicles_" + std::to_string(y + 1) + "_" + std::to_string(v + 1) + "_"
								+ std::to_string(m + 1) + "_" + std::to_string(d + 1);
							status = CPXchgname(env, problem, 'r', nb_constraints, conname.c_str());
							if (status != 0)
							{
								CPXgeterrorstring(env, status, error_text);
								throw std::runtime_error("Error in function IP_VNDS::VNDS_neighborhood_days(). \nCouldn't change constraint name. \nReason: " + std::string(error_text));
							}
						}
					}
				}
			}
		}

		// write to file
		/*status = CPXwriteprob(env, problem, "IP_VNDS.lp", NULL);
		if (status != 0)
		{
			CPXgeterrorstring(env, status, error_text);
			throw std::runtime_error("Error in function IP_VNDS::VNDS_neighborhood_vehicles(). \nCouldn't write problem to lp-file. \nReason: " + std::string(error_text));
		}*/



		// solve problem
		bool solfound = solve_subproblem(_time_limit_subproblem);
		double objval;
		if (solfound)
		{
			// solution found, store objval
			_new_solution = std::make_unique<double[]>(CPXgetnumcols(env, problem));
			status = CPXsolution(env, problem, NULL, &objval, _new_solution.get(), NULL, NULL, NULL);
			if (status != 0)
			{
				CPXgeterrorstring(env, status, error_text);
				throw std::runtime_error("Error in function IP_VNDS::VNDS_neighborhood_vehicles(). \nCouldn't access solution. \nReason: " + std::string(error_text));
			}
		}

		// delete extra constraints
		status = CPXdelrows(env, problem, original_constraints, CPXgetnumrows(env, problem) - 1);

		// write to file
		/*status = CPXwriteprob(env, problem, "IP_VNDS.lp", NULL);
		if (status != 0)
		{
			CPXgeterrorstring(env, status, error_text);
			throw std::runtime_error("Error in function IP_VNDS::VNDS_neighborhood_vehicles(). \nCouldn't write problem to lp-file. \nReason: " + std::string(error_text));
		}*/

		// return value
		if (solfound)
		{
			return objval;
		}
		else
		{
			// no solution found, so high value
			return 1e100;
		}
	}

	double IP_VNDS::VNDS_neighborhood_TEST(const Data& data)
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
		int matind[1];			// Position of each element in constraint matrix
		double matval[1];		// Value of each element in constraint matrix

		matbeg[0] = 0;

		const int startindex_y1_vmd = 0;
		const int startindex_y2_vmd = data.vehicles() * data.nb_customers() * data.days();

		std::chrono::duration<double, std::ratio<1, 1>> elapsed_time;


		// constraints counters
		int original_constraints = CPXgetnumrows(env, problem);
		int nb_constraints = original_constraints - 1; // adjust for indexing

		// PARAM
		int neighborhood_size_vehicles = 5;
		int neighborhood_size_days = 5;

		// choose vehicles and days
		std::uniform_int_distribution<> dist_vehicles(0, data.vehicles() - 1);
		std::uniform_int_distribution<> dist_days(0, data.days() - 1);

		std::vector<int> vehicles_free;
		while (vehicles_free.size() < neighborhood_size_vehicles)
		{
			int rvehicle = dist_vehicles(Random::generator);
			if (std::find(vehicles_free.begin(), vehicles_free.end(), rvehicle) == vehicles_free.end())
				vehicles_free.push_back(rvehicle);
		}

		std::vector<int> days_free;
		while (days_free.size() < neighborhood_size_days)
		{
			int rday = dist_days(Random::generator);
			if (std::find(days_free.begin(), days_free.end(), rday) == days_free.end())
				days_free.push_back(rday);
		}

		// fix current solution, except for chosen vehicle & day
		for (int y = 0; y < 2; ++y)
		{
			for (int v = 0; v < data.vehicles(); ++v)
			{
				for (int m = 0; m < data.nb_customers(); ++m)
				{
					for (int d = 0; d < data.days(); ++d)
					{
						bool free = false;
						for (auto&& vehicle : vehicles_free) {
							for (auto&& day : days_free) {
								if (v == vehicle && d == day) {
									free = true;
									break;
								}
							}
						}				

						if (!free)
						{
							++nb_constraints;

							rhs[0] = _current_solution[y * startindex_y2_vmd + v * data.nb_customers() * data.days() + m * data.days() + d];
							sense[0] = 'E';
							matbeg[0] = 0;

							nonzeroes = 0;

							// y1of2_vmd
							matind[nonzeroes] = y * startindex_y2_vmd + v * data.nb_customers() * data.days() + m * data.days() + d;
							matval[nonzeroes] = 1;
							++nonzeroes;

							status = CPXaddrows(env, problem, 0, 1, nonzeroes, rhs, sense, matbeg, matind, matval, NULL, NULL);
							if (status != 0)
							{
								CPXgeterrorstring(env, status, error_text);
								throw std::runtime_error("Error in function IP_VNDS::VNDS_neighborhood_days(). \nCouldn't add constraint. \nReason: " + std::string(error_text));
							}

							// change name of constraint
							std::string conname = "N_vehicles_" + std::to_string(y + 1) + "_" + std::to_string(v + 1) + "_"
								+ std::to_string(m + 1) + "_" + std::to_string(d + 1);
							status = CPXchgname(env, problem, 'r', nb_constraints, conname.c_str());
							if (status != 0)
							{
								CPXgeterrorstring(env, status, error_text);
								throw std::runtime_error("Error in function IP_VNDS::VNDS_neighborhood_days(). \nCouldn't change constraint name. \nReason: " + std::string(error_text));
							}
						}
					}
				}
			}
		}

		// write to file
		/*status = CPXwriteprob(env, problem, "IP_VNDS.lp", NULL);
		if (status != 0)
		{
			CPXgeterrorstring(env, status, error_text);
			throw std::runtime_error("Error in function IP_VNDS::VNDS_neighborhood_vehicles(). \nCouldn't write problem to lp-file. \nReason: " + std::string(error_text));
		}*/



		// solve problem
		bool solfound = solve_subproblem(_time_limit_subproblem);
		double objval;
		if (solfound)
		{
			// solution found, store objval
			_new_solution = std::make_unique<double[]>(CPXgetnumcols(env, problem));
			status = CPXsolution(env, problem, NULL, &objval, _new_solution.get(), NULL, NULL, NULL);
			if (status != 0)
			{
				CPXgeterrorstring(env, status, error_text);
				throw std::runtime_error("Error in function IP_VNDS::VNDS_neighborhood_vehicles(). \nCouldn't access solution. \nReason: " + std::string(error_text));
			}
		}

		// delete extra constraints
		status = CPXdelrows(env, problem, original_constraints, CPXgetnumrows(env, problem) - 1);

		// write to file
		/*status = CPXwriteprob(env, problem, "IP_VNDS.lp", NULL);
		if (status != 0)
		{
			CPXgeterrorstring(env, status, error_text);
			throw std::runtime_error("Error in function IP_VNDS::VNDS_neighborhood_vehicles(). \nCouldn't write problem to lp-file. \nReason: " + std::string(error_text));
		}*/

		// return value
		if (solfound)
		{
			return objval;
		}
		else
		{
			// no solution found, so high value
			return 1e100;
		}
	}

	bool IP_VNDS::VNDS_shaking(const Data& data, double& objval)
	{
		const int startindex_y1_vmd = 0;
		const int startindex_y2_vmd = data.vehicles() * data.nb_customers() * data.days();
		const int startindex_x1_vmd = startindex_y2_vmd + data.vehicles() * data.nb_customers() * data.days();
		const int startindex_x2_vmd = startindex_x1_vmd + data.vehicles() * data.nb_customers() * data.days();
		const int startindex_w_md = startindex_x2_vmd + data.vehicles() * data.nb_customers() * data.days();
		const int startindex_z = startindex_w_md + data.nb_customers() * data.days();

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
		int matind[1];			// Position of each element in constraint matrix
		double matval[1];		// Value of each element in constraint matrix

		matbeg[0] = 0;

		int indices[1];
		double values[1];

		std::chrono::duration<double, std::ratio<1, 1>> elapsed_time;

		
		std::uniform_int_distribution<> dist_coeff(1, 10);


		//status = CPXchgobjsen(env, problem, CPX_MAX);

		// change objective y1_vmd and y2_vmd to random value
		for (int v = 0; v < data.vehicles(); ++v)
		{
			for (int m = 0; m < data.nb_customers(); ++m)
			{
				for (int d = 0; d < data.days(); ++d)
				{
					// variables y1_vmd
					indices[0] = startindex_y1_vmd + v * data.nb_customers() * data.days() + m * data.days() + d;
					values[0] = dist_coeff(Random::generator);
					status = CPXchgobj(env, problem, 1, indices, values);
					if (status != 0)
					{
						CPXgeterrorstring(env, status, error_text);
						throw std::runtime_error("Error in function IP_VNDS::VNDS_shaking(). \nCouldn't change objective coefficient y1_vmd. \nReason: " + std::string(error_text));
					}

					// variables y2_vmd
					indices[0] = startindex_y2_vmd + v * data.nb_customers() * data.days() + m * data.days() + d;
					values[0] = dist_coeff(Random::generator);
					status = CPXchgobj(env, problem, 1, indices, values);
					if (status != 0)
					{
						CPXgeterrorstring(env, status, error_text);
						throw std::runtime_error("Error in function IP_VNDS::VNDS_shaking(). \nCouldn't change objective coefficient y2_vmd. \nReason: " + std::string(error_text));
					}
				}
			}
		}

		// change obj coeff variable z to random value
		indices[0] = startindex_z;
		values[0] = 0; //dist_coeff(Random::generator); 
		status = CPXchgobj(env, problem, 1, indices, values);
		if (status != 0)
		{
			CPXgeterrorstring(env, status, error_text);
			throw std::runtime_error("Error in function IP_VNDS::VNDS_shaking(). \nCouldn't change objective coefficient z. \nReason: " + std::string(error_text));
		}

		// write to file
		/*status = CPXwriteprob(env, problem, "IP_VNDS.lp", NULL);
		if (status != 0)
		{
			CPXgeterrorstring(env, status, error_text);
			throw std::runtime_error("Error in function IP_VNDS::VNDS_initial_solution(). \nCouldn't write problem to lp-file. \nReason: " + std::string(error_text));
		}*/

		// solve problem
		bool solfound = solve_subproblem(_time_limit_subproblem);
		if (solfound)
		{
			// solution found, store objval
			_current_solution = std::make_unique<double[]>(CPXgetnumcols(env, problem));
			status = CPXsolution(env, problem, NULL, NULL, _current_solution.get(), NULL, NULL, NULL);
			if (status != 0)
			{
				CPXgeterrorstring(env, status, error_text);
				throw std::runtime_error("Error in function IP_VNDS::VNDS_shaking(). \nCouldn't access solution. \nReason: " + std::string(error_text));
			}

			// calculate real objective value and store it
			objval = 0;
			for (int v = 0; v < data.vehicles(); ++v)
			{
				for (int m = 0; m < data.nb_customers(); ++m)
				{
					for (int d = 0; d < data.days(); ++d)
					{
						// c_h * t1_m * y1_vmd
						objval += data.cost_hour() * data.t_trip1(m) * _best_solution[startindex_y1_vmd + v * data.nb_customers() * data.days() + m * data.days() + d];

						// c_h * t2_m * y2_vmd
						objval += data.cost_hour() * data.t_trip2(m) * _best_solution[startindex_y2_vmd + v * data.nb_customers() * data.days() + m * data.days() + d];
					}
				}
			}
			// c_veh*z
			objval += data.cost_vehicle() * _current_solution[startindex_z];
			//std::cout << "\nobjval = " << objval;
		}

		//status = CPXchgobjsen(env, problem, CPX_MIN);
		
		// change obj coeff y1_vmd & y2_vmd back to real values
		for (int v = 0; v < data.vehicles(); ++v)
		{
			for (int m = 0; m < data.nb_customers(); ++m)
			{
				for (int d = 0; d < data.days(); ++d)
				{
					// variables y1_vmd
					indices[0] = startindex_y1_vmd + v * data.nb_customers() * data.days() + m * data.days() + d;
					values[0] = data.cost_hour() * data.t_trip1(m);
					status = CPXchgobj(env, problem, 1, indices, values);
					if (status != 0)
					{
						CPXgeterrorstring(env, status, error_text);
						throw std::runtime_error("Error in function IP_VNDS::VNDS_shaking(). \nCouldn't rechange objective coefficient y1_vmd. \nReason: " + std::string(error_text));
					}

					// variables y2_vmd
					indices[0] = startindex_y2_vmd + v * data.nb_customers() * data.days() + m * data.days() + d;
					values[0] = data.cost_hour() * data.t_trip2(m);
					status = CPXchgobj(env, problem, 1, indices, values);
					if (status != 0)
					{
						CPXgeterrorstring(env, status, error_text);
						throw std::runtime_error("Error in function IP_VNDS::VNDS_shaking(). \nCouldn't rechange objective coefficient y2_vmd. \nReason: " + std::string(error_text));
					}
				}
			}
		}

		// change obj coeff variable z back to real value
		indices[0] = startindex_z;
		values[0] = data.cost_vehicle();  
		status = CPXchgobj(env, problem, 1, indices, values);
		if (status != 0)
		{
			CPXgeterrorstring(env, status, error_text);
			throw std::runtime_error("Error in function IP_VNDS::VNDS_shaking(). \nCouldn't rechange objective coefficient z. \nReason: " + std::string(error_text));
		}

		// write to file
		/*status = CPXwriteprob(env, problem, "IP_VNDS.lp", NULL);
		if (status != 0)
		{
			CPXgeterrorstring(env, status, error_text);
			throw std::runtime_error("Error in function IP_VNDS::VNDS_shaking(). \nCouldn't write problem to lp-file. \nReason: " + std::string(error_text));
		}*/

		// return value
		if (solfound)
			return true;
		else
			return false;
	}

	void IP_VNDS::run(const Data& data)
	{
		int status = 0;
		std::chrono::duration<double, std::ratio<1, 1>> elapsed_time;
		double objval_currentsol = 1e100;
		double objval_newsol = 1e100;
		std::uniform_int_distribution<> dist_neighborhood(1, 9);

		// Time
		_start_time = std::chrono::system_clock::now();

		// Initialization
		initialize_cplex();
		build_problem(data);

		// set CPLEX relative MIP optimality tolerance
		const double MIP_Optimality_Tolerance = 0.01;
		status = CPXsetdblparam(env, CPXPARAM_MIP_Tolerances_MIPGap, MIP_Optimality_Tolerance);

		// Find initial feasible solution
		std::cout << "\n\nFinding initial solution ...";
		VNDS_initial_solution(data); // stores solution in _best_solution
		objval_currentsol = _best_objval;
		_current_solution = std::make_unique<double[]>(CPXgetnumcols(env, problem));
		for (int i = 0; i < CPXgetnumcols(env, problem); ++i)
			_current_solution[i] = _best_solution[i];
		std::cout << "\nInitial solution found with objective value " << _best_objval;
		
		// VNDS loop
		std::cout << "\n\n\nStarting VNDS loop ...";
		/*size_t size_neighborhood_vehicles = 0;
		while (true)
		{
			++size_neighborhood_vehicles;
			std::cout << "\n\n\nIncreasing size neighborhood VEHICLES to " << size_neighborhood_vehicles;

			if (size_neighborhood_vehicles > 15)
				break;

			// loop over all possible neighborhoods VEHICLES of the given size
			for (size_t v = 0; v < data.vehicles() + 1 - size_neighborhood_vehicles; ++v)
			{
				// time check
				elapsed_time = std::chrono::system_clock::now() - _start_time;
				if (elapsed_time.count() > _time_limit_VNDS)
					break;

				objval_newsol = VNDS_neighborhood_vehicles_deterministic(data, size_neighborhood_vehicles, v);

				// check if better solution found
				if (objval_newsol < objval_currentsol)
				{
					std::cout << "\nImprovement! Current best solution: " << objval_newsol << "\n";

					// update objval_currentsol
					objval_currentsol = objval_newsol;

					// save solution 
					for (int i = 0; i < CPXgetnumcols(env, problem); ++i)
						_current_solution[i] = _new_solution[i];
				}
				else
					std::cout << "\nNo improvement ...";
			}

			// time check
			elapsed_time = std::chrono::system_clock::now() - _start_time;
			if (elapsed_time.count() > _time_limit_VNDS)
				break;
		}*/

		size_t size_neighborhood_vehicles = 1;
		size_t size_neighborhood_days = 3;
		size_t size_neighborhood_customers = 3;

		// VNDS loop
		while (true)
		{
			// VND loop (local search)
			std::cout << "\n\n\nStarting VND phase ...";
			
			size_t iterations_VND = 0;
			while (iterations_VND < _max_iterations_VND)
			{
				++iterations_VND;

				const size_t max_iterations_NV = 5;
				size_t iterations_NV = 0;
				while (iterations_NV < max_iterations_NV)
				{
					++iterations_NV;

					// time check
					elapsed_time = std::chrono::system_clock::now() - _start_time;
					if (elapsed_time.count() > _time_limit_VNDS)
						break;

					// neighborhood vehicles
					objval_newsol = VNDS_neighborhood_vehicles(data, size_neighborhood_vehicles);
					std::cout << "\n\nNeighborhood Vehicles (" << size_neighborhood_vehicles << "). Objval = " << objval_newsol;

					// check if better solution found
					if (objval_newsol < objval_currentsol)
					{
						std::cout << "\nImprovement! Current best solution: " << objval_newsol << "\n";

						// update objval_currentsol
						objval_currentsol = objval_newsol;

						// save solution 
						for (int i = 0; i < CPXgetnumcols(env, problem); ++i)
							_current_solution[i] = _new_solution[i];

						// reset iterations VND
						iterations_VND = 0;
					}
				}
				++size_neighborhood_vehicles;
				std::cout << "\n\nStop inner loop neighborhood vehicles";
				if (size_neighborhood_vehicles > 3)
					size_neighborhood_vehicles = 1;


				// time check
				elapsed_time = std::chrono::system_clock::now() - _start_time;
				if (elapsed_time.count() > _time_limit_VNDS)
					break;


				// neighborhood days
				objval_newsol = VNDS_neighborhood_days(data, size_neighborhood_days);
				std::cout << "\n\nNeighborhood Days (" << size_neighborhood_days << "). Objval = " << objval_newsol;

				// check if better solution found
				if (objval_newsol < objval_currentsol)
				{
					std::cout << "\nImprovement! Current best solution: " << objval_newsol << "\n";

					// update objval_currentsol
					objval_currentsol = objval_newsol;

					// save solution 
					for (int i = 0; i < CPXgetnumcols(env, problem); ++i)
						_current_solution[i] = _new_solution[i];

					// reset iterations VND
					iterations_VND = 0;
				}


				// time check
				elapsed_time = std::chrono::system_clock::now() - _start_time;
				if (elapsed_time.count() > _time_limit_VNDS)
					break;


				// neighborhood customers
				objval_newsol = VNDS_neighborhood_customers(data, size_neighborhood_customers);
				std::cout << "\n\nNeighborhood Customers (" << size_neighborhood_customers << "). Objval = " << objval_newsol;

				// check if better solution found
				if (objval_newsol < objval_currentsol)
				{
					std::cout << "\nImprovement! Current best solution: " << objval_newsol << "\n";

					// update objval_currentsol
					objval_currentsol = objval_newsol;

					// save solution 
					for (int i = 0; i < CPXgetnumcols(env, problem); ++i)
						_current_solution[i] = _new_solution[i];

					// reset iterations VND
					//iterations_VND = 0;
				}


				// choose neighborhood 
				//int neighborhood = dist_neighborhood(Random::generator);

				// search neighborhood
				//if (neighborhood <= 5 )
				//	objval_newsol = VNDS_neighborhood_vehicles_random(data);
				//else if (neighborhood <= 7)
				//	objval_newsol = VNDS_neighborhood_days(data);
				//else
				//	objval_newsol = VNDS_neighborhood_customers(data);

				//std::cout << "\n\nNew solution with objective value " << objval_newsol;
			}

			std::cout << "\n\n\nMax iterations (outer loop) reached";
			
			// if improvement
			if (objval_currentsol < _best_objval)
			{
				std::cout << "\nOverall best solution improved. Best sol objval = " << objval_currentsol;

				// update objval
				_best_objval = objval_currentsol;
			
				// save solution 
				for (int i = 0; i < CPXgetnumcols(env, problem); ++i)
					_best_solution[i] = _current_solution[i];
			}

			// time check
			elapsed_time = std::chrono::system_clock::now() - _start_time;
			if (elapsed_time.count() > _time_limit_VNDS)
			{
				std::cout << "\n\n\n\nTime limit reached. STOP.";
				break;
			}

			// Move to shake phase
			std::cout << "\n\nMove to shake phase";
			size_neighborhood_vehicles = 1;
			if (VNDS_shaking(data, objval_currentsol))
				std::cout << "\nSolution shaken - new objective value = " << objval_currentsol << "\n";
			else
				std::cout << "\nShake failed to find solution\n";
		}



		// report solution
		std::cout << "\n\n\nBest solution: " << _best_objval;

		// Free CPLEX memory
		clear_cplex();
	}
}