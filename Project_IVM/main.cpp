#include "data_Jens.h"
#include "models_Jens.h"
#include "data.h"
#include "models.h"

#include <iostream>
#include <exception>

int main()
{
	IVM::Instance instance;
	instance.read_data("random_instance.xml");

	/*IVM::Data data;
	data.read_data("Instance_Allocation_Test.txt");
	data.print_data();

	IVM::IP_model_allocation model;
	try {
		model.run(data);
	}
	catch (const std::exception& e) {
		std::cout << "\n\n\n" << e.what();
	}*/

	

	std::cout << "\n\n\n\n\n\n";
	return 0;
}