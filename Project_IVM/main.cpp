#include "data_Jens.h"
#include "models_Jens.h"

#include <iostream>
#include <exception>

int main()
{
	IVM::Data data;
	data.read_data("Instance1_Jens.txt");
	data.print_data();

	IVM::IP_column_generation model;
	try {
		model.run_column_generation(data);
	} 
	catch (const std::exception& e)
	{
		std::cout << "\n\n\n" << e.what();
	}

	std::cout << "\n\n\n\n\n\n";
	return 0;
}