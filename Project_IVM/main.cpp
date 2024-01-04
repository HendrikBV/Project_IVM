#include "data.h"
#include "models.h"
#include "auxiliaries.h"
#include <iostream>
#include <exception>
#include <chrono>



int main()
{
#ifndef IGNORE

	auto starttime = std::chrono::system_clock::now();

	double total_objval_routing = 0;
	try 
	{
		IVM::Instance data;
		IVM::IP_model_allocation model_allocation;
		IVM::IP_model_routing model_routing;

		data.read_data_xml("IVM_huidig_perceel1.xml");

		model_allocation.set_scenario(IVM::IP_model_allocation::CURRENT_CALENDAR);
		model_allocation.set_fraction_allowed_deviations(0);
		model_allocation.run(data);

		total_objval_routing = 0;
		for (int d = 0; d < data.nb_days() * data.nb_weeks(); ++d)
		{
			model_routing.run(data, d);
			total_objval_routing += model_routing.objective_value();
		}
	}
	catch (const std::exception& e) 
	{
		std::cout << "\n\n\n" << e.what();
	}

	std::chrono::duration<double, std::ratio<1, 1>> elapsedtime = std::chrono::system_clock::now() - starttime;
	std::cout << "\n\n\nTotal elapsed time (s): " << elapsedtime.count();
	std::cout << "\n\nTotal objective value routing: " << total_objval_routing;

#endif // !IGNORE

#ifdef IGNORE	
	aux::TransformerXML t;
	t.transform("input_t.txt", "IVM_huidig_perceel1.xml");
#endif 


	std::cout << "\n\n\n\n\n\n";
	return 0;
}