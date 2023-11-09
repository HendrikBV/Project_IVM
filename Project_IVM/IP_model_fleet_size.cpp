#include "models.h"
#include "data.h"
#include <stdexcept>

namespace IVM
{

	void IP_model_fleet_size::initialize_cplex()
	{
		int status = 0;
		char error_text[CPXMESSAGEBUFSIZE];

		// open the CPLEX environment
		env = CPXopenCPLEX(&status);
		if (status != 0)
		{
			CPXgeterrorstring(env, status, error_text);
			throw std::runtime_error("Error in function IP_model_fleet_size::initialize_cplex(). \nCouldn't open CPLEX. \nReason: " + std::string(error_text));
		}

		// turn output to screen on/off
		status = CPXsetintparam(env, CPX_PARAM_SCRIND, CPX_ON);
		if (status != 0)
		{
			CPXgeterrorstring(env, status, error_text);
			throw std::runtime_error("Error in function IP_model_fleet_size::initialize_cplex(). \nCouldn't change param SCRIND. \nReason: " + std::string(error_text));
		}
	}

	void IP_model_fleet_size::build_problem(const Data& data)
	{
		char error_text[CPXMESSAGEBUFSIZE];

		double obj[1];			// Objective function
		double lb[1];			// Lower bound variables
		double ub[1];			// Upper bound variables
		double rhs[1];			// Right-hand side constraints
		char sense[1];			// Sign of constraint
		int matbeg[1];			// Begin position of the constraint
		int matind[2000];		// Position of each element in constraint matrix
		double matval[2000];	// Value of each element in constraint matrix
		char type[1];			// Type of variable (integer, binary, fractional)
		int nonzeroes = 0;		// To calculate number of nonzero coefficients in each constraint

		int status = 0;

		// create the problem
		problem = CPXcreateprob(env, &status, "IP_model_fleet_size");

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
					obj[0] = 0;
					lb[0] = 0;
					ub[0] = 1;
					type[0] = 'B';

					status = CPXnewcols(env, problem, 1, obj, lb, ub, type, NULL);
					if (status != 0)
					{
						CPXgeterrorstring(env, status, error_text);
						throw std::runtime_error("Error in function IP_model_fleet_size::build_problem(). \nCouldn't add variable. \nReason: " + std::string(error_text));
					}

					// change variable name
					std::string varname = "y1_" + std::to_string(v + 1) + "_" + std::to_string(m + 1) + "_" + std::to_string(d + 1);
					status = CPXchgname(env, problem, 'c', v * data.nb_customers() * data.days() + m * data.days() + d, varname.c_str());
					if (status != 0)
					{
						CPXgeterrorstring(env, status, error_text);
						throw std::runtime_error("Error in function IP_model_fleet_size::build_problem(). \nCouldn't change variable name. \nReason: " + std::string(error_text));
					}
				}
			}
		}

		// variable x_1_vmd
		const int startindex_x1_vmd = data.vehicles() * data.nb_customers() * data.days();
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
						throw std::runtime_error("Error in function IP_model_fleet_size::build_problem(). \nCouldn't add variable. \nReason: " + std::string(error_text));
					}

					// change variable name
					std::string varname = "x1_" + std::to_string(v + 1) + "_" + std::to_string(m + 1) + "_" + std::to_string(d + 1);
					status = CPXchgname(env, problem, 'c', startindex_x1_vmd + v * data.nb_customers() * data.days() + m * data.days() + d, varname.c_str());
					if (status != 0)
					{
						CPXgeterrorstring(env, status, error_text);
						throw std::runtime_error("Error in function IP_model_fleet_size::build_problem(). \nCouldn't change variable name. \nReason: " + std::string(error_text));
					}
				}
			}
		}

		// variable w_md
		const int startindex_w_md = 2 * data.vehicles() * data.nb_customers() * data.days();
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
					throw std::runtime_error("Error in function IP_model_fleet_size::build_problem(). \nCouldn't add variable. \nReason: " + std::string(error_text));
				}

				// change variable name
				std::string varname = "w_" + std::to_string(m + 1) + "_" + std::to_string(d + 1);
				status = CPXchgname(env, problem, 'c', startindex_w_md + m * data.days() + d, varname.c_str());
				if (status != 0)
				{
					CPXgeterrorstring(env, status, error_text);
					throw std::runtime_error("Error in function IP_model_fleet_size::build_problem(). \nCouldn't change variable name. \nReason: " + std::string(error_text));
				}
			}
		}

		// variable z
		const int startindex_z = 2 * data.vehicles() * data.nb_customers() * data.days() + data.nb_customers() * data.days();
		{
			obj[0] = 0;
			lb[0] = 0;
			type[0] = 'I';

			status = CPXnewcols(env, problem, 1, obj, lb, NULL, type, NULL);
			if (status != 0)
			{
				CPXgeterrorstring(env, status, error_text);
				throw std::runtime_error("Error in function IP_model_fleet_size::build_problem(). \nCouldn't add variable. \nReason: " + std::string(error_text));
			}

			// change variable name
			std::string varname = "z";
			status = CPXchgname(env, problem, 'c', 2 * data.vehicles() * data.nb_customers() * data.days() + data.nb_customers() * data.days(), varname.c_str());
			if (status != 0)
			{
				CPXgeterrorstring(env, status, error_text);
				throw std::runtime_error("Error in function IP_model_fleet_size::build_problem(). \nCouldn't change variable name. \nReason: " + std::string(error_text));
			}
		}


		// add constraints
		int nb_constraints = -1;

		// 1: x1_vmd <= L * y1_vmd     forall v,m,d
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

					status = CPXaddrows(env, problem, 0, 1, nonzeroes, rhs, sense, matbeg, matind, matval, NULL, NULL);
					if (status != 0)
					{
						CPXgeterrorstring(env, status, error_text);
						throw std::runtime_error("Error in function IP_model_fleet_size::build_problem(). \nCouldn't add constraint. \nReason: " + std::string(error_text));
					}

					// change name of constraint
					std::string conname = "c1_" + std::to_string(v + 1) + "_" + std::to_string(m + 1) + "_" + std::to_string(d + 1);
					status = CPXchgname(env, problem, 'r', nb_constraints, conname.c_str());
					if (status != 0)
					{
						CPXgeterrorstring(env, status, error_text);
						throw std::runtime_error("Error in function IP_model_fleet_size::build_problem(). \nCouldn't change constraint name. \nReason: " + std::string(error_text));
					}
				}
			}
		}

		// 2: sum(v,d) x1_vmd == Qm
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

			status = CPXaddrows(env, problem, 0, 1, nonzeroes, rhs, sense, matbeg, matind, matval, NULL, NULL);
			if (status != 0)
			{
				CPXgeterrorstring(env, status, error_text);
				throw std::runtime_error("Error in function IP_model_fleet_size::build_problem(). \nCouldn't add constraint. \nReason: " + std::string(error_text));
			}

			// change name of constraint
			std::string conname = "c2_" + std::to_string(m + 1);
			status = CPXchgname(env, problem, 'r', nb_constraints, conname.c_str());
			if (status != 0)
			{
				CPXgeterrorstring(env, status, error_text);
				throw std::runtime_error("Error in function IP_model_fleet_size::build_problem(). \nCouldn't change constraint name. \nReason: " + std::string(error_text));
			}
		}


		// write to file
		status = CPXwriteprob(env, problem, "IP_model_fleet_size.lp", NULL);
		if (status != 0)
		{
			CPXgeterrorstring(env, status, error_text);
			throw std::runtime_error("Error in function IP_model_fleet_size::build_problem(). \nCouldn't write problem to lp-file. \nReason: " + std::string(error_text));
		}
	}

	void IP_model_fleet_size::solve_problem()
	{

	}

	void IP_model_fleet_size::clear_cplex()
	{

	}


	void IP_model_fleet_size::run()
	{

	}
}