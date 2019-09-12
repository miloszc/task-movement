/*
 * cost.cpp
 *
 *  Created on: 20 kwi 2016
 *      Author: miloszc
 */
#include "cost.h"

#include <string>
#include <iostream>
#include <sstream>

namespace scheduler {

Cost::~Cost() {
}

std::string Cost::get_string() const {
	std::ostringstream os;
	os << std::fixed << alg_name << ";" << task_order << ";" << f << ";" << nedges << ";" << ncolors << ";" <<summary_energy << ";" << summary_time << ";"
			  << computation_time << ";" << communication_time << ";" << computation_energy << ";" << communication_energy;
	return os.str();
}

std::string Cost::get_string_verbose() const {
	std::ostringstream os;
	os << std::fixed << alg_name << ";" << task_order << ";" << f << ";" 
		<< " \nnedges: " << nedges << ";"
		<< " \ndelta: " << delta << ";"
		<< " \nmu: " << mu << ";"
		<< " \nncolors: " << ncolors << ";" 
		<< " \nsummary_energy: " << summary_energy 
		<< " \nsummary_time: " << summary_time << ";"
		<< " \ncomputation_time: " << computation_time << ";" 
		<< " \ncommunication_time: " << communication_time << ";" 
		<< " \ncompuputation_energy: " << computation_energy << ";" 
		<< " \ncommunication_energy: " << communication_energy;
	return os.str();
}

std::string Cost::get_string_verbose2() const {
	std::ostringstream os;
	os << std::fixed << alg_name << ";" << task_order << ";" << f << ";"
		<< "\n" << nedges
		<< "\n" << delta
		<< "\n" << mu
		<< "\n" << ncolors
		<< "\n" << summary_energy
		<< "\n" << summary_time
		<< "\n" << computation_time
		<< "\n" << communication_time
		<< "\n" << computation_energy
		<< "\n" << communication_energy;
	return os.str();
}

std::ostream& operator<<(std::ostream &strm, const Cost &cost) {
	strm << cost.get_string_verbose2();
	return strm;
//  return strm << cost.alg_name << ";;;" << cost.nedges << ";" << cost.ncolors << ";" << cost.summary_energy << ";" << cost.summary_time << ";"
//		  << cost.computation_time << ";" << cost.communication_time << ";" << cost.computation_energy << ";" << cost.communication_energy;
}

} /* namespace scheduler */
