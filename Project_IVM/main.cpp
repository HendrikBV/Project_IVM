#include "data.h"
#include "models.h"
#include "auxiliaries.h"
#include <iostream>
#include <array>
#include <exception>
#include <chrono>


void solve_instance(const std::string& filename)
{
	try
	{
		auto starttime = std::chrono::system_clock::now();

		IVM::Instance data;
		IVM::IP_model_allocation model_allocation;
		IVM::IP_model_routing model_routing;

		double total_objval_routing = 0;
		double objval_allocation = 0;

		data.read_data_xml(filename);

		model_allocation.set_scenario(IVM::IP_model_allocation::FREE_WEEK_FREE_DAY);
		model_allocation.set_fraction_allowed_deviations(00);
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
	}
	catch (const std::exception& e)
	{
		std::cout << "\n\n\n" << e.what();
	}
}



int main()
{
	std::array<std::string, 8> files{
		"IVM_perceel1_huidig.xml",
		"IVM_perceel2_huidig.xml",
		"IVM_perceel3_huidig.xml",
		"IVM_perceel4_huidig.xml",
		"IVM_perceel1_overslag.xml",
		"IVM_perceel2_overslag.xml",
		"IVM_perceel3_overslag.xml",
		"IVM_perceel4_overslag.xml"
	};

	for (auto&& f : files)
		solve_instance(f);

	std::cout << "\n\n\n\n\n\n";
	return 0;
}