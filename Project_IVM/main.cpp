/*
	Copyright (c) 2024 KU Leuven
	Code author: Hendrik Vermuyten
*/



#include "data.h"
#include "models.h"
#include "cxxopts.h"
#include <iostream>
#include <exception>
#include <stdexcept>



int main(int argc, char* argv[])
{
	try
	{
		cxxopts::Options options("IVM optimalisatietool", "\nDit programma voert vier optimalisatiemodellen voor afvalophaling uit."
			"\n\nHet eerste model is \"allocatiepre\". Dit model maakt een kalender die de ophaling zo gelijk mogelijk spreidt."
			"\nHet tweede model is \"routing\". Dit model bepaalt de optimale routes gegeven een ophaalkalender."
			"\nHet derde model is \"allocatiepost\". Dit model wijst gegenereerde ophaalroutes toe aan ophaaldagen om een ophaalkalender te maken."
			"\nHet vierde model is \"geintegreerd\". Dit model optimaliseert de routes en de kalender gelijktijdig."
			"\nDe MIP-solver die wordt gebruik is CPLEX.\n");

		options.add_options()
			("model", "Welk optimalisatiemodel. Mogelijkheden: \"allocatiepre\", \"routing\", \"allocatiepost\", \"geintegreerd\", \"geintegreerd_fao\"", cxxopts::value<std::string>())
			("data", "Naam van het xml-bestand met de data voor de instantie", cxxopts::value<std::string>())
			("routes", "Naam van het xml-bestand met de routes", cxxopts::value<std::string>())
			("kalender", "Naam van het xml-bestand met de te volgen kalender", cxxopts::value<std::string>())
			("rekentijd", "De maximale rekentijd in seconden", cxxopts::value<double>())
			("rekentijd_subprobleem", "De maximale rekentijd per subprobleem (fix-and-optimize)", cxxopts::value<double>())
			("output", "Zet de output van de solver aan", cxxopts::value<bool>())
			("scenario", "Het optimalisatiescenario voor de kalenders (0 == FIXED_WEEK_SAME_DAY, 1 == FIXED_WEEK_FREE_DAY, 2 == FREE_WEEK_FREE_DAY", cxxopts::value<int>())
			("maxafwijkingen", "Percentage maximale afwijkingen tov huidige kalender", cxxopts::value<double>())
			("maxtrucks", "Het maximale aantal trucks in de routeoptimalisatie (te weinig = infeasible)", cxxopts::value<int>())
			("maxsegmenten", "Het maximale aantal segmenten per route (minimaal 3)", cxxopts::value<int>())
			("maxbezoeken", "Het maximale aantal bezoeken over de horizon (geintegreerd model)", cxxopts::value<int>())
			("ck", "De doelfunctiecoefficient voor afwijkingen tov de huidige kalender (model 3)", cxxopts::value<double>())
			("cb", "De doelfunctiecoefficient voor het maximale aantal trucks (model 3)", cxxopts::value<double>())
			("cs", "De doelfunctiecoefficient voor het maximale aantal bezoeken per zone (model 3)", cxxopts::value<double>())
			("help", "Uitleg programma");

		auto result = options.parse(argc, argv);

		if (argc <= 1 || result.count("help"))
		{
			std::cout << options.help();
			return EXIT_SUCCESS;
		}

		std::string model;
		if (result.count("model"))
			model = result["model"].as<std::string>();

		std::string datafile;
		if (result.count("data"))
			datafile = result["data"].as<std::string>();

		double rekentijd = 300;
		if (result.count("rekentijd"))
			rekentijd = result["rekentijd"].as<double>();

		bool output = false;
		if (result.count("output"))
			output = true;

		bool lpbestand = false;
		if (result.count("lpbestand"))
			lpbestand = true;


		if (model == "allocatiepre")
		{
			int scenario = IVM::IP_model_allocation::FREE_WEEK_FREE_DAY;
			if (result.count("scenario")) {
				scenario = result["scenario"].as<int>();
				if (scenario < 0 || scenario > 3) {
					std::cerr << "\nScenario moet gelijk zijn aan 0, 1, 2 of 3\nWe gebruiken dan maar scenario 2 (FREE_WEEK_FREE_DAY).";
					scenario = IVM::IP_model_allocation::FREE_WEEK_FREE_DAY;
				}
			}

			double maxdev = 1.0;
			if (result.count("maxafwijkingen")) {
				maxdev = result["maxafwijkingen"].as<double>();
				if (maxdev < 0) {
					maxdev = 0;
				}
			}


			IVM::Instance data;
			data.read_data_xml(datafile);

			IVM::IP_model_allocation model;
			model.set_scenario(scenario);
			model.set_fraction_allowed_deviations(maxdev);
			model.set_max_computation_time(rekentijd);
			model.set_solver_output_on(output);
			model.run(data);
		}
		else if (model == "routing")
		{
			int maxtrucks = 20;
			if (result.count("maxtrucks"))
				maxtrucks = result["maxtrucks"].as<int>();

			int maxsegmenten = 5;
			if (result.count("maxsegmenten"))
				maxsegmenten = result["maxsegmenten"].as<int>();

			std::string calendarfile;
			if (result.count("kalender"))
				calendarfile = result["kalender"].as<std::string>();

			IVM::Instance data;
			data.read_data_xml(datafile);
			data.read_allocation_xml(calendarfile);

			IVM::IP_model_routing model;
			model.set_max_nb_trucks(maxtrucks);
			model.set_max_nb_segments(maxsegmenten);
			model.set_max_computation_time(rekentijd);
			model.set_solver_output_on(output);

			double totobjval = 0;
			for (auto d = 0; d < data.nb_weeks() * data.nb_days(); ++d)
			{
				model.run(data, d);
				totobjval += model.objective_value();
			}
			std::cout << "\n\nTotale kosten alle dagen samen: " << totobjval;
		}
		else if (model == "allocatiepost")
		{
			int scenario = IVM::IP_model_allocation_post::FREE_WEEK_FREE_DAY;
			if (result.count("scenario")) {
				scenario = result["scenario"].as<int>();
				if (scenario < 0 || scenario > 3) {
					std::cerr << "\nScenario moet gelijk zijn aan 0, 1 of 2\nWe gebruiken dan maar scenario 2 (FREE_WEEK_FREE_DAY).";
					scenario = IVM::IP_model_allocation_post::FREE_WEEK_FREE_DAY;
				}
			}

			std::string routesfile;
			if (result.count("routes"))
				routesfile = result["routes"].as<std::string>();

			double coeffz = 1;
			if (result.count("ck"))
				coeffz = result["ck"].as<double>();

			double coeffbeta = 1;
			if (result.count("cb"))
				coeffbeta = result["cb"].as<double>();

			double coefftheta = 1;
			if (result.count("cs"))
				coefftheta = result["cs"].as<double>();


			IVM::Instance data;
			data.read_data_xml(datafile);
			data.read_routes_xml(routesfile);

			IVM::IP_model_allocation_post model;
			model.set_scenario(scenario);
			model.set_coefficient_z_tmdw(coeffz);
			model.set_coefficient_beta(coeffbeta);
			model.set_coefficient_theta(coefftheta);
			model.set_max_computation_time(rekentijd);
			model.set_solver_output_on(output);
			model.run(data);
		}
		else if (model == "geintegreerd")
		{
			int maxtrucks = 20;
			if (result.count("maxtrucks"))
				maxtrucks = result["maxtrucks"].as<int>();

			int maxsegmenten = 5;
			if (result.count("maxsegmenten"))
				maxsegmenten = result["maxsegmenten"].as<int>();

			int maxvisits = 1;
			if (result.count("maxbezoeken"))
				maxvisits = result["maxbezoeken"].as<int>();

			IVM::Instance data;
			data.read_data_xml(datafile);

			IVM::IP_model_integrated model;
			model.set_max_nb_trucks(maxtrucks);
			model.set_max_nb_segments(maxsegmenten);
			model.set_max_computation_time(rekentijd);
			model.set_max_visits(maxvisits);
			model.set_solver_output_on(output);
			model.run(data);
		}
		else if (model == "geintegreerd_fao")
		{
			int maxtrucks = 20;
			if (result.count("maxtrucks"))
				maxtrucks = result["maxtrucks"].as<int>();

			int maxsegmenten = 5;
			if (result.count("maxsegmenten"))
				maxsegmenten = result["maxsegmenten"].as<int>();

			double max_time_subproblem = 40;
			if (result.count("rekentijd_subprobleem"))
				max_time_subproblem = result["rekentijd_subprobleem"].as<double>();

			int maxvisits = 1;
			if (result.count("maxbezoeken"))
				maxvisits = result["maxbezoeken"].as<int>();

			IVM::Instance data;
			data.read_data_xml(datafile);

			IVM::IP_model_integrated model;
			model.set_max_nb_trucks(maxtrucks);
			model.set_max_nb_segments(maxsegmenten);
			model.set_max_computation_time(rekentijd);
			model.set_max_computation_time_subproblem(max_time_subproblem);
			model.set_max_visits(maxvisits);
			model.set_solver_output_on(output);
			model.run_fix_and_optimize(data);
		}
		else
		{
			throw std::invalid_argument("Model \"" + model + "\"bestaat niet");
		}


		std::cout << "\n\n\n";
		return EXIT_SUCCESS;
	}
	catch (const std::exception& e)
	{
		std::cout << e.what() << "\n\n\n";
		return EXIT_FAILURE;
	}
}