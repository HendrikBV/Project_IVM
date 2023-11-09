#pragma once
#ifndef MODELS_H
#define MODELS_H

#include "ilcplex/cplex.h"

namespace IVM
{
	// forward declaration
	class Data;

	class IP_model_fleet_size
	{
		CPXENVptr env = nullptr;
		CPXLPptr problem = nullptr;
		
		void initialize_cplex();
		void build_problem(const Data& data);
		void solve_problem();
		void clear_cplex();

	public:
		void run();
	};

	class IP_model_monolithic
	{

	};

	class IP_column_generation
	{

	};

	class IP_two_trip_heuristic
	{

	};
}

#endif // !MODELS_H
