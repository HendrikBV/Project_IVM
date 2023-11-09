#include "data.h"
#include "models.h"

#include <iostream>
#include <exception>

int main()
{
	IVM::Data data;
	data.read_data("Instance1_Jens.txt");
	data.print_data();

	IVM::IP_VNDS model;
	try {
		model.run(data);
	} 
	catch (const std::exception& e)
	{
		std::cout << "\n\n\n" << e.what();
	}

	std::cout << "\n\n\n\n\n\n";
	return 0;
}