/*
	Copyright (c) 2024 KU Leuven
	Code author: Hendrik Vermuyten
*/



#include "data.h"
#include "models.h"
#include "auxiliaries.h"
#include <iostream>
#include <fstream>
#include <array>
#include <exception>
#include <chrono>





std::ofstream logfile;

void solve_instance(const std::string& filename, int scenario, double maxdeviations)
{
	try
	{
		auto starttime = std::chrono::system_clock::now();
		std::cout << "\n\n\n\n\nSolving instance " << filename;

		IVM::Instance data;
		IVM::IP_model_allocation model_allocation;
		IVM::IP_model_routing model_routing;

		model_allocation.set_scenario(scenario);
		model_allocation.set_fraction_allowed_deviations(maxdeviations);

		model_routing.set_max_computation_time(600);
		model_routing.set_max_nb_trucks(50);
		model_routing.set_max_nb_segments(9);
		//model_routing.set_optimality_tolerance(0.005); // 0.50%


		double total_objval_routing = 0;
		double objval_allocation = 0;

		data.read_data_xml(filename);

		model_allocation.run(data);
		objval_allocation = model_allocation.objective_value();

		total_objval_routing = 0;
		for (int d = 0; d < data.nb_days() * data.nb_weeks(); ++d)
		{
			model_routing.run(data, d);
			total_objval_routing += model_routing.objective_value();
		}

		std::chrono::duration<double, std::ratio<1, 1>> elapsedtime = std::chrono::system_clock::now() - starttime;
		std::cout << "\n\n\nTotal elapsed time (s): " << elapsedtime.count();
		std::cout << "\n\nObjective value allocation: " << objval_allocation;
		std::cout << "\nTotal objective value routing: " << total_objval_routing;
		std::cout << "\n\n\n\n==========================================================================================================";

		logfile << "\n" << data.name_instance()
			<< "\t" << maxdeviations;

		if (scenario == IVM::IP_model_allocation::FIXED_WEEK_SAME_DAY || scenario == IVM::IP_model_allocation::FIXED_WEEK_FREE_DAY)
			logfile << "\tJa";
		else
			logfile << "\tNee";
		if (scenario == IVM::IP_model_allocation::FIXED_WEEK_SAME_DAY)
			logfile << "\tJa";
		else
			logfile << "\tNee";

		logfile << "\t??" // overslag
			<< "\t" << objval_allocation
			<< "\t" << total_objval_routing
			<< "\t" << elapsedtime.count();
		logfile.flush();
	}
	catch (const std::exception& e)
	{
		std::cout << "\n\n\n" << e.what();
	}
}



int main()
{
	/*std::array<std::string, 8> files{
		"IVM_perceel1_huidig.xml",
		"IVM_perceel2_huidig.xml",
		"IVM_perceel3_huidig.xml",
		"IVM_perceel4_huidig.xml",
		"IVM_perceel1_overslag.xml",
		"IVM_perceel2_overslag.xml",
		"IVM_perceel3_overslag.xml",
		"IVM_perceel4_overslag.xml"
	};*/

	std::array<std::string, 1> files{
		"IVM_perceel3_huidig.xml"
		//"IVM_allezones.xml",
		//"IVM_allezones_overslag.xml",
		//"IVM_gemeenten_3bezoeken.xml",
		//"IVM_gemeenten_overslag_3bezoeken.xml",
		//"IVM_gemeenten_1bezoek.xml",
		//"IVM_gemeenten_overslag_1bezoek.xml",
		//"IVM_gemeenten_overslag_1dag.xml",
		//"IVM_gemeenten_1dag.xml"
	};


	logfile.open("log.txt");
	logfile << "Instantie\tMax_afwijkingen\tVaste_week\tVaste_dag\tOverslag\tDoelfunctiewaarde_toewijzingsmodel\tDoelfunctiewaarde_routemodel\tRekentijd_(s)";

	for (auto& f : files)
	{
		solve_instance(f, IVM::IP_model_allocation::FIXED_WEEK_SAME_DAY, 100.0);
	}

	logfile.close();





	std::cout << "\n\n\n\n\n\n";
	return 0;
}