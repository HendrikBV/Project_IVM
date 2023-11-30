#include "data.h"
#include <iostream>
#include <fstream>
#include <stdexcept>

namespace IVM
{
	bool Data::read_data(const std::string& filename)
	{
		std::ifstream file;
		file.open(filename);
		if (!file.is_open())
		{
			std::cout << "\nCouldn't open file " << filename;
			return false;
		}

		std::string names_variables;

		// general data
		file >> names_variables >> _name_instance;
		file >> names_variables >> _nb_days;
		file >> names_variables >> _nb_weeks;
		file >> names_variables >> _max_visits_customers;

		// customer data
		int nb_customers = 0;
		file >> names_variables >> nb_customers;

		file >> names_variables >> names_variables >> names_variables >> names_variables >> names_variables >> names_variables >> names_variables >> names_variables >> names_variables;
		for (int i = 0; i < nb_customers; ++i)
		{
			_customers.push_back(Customer());

			for(int )
			file >> _customers.back()._name;
			file >> _customers.back()._t_dep;
			file >> _customers.back()._t_disp;
			file >> _customers.back()._t_disp_dep;
			file >> _customers.back()._t_unl;
			file >> _customers.back()._t_trip1;
			file >> _customers.back()._t_trip2;
			file >> _customers.back()._Q;
			file >> _customers.back()._s;
		}

		return true;
	}

	void Data::print_data()
	{
		std::cout << "\n\n\n\nDATA";

		std::cout << "\n\nName: " << _name_instance;
		std::cout << "\nD = " << _D;
		std::cout << "\nc_h = " << _c_h;
		std::cout << "\nc_veh = " << _c_veh;
		std::cout << "\nT = " << _T;
		std::cout << "\nL = " << _L;
		std::cout << "\nvehicles = " << _max_vehicles;
		std::cout << "\nWm = " << _Wm;

		std::cout << "\n\nCustomers\nindex\tname\tt_dep\tt_disp\tt_disp_dep\tt_unl\tt_trip1\tt_trip2\tQ\ts";
		for (int i = 0; i < _customers.size(); ++i)
		{
			std::cout << "\n" << i + 1 << "\t" << _customers[i]._name << "\t" << _customers[i]._t_dep
				<< "\t" << _customers[i]._t_disp << "\t" << _customers[i]._t_disp_dep << "\t" << _customers[i]._t_unl
				<< "\t" << _customers[i]._t_trip1 << "\t" << _customers[i]._t_trip2 << "\t" << _customers[i]._Q
				<< "\t" << _customers[i]._s;
		}

		std::cout << "\n\n\n\n\n\n";
	}

	void Data::clear_data()
	{
		_customers.clear();
	}
}