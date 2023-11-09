#include "data.h"

int main()
{
	IVM::Data data;
	data.read_data("Instance1_Jens.txt");
	data.print_data();

	return 0;
}