#include "data.h"
#include "models.h"

#include <iostream>
#include <exception>

int main()
{
	/*IVM::Instance instance;
	instance.read_data_xml("random_instance.xml");*/

	IVM::Instance data;
	data.read_data_xml("small_instance.xml");

	IVM::IP_model_allocation model;
	try {
		model.run(data);
	}
	catch (const std::exception& e) {
		std::cout << "\n\n\n" << e.what();
	}

	

	std::cout << "\n\n\n\n\n\n";
	return 0;
}