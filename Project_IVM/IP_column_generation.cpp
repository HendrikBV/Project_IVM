#include "models.h"
#include "data.h"
#include <stdexcept>
#include <memory>
#include <iostream>
#include <chrono>


namespace IVM
{
	///////////////////////////////////////////
	///			IP Column Generation		///
	///////////////////////////////////////////

	void IP_column_generation::initialize_cplex()
	{
		int status = 0;
		char error_text[CPXMESSAGEBUFSIZE];

		// open the CPLEX environment
		env = CPXopenCPLEX(&status);
		if (status != 0)
		{
			CPXgeterrorstring(env, status, error_text);
			throw std::runtime_error("Error in function IP_column_generation::initialize_cplex(). \nCouldn't open CPLEX. \nReason: " + std::string(error_text));
		}

		// turn output to screen on/off
		status = CPXsetintparam(env, CPX_PARAM_SCRIND, CPX_OFF);
		if (status != 0)
		{
			CPXgeterrorstring(env, status, error_text);
			throw std::runtime_error("Error in function IP_column_generation::initialize_cplex(). \nCouldn't change param SCRIND. \nReason: " + std::string(error_text));
		}
	}

	void IP_column_generation::clear_cplex()
	{
		int status = 0;
		char error_text[CPXMESSAGEBUFSIZE];

		// Free the master problem
		status = CPXfreeprob(env, &masterproblem);
		if (status != 0)
		{
			CPXgeterrorstring(env, status, error_text);
			throw std::runtime_error("Error in function IP_column_generation::clear_cplex(). \nCouldn't free master problem. \nReason: " + std::string(error_text));
		}

		// Free the pricing problem
		status = CPXfreeprob(env, &pricingproblem);
		if (status != 0)
		{
			CPXgeterrorstring(env, status, error_text);
			throw std::runtime_error("Error in function IP_column_generation::clear_cplex(). \nCouldn't free pricing problem. \nReason: " + std::string(error_text));
		}

		// Close the cplex environment
		status = CPXcloseCPLEX(&env);
		if (status != 0)
		{
			CPXgeterrorstring(env, status, error_text);
			throw std::runtime_error("Error in function IP_column_generation::clear_cplex(). \nCouldn't close cplex environment. \nReason: " + std::string(error_text));
		}
	}

	void IP_column_generation::build_masterproblem(const Data& data)
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

		const double big_M = 5;

		// allocate memory
		const size_t maxnonzeroes = 100000;
		matind = std::make_unique<int[]>(maxnonzeroes);
		matval = std::make_unique<double[]>(maxnonzeroes);


		// create the problem
		masterproblem = CPXcreateprob(env, &status, "IVM_master_problem");

		// problem is minimization
		status = CPXchgobjsen(env, masterproblem, CPX_MIN);

		// add variables
		// variable z
		const int startindex_z = 0;
		{
			obj[0] = data.cost_vehicle();
			lb[0] = 0;

			status = CPXnewcols(env, masterproblem, 1, obj, lb, NULL, NULL, NULL);
			if (status != 0)
			{
				CPXgeterrorstring(env, status, error_text);
				throw std::runtime_error("Error in function IP_column_generation::build_masterproblem(). \nCouldn't add variable. \nReason: " + std::string(error_text));
			}

			// change variable name
			std::string varname = "z";
			status = CPXchgname(env, masterproblem, 'c', startindex_z, varname.c_str());
			if (status != 0)
			{
				CPXgeterrorstring(env, status, error_text);
				throw std::runtime_error("Error in function IP_column_generation::build_masterproblem(). \nCouldn't change variable name. \nReason: " + std::string(error_text));
			}
		}

		// variable w_md
		const int startindex_w_md = startindex_z + 1;
		for (int m = 0; m < data.nb_customers(); ++m)
		{
			for (int d = 0; d < data.days(); ++d)
			{
				obj[0] = 0;
				lb[0] = 0;
				ub[0] = 1;

				status = CPXnewcols(env, masterproblem, 1, obj, lb, ub, NULL, NULL);
				if (status != 0)
				{
					CPXgeterrorstring(env, status, error_text);
					throw std::runtime_error("Error in function IP_column_generation::build_masterproblem(). \nCouldn't add variable. \nReason: " + std::string(error_text));
				}

				// change variable name
				std::string varname = "w_" + std::to_string(m + 1) + "_" + std::to_string(d + 1);
				status = CPXchgname(env, masterproblem, 'c', startindex_w_md + m * data.days() + d, varname.c_str());
				if (status != 0)
				{
					CPXgeterrorstring(env, status, error_text);
					throw std::runtime_error("Error in function IP_column_generation::build_masterproblem(). \nCouldn't change variable name. \nReason: " + std::string(error_text));
				}
			}
		}

		// variable supercolumn
		const int startindex_supercolumn = startindex_w_md + data.nb_customers() * data.days();
		{
			obj[0] = std::max(data.cost_hour(), data.cost_vehicle()) * 100; // large value
			lb[0] = 0;
			ub[0] = 1;

			status = CPXnewcols(env, masterproblem, 1, obj, lb, ub, NULL, NULL);
			if (status != 0)
			{
				CPXgeterrorstring(env, status, error_text);
				throw std::runtime_error("Error in function IP_column_generation::build_masterproblem(). \nCouldn't add variable. \nReason: " + std::string(error_text));
			}

			// change variable name
			std::string varname = "r_super";
			status = CPXchgname(env, masterproblem, 'c', startindex_supercolumn, varname.c_str());
			if (status != 0)
			{
				CPXgeterrorstring(env, status, error_text);
				throw std::runtime_error("Error in function IP_column_generation::build_masterproblem(). \nCouldn't change variable name. \nReason: " + std::string(error_text));
			}
		}


		// add constraints
		int nb_constraints = -1;

		// 1: sum(v,k) a_km * r_vk >= Q_m   forall m
		for (int m = 0; m < data.nb_customers(); ++m)
		{
			++nb_constraints;

			rhs[0] = data.demand(m);
			sense[0] = 'G';
			matbeg[0] = 0;

			nonzeroes = 0;

			// r_vk (supercolumn)
			matind[nonzeroes] = startindex_supercolumn;
			matval[nonzeroes] = data.demand(m) + 1; // a_km: assume sufficient to meet demand
			++nonzeroes;

			if (nonzeroes >= maxnonzeroes)
				throw std::runtime_error("Error in function IP_column_generation::build_masterproblem(). Nonzeroes exceeds size of maxnonzeroes (matind and matval)");

			status = CPXaddrows(env, masterproblem, 0, 1, nonzeroes, rhs, sense, matbeg, matind.get(), matval.get(), NULL, NULL);
			if (status != 0)
			{
				CPXgeterrorstring(env, status, error_text);
				throw std::runtime_error("Error in function IP_column_generation::build_masterproblem(). \nCouldn't add constraint. \nReason: " + std::string(error_text));
			}

			// change name of constraint
			std::string conname = "c1_" + std::to_string(m + 1);
			status = CPXchgname(env, masterproblem, 'r', nb_constraints, conname.c_str());
			if (status != 0)
			{
				CPXgeterrorstring(env, status, error_text);
				throw std::runtime_error("Error in function IP_column_generation::build_masterproblem(). \nCouldn't change constraint name. \nReason: " + std::string(error_text));
			}
		}

		// 2: sum(v,k) g_kmd * r_vk <= N * w_md    forall m,d
		for (int m = 0; m < data.nb_customers(); ++m)
		{
			for (int d = 0; d < data.days(); ++d)
			{
				++nb_constraints;

				rhs[0] = 0;
				sense[0] = 'L';
				matbeg[0] = 0;

				nonzeroes = 0;

				// r_vk (supercolumn)
				matind[nonzeroes] = startindex_supercolumn;
				matval[nonzeroes] = 1; // g_kmd = 1 (assume customer visited)
				++nonzeroes;

				// w_md
				matind[nonzeroes] = startindex_w_md + m * data.days() + d;
				matval[nonzeroes] = -big_M;
				++nonzeroes;

				if (nonzeroes >= maxnonzeroes)
					throw std::runtime_error("Error in function IP_column_generation::build_masterproblem(). Nonzeroes exceeds size of maxnonzeroes (matind and matval)");

				status = CPXaddrows(env, masterproblem, 0, 1, nonzeroes, rhs, sense, matbeg, matind.get(), matval.get(), NULL, NULL);
				if (status != 0)
				{
					CPXgeterrorstring(env, status, error_text);
					throw std::runtime_error("Error in function IP_column_generation::build_masterproblem(). \nCouldn't add constraint. \nReason: " + std::string(error_text));
				}

				// change name of constraint
				std::string conname = "c2_" + std::to_string(m + 1) + "_" + std::to_string(d + 1);
				status = CPXchgname(env, masterproblem, 'r', nb_constraints, conname.c_str());
				if (status != 0)
				{
					CPXgeterrorstring(env, status, error_text);
					throw std::runtime_error("Error in function IP_column_generation::build_masterproblem(). \nCouldn't change constraint name. \nReason: " + std::string(error_text));
				}
			}
		}

		// 3: sum(d) w_md <= W_m   forall m
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
				throw std::runtime_error("Error in function IP_column_generation::build_masterproblem(). Nonzeroes exceeds size of maxnonzeroes (matind and matval)");

			status = CPXaddrows(env, masterproblem, 0, 1, nonzeroes, rhs, sense, matbeg, matind.get(), matval.get(), NULL, NULL);
			if (status != 0)
			{
				CPXgeterrorstring(env, status, error_text);
				throw std::runtime_error("Error in function IP_column_generation::build_masterproblem(). \nCouldn't add constraint. \nReason: " + std::string(error_text));
			}

			// change name of constraint
			std::string conname = "c3_" + std::to_string(m + 1);
			status = CPXchgname(env, masterproblem, 'r', nb_constraints, conname.c_str());
			if (status != 0)
			{
				CPXgeterrorstring(env, status, error_text);
				throw std::runtime_error("Error in function IP_column_generation::build_masterproblem(). \nCouldn't change constraint name. \nReason: " + std::string(error_text));
			}
		}

		// 4: sum(k) h_kd * r_vk <= 1   forall v,d
		for (int v = 0; v < data.vehicles(); ++v)
		{
			for (int d = 0; d < data.days(); ++d)
			{
				++nb_constraints;

				rhs[0] = 1;
				sense[0] = 'L';
				matbeg[0] = 0;

				nonzeroes = 0;

				// r_vk (supercolumn)
				if (d == 0)
				{
					matind[nonzeroes] = startindex_supercolumn;
					matval[nonzeroes] = 1; // h_kd: is this route a day d route?
					++nonzeroes;
				}
				else
				{
					matind[nonzeroes] = startindex_supercolumn;
					matval[nonzeroes] = 0; // h_kd: is this route a day d route?
					++nonzeroes;
				}

				if (nonzeroes >= maxnonzeroes)
					throw std::runtime_error("Error in function IP_column_generation::build_masterproblem(). Nonzeroes exceeds size of maxnonzeroes (matind and matval)");

				status = CPXaddrows(env, masterproblem, 0, 1, nonzeroes, rhs, sense, matbeg, matind.get(), matval.get(), NULL, NULL);
				if (status != 0)
				{
					CPXgeterrorstring(env, status, error_text);
					throw std::runtime_error("Error in function IP_column_generation::build_masterproblem(). \nCouldn't add constraint. \nReason: " + std::string(error_text));
				}

				// change name of constraint
				std::string conname = "c4_" + std::to_string(v + 1) + "_" + std::to_string(d + 1);
				status = CPXchgname(env, masterproblem, 'r', nb_constraints, conname.c_str());
				if (status != 0)
				{
					CPXgeterrorstring(env, status, error_text);
					throw std::runtime_error("Error in function IP_column_generation::build_masterproblem(). \nCouldn't change constraint name. \nReason: " + std::string(error_text));
				}
			}
		}

		// 5: sum(v,k) h_kd * r_vk <= z   forall d
		for (int d = 0; d < data.days(); ++d)
		{
			++nb_constraints;

			rhs[0] = 0;
			sense[0] = 'L';
			matbeg[0] = 0;

			nonzeroes = 0;

			// r_vk (supercolumn)
			if (d == 0)
			{
				matind[nonzeroes] = startindex_supercolumn;
				matval[nonzeroes] = 1; // h_kd: is this route a day d route?
				++nonzeroes;
			}
			else
			{
				matind[nonzeroes] = startindex_supercolumn;
				matval[nonzeroes] = 0; // h_kd: is this route a day d route?
				++nonzeroes;
			}

			// z
			matind[nonzeroes] = startindex_z;
			matval[nonzeroes] = -1; // h_kd: is this route a day d route?
			++nonzeroes;

			if (nonzeroes >= maxnonzeroes)
				throw std::runtime_error("Error in function IP_column_generation::build_masterproblem(). Nonzeroes exceeds size of maxnonzeroes (matind and matval)");

			status = CPXaddrows(env, masterproblem, 0, 1, nonzeroes, rhs, sense, matbeg, matind.get(), matval.get(), NULL, NULL);
			if (status != 0)
			{
				CPXgeterrorstring(env, status, error_text);
				throw std::runtime_error("Error in function IP_column_generation::build_masterproblem(). \nCouldn't add constraint. \nReason: " + std::string(error_text));
			}

			// change name of constraint
			std::string conname = "c5_" + std::to_string(d + 1);
			status = CPXchgname(env, masterproblem, 'r', nb_constraints, conname.c_str());
			if (status != 0)
			{
				CPXgeterrorstring(env, status, error_text);
				throw std::runtime_error("Error in function IP_column_generation::build_masterproblem(). \nCouldn't change constraint name. \nReason: " + std::string(error_text));
			}
		}

		// write to file
		status = CPXwriteprob(env, masterproblem, "IVM_masterproblem.lp", NULL);
		if (status != 0)
		{
			CPXgeterrorstring(env, status, error_text);
			throw std::runtime_error("Error in function IP_column_generation::build_masterproblem(). \nCouldn't write problem to lp-file. \nReason: " + std::string(error_text));
		}


		// allocate memory for dual prices
		_dual_prices = std::make_unique<double[]>(CPXgetnumrows(env, masterproblem));
	}

	double IP_column_generation::solve_masterproblem()
	{
		int status = 0;
		int solstat = 0;
		double objval;

		// Optimize the problem
		status = CPXlpopt(env, masterproblem);
		if (status != 0)
		{
			char error_text[CPXMESSAGEBUFSIZE];
			CPXgeterrorstring(env, status, error_text);
			throw std::runtime_error("Error in function IP_column_generation::solve_masterproblem(). \nCPXlpopt failed. \nReason: " + std::string(error_text));
		}

		// Get the solution
		status = CPXsolution(env, masterproblem, &solstat, &objval, NULL, _dual_prices.get(), NULL, NULL);
		if (status != 0)
		{
			char error_text[CPXMESSAGEBUFSIZE];
			CPXgeterrorstring(env, status, error_text);
			throw std::runtime_error("Error in function IP_column_generation::solve_masterproblem(). \nCPXsolution failed. \nReason: " + std::string(error_text));
		}

		if (solstat == CPX_STAT_OPTIMAL)
			return objval;

		else if (solstat == CPX_STAT_INFEASIBLE)
			throw std::runtime_error("Error in function IP_column_generation::solve_masterproblem(). \nProblem is infeasible");
		else if (solstat == CPX_STAT_UNBOUNDED)
			throw std::runtime_error("Error in function IP_column_generation::solve_masterproblem(). \nProblem is unbounded");
		else if (solstat == CPX_STAT_INForUNBD) {
			throw std::runtime_error("Error in function IP_column_generation::solve_masterproblem(). \nProblem is infeasible or unbounded");

		}
	}

	void IP_column_generation::build_pricingproblem(const Data& data)
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

		const double big_M = 5;

		// allocate memory
		const size_t maxnonzeroes = 100000;
		matind = std::make_unique<int[]>(maxnonzeroes);
		matval = std::make_unique<double[]>(maxnonzeroes);


		// create the problem
		pricingproblem = CPXcreateprob(env, &status, "IVM_pricing_problem");

		// problem is minimization
		status = CPXchgobjsen(env, pricingproblem, CPX_MIN);

		// add variables
		// variable y1_m
		const int startindex_y1_m = 0;
		for(int m = 0; m < data.nb_customers(); ++m)
		{
			obj[0] = data.cost_hour() * data.t_trip1(m);
			lb[0] = 0;
			ub[0] = 1;
			type[0] = 'B';

			status = CPXnewcols(env, pricingproblem, 1, obj, lb, ub, type, NULL);
			if (status != 0)
			{
				CPXgeterrorstring(env, status, error_text);
				throw std::runtime_error("Error in function IP_column_generation::build_pricingproblem(). \nCouldn't add variable. \nReason: " + std::string(error_text));
			}

			// change variable name
			std::string varname = "y1_" + std::to_string(m + 1);
			status = CPXchgname(env, pricingproblem, 'c', startindex_y1_m + m, varname.c_str());
			if (status != 0)
			{
				CPXgeterrorstring(env, status, error_text);
				throw std::runtime_error("Error in function IP_column_generation::build_pricingproblem(). \nCouldn't change variable name. \nReason: " + std::string(error_text));
			}
		}

		// variable y2_m
		const int startindex_y2_m = startindex_y1_m + data.nb_customers();
		for (int m = 0; m < data.nb_customers(); ++m)
		{
			obj[0] = data.cost_hour() * data.t_trip2(m);
			lb[0] = 0;
			type[0] = 'I';

			status = CPXnewcols(env, pricingproblem, 1, obj, lb, NULL, type, NULL);
			if (status != 0)
			{
				CPXgeterrorstring(env, status, error_text);
				throw std::runtime_error("Error in function IP_column_generation::build_pricingproblem(). \nCouldn't add variable. \nReason: " + std::string(error_text));
			}

			// change variable name
			std::string varname = "y2_" + std::to_string(m + 1);
			status = CPXchgname(env, pricingproblem, 'c', startindex_y2_m + m, varname.c_str());
			if (status != 0)
			{
				CPXgeterrorstring(env, status, error_text);
				throw std::runtime_error("Error in function IP_column_generation::build_pricingproblem(). \nCouldn't change variable name. \nReason: " + std::string(error_text));
			}
		}

		// variable q2_m
		const int startindex_q2_m = startindex_y2_m + data.nb_customers();
		for (int m = 0; m < data.nb_customers(); ++m)
		{
			obj[0] = 0;
			lb[0] = 0;
			ub[0] = 1;
			type[0] = 'B';

			status = CPXnewcols(env, pricingproblem, 1, obj, lb, ub, type, NULL);
			if (status != 0)
			{
				CPXgeterrorstring(env, status, error_text);
				throw std::runtime_error("Error in function IP_column_generation::build_pricingproblem(). \nCouldn't add variable. \nReason: " + std::string(error_text));
			}

			// change variable name
			std::string varname = "q2_" + std::to_string(m + 1);
			status = CPXchgname(env, pricingproblem, 'c', startindex_q2_m + m, varname.c_str());
			if (status != 0)
			{
				CPXgeterrorstring(env, status, error_text);
				throw std::runtime_error("Error in function IP_column_generation::build_pricingproblem(). \nCouldn't change variable name. \nReason: " + std::string(error_text));
			}
		}

		// variable x1_m
		const int startindex_x1_m = startindex_q2_m + data.nb_customers();
		for (int m = 0; m < data.nb_customers(); ++m)
		{
			obj[0] = 0;
			lb[0] = 0;

			status = CPXnewcols(env, pricingproblem, 1, obj, lb, NULL, NULL, NULL);
			if (status != 0)
			{
				CPXgeterrorstring(env, status, error_text);
				throw std::runtime_error("Error in function IP_column_generation::build_pricingproblem(). \nCouldn't add variable. \nReason: " + std::string(error_text));
			}

			// change variable name
			std::string varname = "x1_" + std::to_string(m + 1);
			status = CPXchgname(env, pricingproblem, 'c', startindex_x1_m + m, varname.c_str());
			if (status != 0)
			{
				CPXgeterrorstring(env, status, error_text);
				throw std::runtime_error("Error in function IP_column_generation::build_pricingproblem(). \nCouldn't change variable name. \nReason: " + std::string(error_text));
			}
		}

		// variable x2_m
		const int startindex_x2_m = startindex_x1_m + data.nb_customers();
		for (int m = 0; m < data.nb_customers(); ++m)
		{
			obj[0] = 0;
			lb[0] = 0;

			status = CPXnewcols(env, pricingproblem, 1, obj, lb, NULL, NULL, NULL);
			if (status != 0)
			{
				CPXgeterrorstring(env, status, error_text);
				throw std::runtime_error("Error in function IP_column_generation::build_pricingproblem(). \nCouldn't add variable. \nReason: " + std::string(error_text));
			}

			// change variable name
			std::string varname = "x2_" + std::to_string(m + 1);
			status = CPXchgname(env, pricingproblem, 'c', startindex_x2_m + m, varname.c_str());
			if (status != 0)
			{
				CPXgeterrorstring(env, status, error_text);
				throw std::runtime_error("Error in function IP_column_generation::build_pricingproblem(). \nCouldn't change variable name. \nReason: " + std::string(error_text));
			}
		}

		// variable g_md
		const int startindex_g_md = startindex_x2_m + data.nb_customers();
		for (int m = 0; m < data.nb_customers(); ++m)
		{
			for (int d = 0; d < data.days(); ++d)
			{
				obj[0] = 0;
				lb[0] = 0;
				ub[0] = 1;
				type[0] = 'B';

				status = CPXnewcols(env, pricingproblem, 1, obj, lb, ub, type, NULL);
				if (status != 0)
				{
					CPXgeterrorstring(env, status, error_text);
					throw std::runtime_error("Error in function IP_column_generation::build_pricingproblem(). \nCouldn't add variable. \nReason: " + std::string(error_text));
				}

				// change variable name
				std::string varname = "q_" + std::to_string(m + 1) + "_" + std::to_string(d + 1);
				status = CPXchgname(env, pricingproblem, 'c', startindex_g_md + m * data.days() + d, varname.c_str());
				if (status != 0)
				{
					CPXgeterrorstring(env, status, error_text);
					throw std::runtime_error("Error in function IP_column_generation::build_pricingproblem(). \nCouldn't change variable name. \nReason: " + std::string(error_text));
				}
			}
		}

		// variable h_d
		const int startindex_h_d = startindex_g_md + data.nb_customers() * data.days();
		for (int d = 0; d < data.days(); ++d)
		{
			obj[0] = 0;
			lb[0] = 0;
			ub[0] = 1;
			type[0] = 'B';

			status = CPXnewcols(env, pricingproblem, 1, obj, lb, ub, type, NULL);
			if (status != 0)
			{
				CPXgeterrorstring(env, status, error_text);
				throw std::runtime_error("Error in function IP_column_generation::build_pricingproblem(). \nCouldn't add variable. \nReason: " + std::string(error_text));
			}

			// change variable name
			std::string varname = "h_" + std::to_string(d + 1);
			status = CPXchgname(env, pricingproblem, 'c', startindex_h_d + d, varname.c_str());
			if (status != 0)
			{
				CPXgeterrorstring(env, status, error_text);
				throw std::runtime_error("Error in function IP_column_generation::build_pricingproblem(). \nCouldn't change variable name. \nReason: " + std::string(error_text));
			}
		}

		// add constraints
		int nb_constraints = -1;

		// 1: x1_m <= L * y1_m    forall m
		for (int m = 0; m < data.nb_customers(); ++m)
		{
			++nb_constraints;

			rhs[0] = 0;
			sense[0] = 'L';
			matbeg[0] = 0;

			nonzeroes = 0;

			// x1_m
			matind[nonzeroes] = startindex_x1_m + m;
			matval[nonzeroes] = 1;
			++nonzeroes;

			// y1_m
			matind[nonzeroes] = startindex_y1_m + m;
			matval[nonzeroes] = -data.max_load();
			++nonzeroes;

			if (nonzeroes >= maxnonzeroes)
				throw std::runtime_error("Error in function IP_column_generation::build_pricingproblem(). Nonzeroes exceeds size of maxnonzeroes (matind and matval)");

			status = CPXaddrows(env, pricingproblem, 0, 1, nonzeroes, rhs, sense, matbeg, matind.get(), matval.get(), NULL, NULL);
			if (status != 0)
			{
				CPXgeterrorstring(env, status, error_text);
				throw std::runtime_error("Error in function IP_column_generation::build_pricingproblem(). \nCouldn't add constraint. \nReason: " + std::string(error_text));
			}

			// change name of constraint
			std::string conname = "c1_" + std::to_string(m + 1);
			status = CPXchgname(env, pricingproblem, 'r', nb_constraints, conname.c_str());
			if (status != 0)
			{
				CPXgeterrorstring(env, status, error_text);
				throw std::runtime_error("Error in function IP_column_generation::build_pricingproblem(). \nCouldn't change constraint name. \nReason: " + std::string(error_text));
			}
		}

		// 2: x2_m <= L * y2_m    forall m
		for (int m = 0; m < data.nb_customers(); ++m)
		{
			++nb_constraints;

			rhs[0] = 0;
			sense[0] = 'L';
			matbeg[0] = 0;

			nonzeroes = 0;

			// x2_m
			matind[nonzeroes] = startindex_x2_m + m;
			matval[nonzeroes] = 1;
			++nonzeroes;

			// y2_m
			matind[nonzeroes] = startindex_y2_m + m;
			matval[nonzeroes] = -data.max_load();
			++nonzeroes;

			if (nonzeroes >= maxnonzeroes)
				throw std::runtime_error("Error in function IP_column_generation::build_pricingproblem(). Nonzeroes exceeds size of maxnonzeroes (matind and matval)");

			status = CPXaddrows(env, pricingproblem, 0, 1, nonzeroes, rhs, sense, matbeg, matind.get(), matval.get(), NULL, NULL);
			if (status != 0)
			{
				CPXgeterrorstring(env, status, error_text);
				throw std::runtime_error("Error in function IP_column_generation::build_pricingproblem(). \nCouldn't add constraint. \nReason: " + std::string(error_text));
			}

			// change name of constraint
			std::string conname = "c2_" + std::to_string(m + 1);
			status = CPXchgname(env, pricingproblem, 'r', nb_constraints, conname.c_str());
			if (status != 0)
			{
				CPXgeterrorstring(env, status, error_text);
				throw std::runtime_error("Error in function IP_column_generation::build_pricingproblem(). \nCouldn't change constraint name. \nReason: " + std::string(error_text));
			}
		}

		// 3: x1_m + x2_m <= Q_m   forall m
		for (int m = 0; m < data.nb_customers(); ++m)
		{
			++nb_constraints;

			rhs[0] = data.demand(m);
			sense[0] = 'L';
			matbeg[0] = 0;

			nonzeroes = 0;

			// x1_m
			matind[nonzeroes] = startindex_x1_m + m;
			matval[nonzeroes] = 1;
			++nonzeroes;

			// x2_m
			matind[nonzeroes] = startindex_x2_m + m;
			matval[nonzeroes] = -data.max_load();
			++nonzeroes;

			if (nonzeroes >= maxnonzeroes)
				throw std::runtime_error("Error in function IP_column_generation::build_pricingproblem(). Nonzeroes exceeds size of maxnonzeroes (matind and matval)");

			status = CPXaddrows(env, pricingproblem, 0, 1, nonzeroes, rhs, sense, matbeg, matind.get(), matval.get(), NULL, NULL);
			if (status != 0)
			{
				CPXgeterrorstring(env, status, error_text);
				throw std::runtime_error("Error in function IP_column_generation::build_pricingproblem(). \nCouldn't add constraint. \nReason: " + std::string(error_text));
			}

			// change name of constraint
			std::string conname = "c3_" + std::to_string(m + 1);
			status = CPXchgname(env, pricingproblem, 'r', nb_constraints, conname.c_str());
			if (status != 0)
			{
				CPXgeterrorstring(env, status, error_text);
				throw std::runtime_error("Error in function IP_column_generation::build_pricingproblem(). \nCouldn't change constraint name. \nReason: " + std::string(error_text));
			}
		}

		// 4: sum(m) s_m * (x1_m + x2_m) + t1_m * y1_m + t2_m * y2_m <= T
		{
			++nb_constraints;

			rhs[0] = data.max_hours();
			sense[0] = 'L';
			matbeg[0] = 0;

			nonzeroes = 0;

			// variables
			for (int m = 0; m < data.nb_customers(); ++m)
			{
				// x1_m
				matind[nonzeroes] = startindex_x1_m + m;
				matval[nonzeroes] = data.collection_speed(m);
				++nonzeroes;

				// x2_m
				matind[nonzeroes] = startindex_x2_m + m;
				matval[nonzeroes] = data.collection_speed(m);
				++nonzeroes;

				// y1_m
				matind[nonzeroes] = startindex_y1_m + m;
				matval[nonzeroes] = data.t_trip1(m);
				++nonzeroes;

				// y2_m
				matind[nonzeroes] = startindex_y2_m + m;
				matval[nonzeroes] = data.t_trip2(m);
				++nonzeroes;
			}

			if (nonzeroes >= maxnonzeroes)
				throw std::runtime_error("Error in function IP_column_generation::build_pricingproblem(). Nonzeroes exceeds size of maxnonzeroes (matind and matval)");

			status = CPXaddrows(env, pricingproblem, 0, 1, nonzeroes, rhs, sense, matbeg, matind.get(), matval.get(), NULL, NULL);
			if (status != 0)
			{
				CPXgeterrorstring(env, status, error_text);
				throw std::runtime_error("Error in function IP_column_generation::build_pricingproblem(). \nCouldn't add constraint. \nReason: " + std::string(error_text));
			}

			// change name of constraint
			std::string conname = "c4";
			status = CPXchgname(env, pricingproblem, 'r', nb_constraints, conname.c_str());
			if (status != 0)
			{
				CPXgeterrorstring(env, status, error_text);
				throw std::runtime_error("Error in function IP_column_generation::build_pricingproblem(). \nCouldn't change constraint name. \nReason: " + std::string(error_text));
			}
		}

		// 5: sum(m) y1_m = 1
		{
			++nb_constraints;

			rhs[0] = 1;
			sense[0] = 'E';
			matbeg[0] = 0;

			nonzeroes = 0;

			// x1_m
			for (int m = 0; m < data.nb_customers(); ++m)
			{
				matind[nonzeroes] = startindex_y1_m + m;
				matval[nonzeroes] = 1;
				++nonzeroes;
			}

			if (nonzeroes >= maxnonzeroes)
				throw std::runtime_error("Error in function IP_column_generation::build_pricingproblem(). Nonzeroes exceeds size of maxnonzeroes (matind and matval)");

			status = CPXaddrows(env, pricingproblem, 0, 1, nonzeroes, rhs, sense, matbeg, matind.get(), matval.get(), NULL, NULL);
			if (status != 0)
			{
				CPXgeterrorstring(env, status, error_text);
				throw std::runtime_error("Error in function IP_column_generation::build_pricingproblem(). \nCouldn't add constraint. \nReason: " + std::string(error_text));
			}

			// change name of constraint
			std::string conname = "c5";
			status = CPXchgname(env, pricingproblem, 'r', nb_constraints, conname.c_str());
			if (status != 0)
			{
				CPXgeterrorstring(env, status, error_text);
				throw std::runtime_error("Error in function IP_column_generation::build_pricingproblem(). \nCouldn't change constraint name. \nReason: " + std::string(error_text));
			}
		}

		// 6: sum(d) h_d = 1
		{
			++nb_constraints;

			rhs[0] = 1;
			sense[0] = 'E';
			matbeg[0] = 0;

			nonzeroes = 0;

			// h_d
			for (int d = 0; d < data.days(); ++d)
			{
				matind[nonzeroes] = startindex_h_d + d;
				matval[nonzeroes] = 1;
				++nonzeroes;
			}

			if (nonzeroes >= maxnonzeroes)
				throw std::runtime_error("Error in function IP_column_generation::build_pricingproblem(). Nonzeroes exceeds size of maxnonzeroes (matind and matval)");

			status = CPXaddrows(env, pricingproblem, 0, 1, nonzeroes, rhs, sense, matbeg, matind.get(), matval.get(), NULL, NULL);
			if (status != 0)
			{
				CPXgeterrorstring(env, status, error_text);
				throw std::runtime_error("Error in function IP_column_generation::build_pricingproblem(). \nCouldn't add constraint. \nReason: " + std::string(error_text));
			}

			// change name of constraint
			std::string conname = "c6";
			status = CPXchgname(env, pricingproblem, 'r', nb_constraints, conname.c_str());
			if (status != 0)
			{
				CPXgeterrorstring(env, status, error_text);
				throw std::runtime_error("Error in function IP_column_generation::build_pricingproblem(). \nCouldn't change constraint name. \nReason: " + std::string(error_text));
			}
		}

		// 7: g_md + 1 >= y1_m + h_d   forall m,d
		for (int m = 0; m < data.nb_customers(); ++m)
		{
			for (int d = 0; d < data.days(); ++d)
			{
				++nb_constraints;

				rhs[0] = 1;
				sense[0] = 'L';
				matbeg[0] = 0;

				nonzeroes = 0;

				// y1_m
				matind[nonzeroes] = startindex_y1_m + m;
				matval[nonzeroes] = 1;
				++nonzeroes;

				// h_d
				matind[nonzeroes] = startindex_h_d + d;
				matval[nonzeroes] = 1;
				++nonzeroes;

				// g_md
				matind[nonzeroes] = startindex_g_md + m * data.days() + d;
				matval[nonzeroes] = -1;
				++nonzeroes;

				if (nonzeroes >= maxnonzeroes)
					throw std::runtime_error("Error in function IP_column_generation::build_pricingproblem(). Nonzeroes exceeds size of maxnonzeroes (matind and matval)");

				status = CPXaddrows(env, pricingproblem, 0, 1, nonzeroes, rhs, sense, matbeg, matind.get(), matval.get(), NULL, NULL);
				if (status != 0)
				{
					CPXgeterrorstring(env, status, error_text);
					throw std::runtime_error("Error in function IP_column_generation::build_pricingproblem(). \nCouldn't add constraint. \nReason: " + std::string(error_text));
				}

				// change name of constraint
				std::string conname = "c7_" + std::to_string(m + 1) + "_" + std::to_string(d + 1);
				status = CPXchgname(env, pricingproblem, 'r', nb_constraints, conname.c_str());
				if (status != 0)
				{
					CPXgeterrorstring(env, status, error_text);
					throw std::runtime_error("Error in function IP_column_generation::build_pricingproblem(). \nCouldn't change constraint name. \nReason: " + std::string(error_text));
				}
			}
		}

		// 8: N*g_md + 1 >= y2_m + h_d   forall m,d
		for (int m = 0; m < data.nb_customers(); ++m)
		{
			for (int d = 0; d < data.days(); ++d)
			{
				++nb_constraints;

				rhs[0] = 1;
				sense[0] = 'L';
				matbeg[0] = 0;

				nonzeroes = 0;

				// y2_m
				matind[nonzeroes] = startindex_y2_m + m;
				matval[nonzeroes] = 1;
				++nonzeroes;

				// h_d
				matind[nonzeroes] = startindex_h_d + d;
				matval[nonzeroes] = 1;
				++nonzeroes;

				// g_md
				matind[nonzeroes] = startindex_g_md + m * data.days() + d;
				matval[nonzeroes] = -big_M;
				++nonzeroes;

				if (nonzeroes >= maxnonzeroes)
					throw std::runtime_error("Error in function IP_column_generation::build_pricingproblem(). Nonzeroes exceeds size of maxnonzeroes (matind and matval)");

				status = CPXaddrows(env, pricingproblem, 0, 1, nonzeroes, rhs, sense, matbeg, matind.get(), matval.get(), NULL, NULL);
				if (status != 0)
				{
					CPXgeterrorstring(env, status, error_text);
					throw std::runtime_error("Error in function IP_column_generation::build_pricingproblem(). \nCouldn't add constraint. \nReason: " + std::string(error_text));
				}

				// change name of constraint
				std::string conname = "c8_" + std::to_string(m + 1) + "_" + std::to_string(d + 1);
				status = CPXchgname(env, pricingproblem, 'r', nb_constraints, conname.c_str());
				if (status != 0)
				{
					CPXgeterrorstring(env, status, error_text);
					throw std::runtime_error("Error in function IP_column_generation::build_pricingproblem(). \nCouldn't change constraint name. \nReason: " + std::string(error_text));
				}
			}
		}

		// 9: g_md - h_d <= 0   forall m,d
		for (int m = 0; m < data.nb_customers(); ++m)
		{
			for (int d = 0; d < data.days(); ++d)
			{
				++nb_constraints;

				rhs[0] = 0;
				sense[0] = 'L';
				matbeg[0] = 0;

				nonzeroes = 0;

				// h_d
				matind[nonzeroes] = startindex_h_d + d;
				matval[nonzeroes] = -1;
				++nonzeroes;

				// g_md
				matind[nonzeroes] = startindex_g_md + m * data.days() + d;
				matval[nonzeroes] = 1;
				++nonzeroes;

				if (nonzeroes >= maxnonzeroes)
					throw std::runtime_error("Error in function IP_column_generation::build_pricingproblem(). Nonzeroes exceeds size of maxnonzeroes (matind and matval)");

				status = CPXaddrows(env, pricingproblem, 0, 1, nonzeroes, rhs, sense, matbeg, matind.get(), matval.get(), NULL, NULL);
				if (status != 0)
				{
					CPXgeterrorstring(env, status, error_text);
					throw std::runtime_error("Error in function IP_column_generation::build_pricingproblem(). \nCouldn't add constraint. \nReason: " + std::string(error_text));
				}

				// change name of constraint
				std::string conname = "c9_" + std::to_string(m + 1) + "_" + std::to_string(d + 1);
				status = CPXchgname(env, pricingproblem, 'r', nb_constraints, conname.c_str());
				if (status != 0)
				{
					CPXgeterrorstring(env, status, error_text);
					throw std::runtime_error("Error in function IP_column_generation::build_pricingproblem(). \nCouldn't change constraint name. \nReason: " + std::string(error_text));
				}
			}
		}

		// 10: g_md - y1_m - y2_m <= 0   forall m,d
		for (int m = 0; m < data.nb_customers(); ++m)
		{
			for (int d = 0; d < data.days(); ++d)
			{
				++nb_constraints;

				rhs[0] = 0;
				sense[0] = 'L';
				matbeg[0] = 0;

				nonzeroes = 0;

				// y1_m
				matind[nonzeroes] = startindex_y1_m + m;
				matval[nonzeroes] = -1;
				++nonzeroes;

				// y2_m
				matind[nonzeroes] = startindex_y2_m + m;
				matval[nonzeroes] = -1;
				++nonzeroes;

				// g_md
				matind[nonzeroes] = startindex_g_md + m * data.days() + d;
				matval[nonzeroes] = 1;
				++nonzeroes;

				if (nonzeroes >= maxnonzeroes)
					throw std::runtime_error("Error in function IP_column_generation::build_pricingproblem(). Nonzeroes exceeds size of maxnonzeroes (matind and matval)");

				status = CPXaddrows(env, pricingproblem, 0, 1, nonzeroes, rhs, sense, matbeg, matind.get(), matval.get(), NULL, NULL);
				if (status != 0)
				{
					CPXgeterrorstring(env, status, error_text);
					throw std::runtime_error("Error in function IP_column_generation::build_pricingproblem(). \nCouldn't add constraint. \nReason: " + std::string(error_text));
				}

				// change name of constraint
				std::string conname = "c10_" + std::to_string(m + 1) + "_" + std::to_string(d + 1);
				status = CPXchgname(env, pricingproblem, 'r', nb_constraints, conname.c_str());
				if (status != 0)
				{
					CPXgeterrorstring(env, status, error_text);
					throw std::runtime_error("Error in function IP_column_generation::build_pricingproblem(). \nCouldn't change constraint name. \nReason: " + std::string(error_text));
				}
			}
		}

		// 11: (tdep_m - tdisp_m) - N*(1-y1_m) - N*(1-q2_m') <= tdep_m' - tdisp_m'   forall m1,m2 (valid inequality)   
		for (int m1 = 0; m1 < data.nb_customers(); ++m1)
		{
			for (int m2 = 0; m2 < data.nb_customers(); ++m2)
			{
				++nb_constraints;

				rhs[0] = data.t_dep(m2) - data.t_disp(m2) - data.t_dep(m1) + data.t_disp(m2) + 2 * big_M;
				sense[0] = 'L';
				matbeg[0] = 0;

				nonzeroes = 0;

				// y1_m
				matind[nonzeroes] = startindex_y1_m + m1;
				matval[nonzeroes] = big_M;
				++nonzeroes;

				// q2_m
				matind[nonzeroes] = startindex_q2_m + m2;
				matval[nonzeroes] = big_M;
				++nonzeroes;


				if (nonzeroes >= maxnonzeroes)
					throw std::runtime_error("Error in function IP_column_generation::build_pricingproblem(). Nonzeroes exceeds size of maxnonzeroes (matind and matval)");

				status = CPXaddrows(env, pricingproblem, 0, 1, nonzeroes, rhs, sense, matbeg, matind.get(), matval.get(), NULL, NULL);
				if (status != 0)
				{
					CPXgeterrorstring(env, status, error_text);
					throw std::runtime_error("Error in function IP_column_generation::build_pricingproblem(). \nCouldn't add constraint. \nReason: " + std::string(error_text));
				}

				// change name of constraint
				std::string conname = "c11_" + std::to_string(m1 + 1) + "_" + std::to_string(m2 + 1);
				status = CPXchgname(env, pricingproblem, 'r', nb_constraints, conname.c_str());
				if (status != 0)
				{
					CPXgeterrorstring(env, status, error_text);
					throw std::runtime_error("Error in function IP_column_generation::build_pricingproblem(). \nCouldn't change constraint name. \nReason: " + std::string(error_text));
				}
			}
		}

		// 12: N * q2_m - y2_m >= 0   forall m
		for (int m = 0; m < data.nb_customers(); ++m)
		{
			++nb_constraints;

			rhs[0] = 0;
			sense[0] = 'G';
			matbeg[0] = 0;

			nonzeroes = 0;

			// y2_m
			matind[nonzeroes] = startindex_y2_m + m;
			matval[nonzeroes] = -1;
			++nonzeroes;

			// q2_m
			matind[nonzeroes] = startindex_q2_m + m;
			matval[nonzeroes] = big_M;
			++nonzeroes;

			if (nonzeroes >= maxnonzeroes)
				throw std::runtime_error("Error in function IP_column_generation::build_pricingproblem(). Nonzeroes exceeds size of maxnonzeroes (matind and matval)");

			status = CPXaddrows(env, pricingproblem, 0, 1, nonzeroes, rhs, sense, matbeg, matind.get(), matval.get(), NULL, NULL);
			if (status != 0)
			{
				CPXgeterrorstring(env, status, error_text);
				throw std::runtime_error("Error in function IP_column_generation::build_pricingproblem(). \nCouldn't add constraint. \nReason: " + std::string(error_text));
			}

			// change name of constraint
			std::string conname = "c12_" + std::to_string(m + 1);
			status = CPXchgname(env, pricingproblem, 'r', nb_constraints, conname.c_str());
			if (status != 0)
			{
				CPXgeterrorstring(env, status, error_text);
				throw std::runtime_error("Error in function IP_column_generation::build_pricingproblem(). \nCouldn't change constraint name. \nReason: " + std::string(error_text));
			}
		}

		// 12: q2_m - y2_m <= 0   forall m
		for (int m = 0; m < data.nb_customers(); ++m)
		{
			++nb_constraints;

			rhs[0] = 0;
			sense[0] = 'L';
			matbeg[0] = 0;

			nonzeroes = 0;

			// y2_m
			matind[nonzeroes] = startindex_y2_m + m;
			matval[nonzeroes] = -1;
			++nonzeroes;

			// q2_m
			matind[nonzeroes] = startindex_q2_m + m;
			matval[nonzeroes] = 1;
			++nonzeroes;

			if (nonzeroes >= maxnonzeroes)
				throw std::runtime_error("Error in function IP_column_generation::build_pricingproblem(). Nonzeroes exceeds size of maxnonzeroes (matind and matval)");

			status = CPXaddrows(env, pricingproblem, 0, 1, nonzeroes, rhs, sense, matbeg, matind.get(), matval.get(), NULL, NULL);
			if (status != 0)
			{
				CPXgeterrorstring(env, status, error_text);
				throw std::runtime_error("Error in function IP_column_generation::build_pricingproblem(). \nCouldn't add constraint. \nReason: " + std::string(error_text));
			}

			// change name of constraint
			std::string conname = "c13_" + std::to_string(m + 1);
			status = CPXchgname(env, pricingproblem, 'r', nb_constraints, conname.c_str());
			if (status != 0)
			{
				CPXgeterrorstring(env, status, error_text);
				throw std::runtime_error("Error in function IP_column_generation::build_pricingproblem(). \nCouldn't change constraint name. \nReason: " + std::string(error_text));
			}
		}

		// write to file
		status = CPXwriteprob(env, masterproblem, "IVM_pricingproblem.lp", NULL);
		if (status != 0)
		{
			CPXgeterrorstring(env, status, error_text);
			throw std::runtime_error("Error in function IP_column_generation::build_pricingproblem(). \nCouldn't write problem to lp-file. \nReason: " + std::string(error_text));
		}



		// Assign memory for solution
		_solution_pricingproblem = std::make_unique<double[]>(CPXgetnumcols(env, pricingproblem));
	}

	void IP_column_generation::change_coefficients_pricingproblem(const Data& data)
	{
		// indices rows master (dual prices)
		const int startindex_lambda_m = 0;
		const int startindex_gamma_md = startindex_lambda_m + data.nb_customers();
		const int startindex_tau_vd = startindex_gamma_md + data.nb_customers() * data.days() + data.nb_customers();
		const int startindex_eta_d = startindex_tau_vd + data.vehicles() * data.days();

		// indices variables pricing (objective coefficients)
		const int startindex_y1_m = 0;
		const int startindex_y2_m = data.nb_customers();
		const int startindex_q2_m = startindex_y2_m + data.nb_customers();
		const int startindex_x1_m = startindex_q2_m + data.nb_customers();
		const int startindex_x2_m = startindex_x1_m + data.nb_customers();
		const int startindex_g_md = startindex_x2_m + data.nb_customers();
		const int startindex_h_d = startindex_g_md + data.nb_customers() * data.days();

		int status = 0;
		const int row = -1; // objective coefficients

		// lambda_m
		for (int m = 0; m < data.nb_customers(); ++m)
		{
			const double newvalue = -_dual_prices[startindex_lambda_m + m];

			// x1
			const int column = startindex_x1_m + m;
			status = CPXchgcoef(env, pricingproblem, row, column, newvalue);
			if (status != 0)
			{
				char error_text[CPXMESSAGEBUFSIZE];
				CPXgeterrorstring(env, status, error_text);
				throw std::runtime_error("Error in function IP_column_generation::change_coefficients_pricing_problem(). \nCouldn't change objective function coefficient.\nReason: " + std::string(error_text));
			}

			// x2
			const int column = startindex_x1_m + m;
			status = CPXchgcoef(env, pricingproblem, row, column, newvalue);
			if (status != 0)
			{
				char error_text[CPXMESSAGEBUFSIZE];
				CPXgeterrorstring(env, status, error_text);
				throw std::runtime_error("Error in function IP_column_generation::change_coefficients_pricing_problem(). \nCouldn't change objective function coefficient.\nReason: " + std::string(error_text));
			}
		}

		// gamma_md
		for (int m = 0; m < data.nb_customers(); ++m)
		{
			for (int d = 0; d < data.days(); ++d)
			{
				const double newvalue = -_dual_prices[startindex_gamma_md + m * data.days() + d];

				// g_md
				const int column = startindex_g_md + m * data.days() + d;
				status = CPXchgcoef(env, pricingproblem, row, column, newvalue);
				if (status != 0)
				{
					char error_text[CPXMESSAGEBUFSIZE];
					CPXgeterrorstring(env, status, error_text);
					throw std::runtime_error("Error in function IP_column_generation::change_coefficients_pricing_problem(). \nCouldn't change objective function coefficient.\nReason: " + std::string(error_text));
				}
			}
		}

		// tau_vd & eta_d for h_d
		for (int v = 0; v < data.vehicles(); ++v)
		{
			for (int d = 0; d < data.days(); ++d)
			{
				const double newvalue = -_dual_prices[startindex_tau_vd + v*data.days() + d] - _dual_prices[startindex_eta_d + d];

				// h_d
				const int column = startindex_h_d + d;
				status = CPXchgcoef(env, pricingproblem, row, column, newvalue);
				if (status != 0)
				{
					char error_text[CPXMESSAGEBUFSIZE];
					CPXgeterrorstring(env, status, error_text);
					throw std::runtime_error("Error in function IP_column_generation::change_coefficients_pricing_problem(). \nCouldn't change objective function coefficient.\nReason: " + std::string(error_text));
				}
			}
		}


		// Write the problem to a file
		status = CPXwriteprob(env, pricingproblem, "IVM_pricingproblem.lp", NULL);
		if (status != 0)
		{
			char error_text[CPXMESSAGEBUFSIZE];
			CPXgeterrorstring(env, status, error_text);
			throw std::runtime_error("Error in function IP_column_generation::change_coefficients_pricing_problem(). \nFailed to write the problem to a file. \nReason: " + std::string(error_text));
		}
	}

	double IP_column_generation::solve_pricingproblem()
	{
		int status = 0;
		int solstat = 0;
		double objval;

		// Optimize the problem
		status = CPXmipopt(env, pricingproblem);
		if (status != 0)
		{
			char error_text[CPXMESSAGEBUFSIZE];
			CPXgeterrorstring(env, status, error_text);
			throw std::runtime_error("Error in function IP_column_generation::solve_pricingproblem(). \nCPXmipopt failed. \nReason: " + std::string(error_text));
		}

		// Get the solution
		status = CPXsolution(env, pricingproblem, &solstat, &objval, _solution_pricingproblem.get(), NULL, NULL, NULL);
		if (status != 0)
		{
			char error_text[CPXMESSAGEBUFSIZE];
			CPXgeterrorstring(env, status, error_text);
			throw std::runtime_error("Error in function IP_column_generation::solve_pricingproblem(). \nCPXsolution failed. \nReason: " + std::string(error_text));
		}

		if (solstat == CPXMIP_OPTIMAL || solstat == CPXMIP_OPTIMAL_TOL)
		{
			// return reduced cost (objval == reduced cost)
			return objval;
		}

		else
			throw std::runtime_error("Error in function IP_column_generation::solve_pricingproblem(). \nDidn't find optimal solution pricing problem. Solstat = " + std::to_string(solstat));
	}

	void IP_column_generation::add_column_to_masterproblem(const Data& data, int iteration)
	{
		int status = 0;
		const int maxnonzeroes = 1000;
		double obj[1];						// Objective function coefficient of variables
		double lb[1];						// Lower bounds of variables
		double ub[1];						// Upper bounds of variables
		int matbeg[1];						// Begin position of the constraint (always 0)
		int nonzeroes = 0;					// To calculate number of nonzero coefficients in each constraint
		int matind[maxnonzeroes];			// Position of each element in constraint matrix
		double matval[maxnonzeroes];		// Value of each element in constraint matrix


		// indices variables pricing (objective coefficients)
		const int startindex_y1_m = 0;
		const int startindex_y2_m = data.nb_customers();
		const int startindex_q2_m = startindex_y2_m + data.nb_customers();
		const int startindex_x1_m = startindex_q2_m + data.nb_customers();
		const int startindex_x2_m = startindex_x1_m + data.nb_customers();
		const int startindex_g_md = startindex_x2_m + data.nb_customers();
		const int startindex_h_d = startindex_g_md + data.nb_customers() * data.days();

		// indices rows master (new coefficients)
		const int startindex_a_km = 0;
		const int startindex_g_kmd = startindex_a_km + data.nb_customers();
		const int startindex_w_md = startindex_g_kmd + data.nb_customers() * data.days();
		const int startindex_h_kd = startindex_w_md + data.nb_customers();
		const int startindex_h_kd_bis = startindex_h_kd + data.vehicles() * data.days();

		// indexing
		matbeg[0] = 0;

		// bounds
		lb[0] = 0;
		ub[0] = 1;

		// calculate obj value
		obj[0] = 0;
		for (int m = 0; m < data.nb_customers(); ++m)
		{
			obj[0] += data.cost_hour() * data.t_trip1(m) * _solution_pricingproblem[startindex_y1_m + m];
			obj[0] += data.cost_hour() * data.t_trip2(m) * _solution_pricingproblem[startindex_y2_m + m];
		}

		// calculate coefficients in the constraints
		// a_km
		for (int m = 0; m < data.nb_customers(); ++m)
		{
			matind[nonzeroes] = startindex_a_km + m; // row
			matval[nonzeroes] = _solution_pricingproblem[startindex_x1_m + m] + _solution_pricingproblem[startindex_x2_m + m];
			++nonzeroes;
		}

		// g_kmd
		for (int m = 0; m < data.nb_customers(); ++m)
		{
			for (int d = 0; d < data.days(); ++d)
			{
				matind[nonzeroes] = startindex_g_kmd + m * data.days() + d; // row
				matval[nonzeroes] = _solution_pricingproblem[startindex_g_md + m * data.days() + d];
				++nonzeroes;
			}
		}

		// h_kd
		for (int d = 0; d < data.days(); ++d)
		{
			matind[nonzeroes] = startindex_h_kd + d; // row
			matval[nonzeroes] = _solution_pricingproblem[startindex_h_d + d];
			++nonzeroes;
		}

		if (nonzeroes >= maxnonzeroes)
			throw std::runtime_error("Error in function IP_column_generation::add_column_to_masterproblem(). Nonzeroes exceeds size of maxnonzeroes (matind and matval)");
	
		// add the variables (same column for every vehicle)
		for (int v = 0; v < data.vehicles(); ++v)
		{
			status = CPXaddcols(env, masterproblem, 1, nonzeroes, obj, matbeg, matind, matval, lb, ub, NULL);
			if (status != 0)
			{
				char error_text[CPXMESSAGEBUFSIZE];
				CPXgeterrorstring(env, status, error_text);
				throw std::runtime_error("Error in function IP_column_generation::add_column_to_masterproblem(). \nCouldn't add variable. \nReason: " + std::string(error_text));
			}

			std::string varname = "r_" + std::to_string(v+1) + "_" + std::to_string(iteration);
			status = CPXchgname(env, masterproblem, 'c', CPXgetnumcols(env, masterproblem) - 1, varname.c_str());
			if (status != 0)
			{
				char error_text[CPXMESSAGEBUFSIZE];
				CPXgeterrorstring(env, status, error_text);
				throw std::runtime_error("Error in function IP_column_generation::add_column_to_masterproblem(). \nCouldn't change variable name. \nReason: " + std::string(error_text));
			}
		}

		// Write the problem to a file
		status = CPXwriteprob(env, masterproblem, "IVM_problem.lp", NULL);
		if (status != 0)
		{
			char error_text[CPXMESSAGEBUFSIZE];
			CPXgeterrorstring(env, status, error_text);
			throw std::runtime_error("Error in function IP_column_generation::add_column_to_masterproblem(). \nFailed to write the problem to a file. \nReason: " + std::string(error_text));
		}
	}

	void IP_column_generation::run_column_generation(const Data& data)
	{
		initialize_cplex();
		build_masterproblem(data);
		build_pricingproblem(data);


		auto start_time = std::chrono::system_clock::now();
		size_t iteration = 0;

		while (true)
		{
			++iteration;

			double objval_master = solve_masterproblem();
			std::cout << "\n\n\nIteration " << iteration << "\nMaster obj : " << objval_master;

			change_coefficients_pricingproblem(data);

			double reduced_cost = solve_pricingproblem();
			std::cout << "\nReduced cost: " << reduced_cost;
			if (reduced_cost > -0.0001) // tolerance round-off errors
				break; // no column found, exit column generation loop

			add_column_to_masterproblem(data, iteration);
		}

		std::chrono::duration<double, std::ratio<1, 1>> elapsed_time = std::chrono::system_clock::now() - start_time;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////

	void IP_column_generation::run_CG_MIP_heuristic()
	{

	}

	///////////////////////////////////////////////////////////////////////////////////////////////

	void IP_column_generation::run_diving_heuristic()
	{

	}

	///////////////////////////////////////////////////////////////////////////////////////////////

	void IP_column_generation::run_branch_and_price()
	{

	}
}