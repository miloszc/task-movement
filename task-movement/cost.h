/*
 * Cost.h
 *
 *  Created on: 20 kwi 2016
 *      Author: miloszc
 */

#ifndef COST_H_
#define COST_H_

#include <string>
#include <iostream>

#include "schedparams.h"

namespace scheduler {

class Cost {
public:
	Cost() : alg_name(""), task_order(IJK), f(0.1f), nedges(0), delta(0), mu(0), ncolors(0), computation_time(0),
		communication_time(0), summary_time(0), computation_energy(0),
		communication_energy(0), summary_energy(0) {};
    Cost(std::string alg_name_, int nedges_, int delta_, int mu_, int ncolors_, long long summary_energy_,
		long long summary_time_, long long computation_time_, long long communication_time_,
		long long computation_energy_, long long communication_energy_) :
        alg_name(alg_name_), task_order(IJK), f(0.1f), nedges(nedges_), delta(delta_), mu(mu_), ncolors(ncolors_),
        computation_time(computation_time_), communication_time(communication_time_),
        summary_time(summary_time_), computation_energy(computation_energy_),
        communication_energy(communication_energy_), summary_energy(summary_energy_) {};
	Cost(std::string alg_name_, TaskOrder task_order_, float f_, int nedges_, int delta_, int mu_, int ncolors_, long long summary_energy_,
	        long long summary_time_, long long computation_time_, long long communication_time_,
	        long long computation_energy_, long long communication_energy_) :
	    alg_name(alg_name_), task_order(task_order_), f(f_), nedges(nedges_), delta(delta_), mu(mu_), ncolors(ncolors_),
	    computation_time(computation_time_), communication_time(communication_time_),
	    summary_time(summary_time_), computation_energy(computation_energy_),
	    communication_energy(communication_energy_), summary_energy(summary_energy_) {};
	Cost(const Cost &obj) :
		alg_name(obj.alg_name), task_order(obj.task_order), f(obj.f), nedges(obj.nedges), delta(obj.delta), mu(obj.mu), ncolors(obj.ncolors),
		computation_time(obj.computation_time), communication_time(obj.communication_time),
		summary_time(obj.summary_time), computation_energy(obj.computation_energy),
		communication_energy(obj.communication_energy), summary_energy(obj.summary_energy) {};
	virtual ~Cost();

	std::string get_string() const;
	std::string get_string_verbose() const;
	std::string get_string_verbose2() const;

	std::string alg_name;
    // Order of input tasks
    TaskOrder task_order;
    // load factor
    float f;
	long nedges;
	long delta;
	long mu;
	long ncolors;
	long long computation_time;
	long long communication_time;
	long long summary_time;
	long long computation_energy;
	long long communication_energy;
	long long summary_energy;

private:
	friend std::ostream& operator<<(std::ostream&, const Cost&);
};

} /* namespace scheduler */

#endif /* COST_H_ */
