#include "data.h"
#include "models.h"
#include <iostream>
#include <exception>



int main()
{
	IVM::Instance data;
	IVM::IP_model_allocation model_allocation;
	IVM::IP_model_routing model_routing;

	try 
	{
		data.read_data_xml("large_instance.xml");

		model_allocation.set_scenario(IVM::IP_model_allocation::FREE_WEEK_FREE_DAY);
		model_allocation.set_fraction_allowed_deviations(0);
		model_allocation.run(data);

		for (int d = 0; d < data.nb_days() * data.nb_weeks(); ++d)
		{
			model_routing.run(data, d);
		}
	}
	catch (const std::exception& e) 
	{
		std::cout << "\n\n\n" << e.what();
	}


	std::cout << "\n\n\n\n\n\n";
	return 0;
}