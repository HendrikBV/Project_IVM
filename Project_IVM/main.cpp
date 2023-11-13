#include "data.h"
#include "models.h"

#include <iostream>
#include <exception>

int main()
{
	IVM::Data data;
	data.read_data("Instance2_Jens.txt");
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