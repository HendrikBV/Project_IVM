#include "data.h"
#include "models.h"

#include <iostream>
#include <exception>

int main()
{
	/*IVM::Instance instance;
	instance.read_data_xml("random_instance.xml");*/

	IVM::Instance data;
	data.read_data_xml("random_instance.xml");

	IVM::IP_model_allocation model;
	try {
		model.set_scenario(IVM::IP_model_allocation::FREE_WEEK_FREE_DAY);
		model.set_fraction_allowed_deviations(0.2);
		model.run(data);
	}
	catch (const std::exception& e) {
		std::cout << "\n\n\n" << e.what();
	}

	

	std::cout << "\n\n\n\n\n\n";
	return 0;
}