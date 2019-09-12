/*
 * schedparams.h
 *
 *  Created on: 1 maj 2016
 *      Author: miloszc
 */

#ifndef SCHEDPARAMS_H_
#define SCHEDPARAMS_H_

namespace scheduler {

enum TaskOrder {
    IJK = 0,
    JIK = 1,
    KIJ = 2,
    JKI = 3,
    RANDOM = 4,
    KJI = 5,
    IKJ = 6
};

typedef struct SchedParams_t {
    SchedParams_t() :
            alg_idx(0), task_order(IJK), f(0.0f) {};
    SchedParams_t(int alg_idx_, TaskOrder task_order_, float f_) :
        alg_idx(alg_idx_), task_order(task_order_), f(f_) {};
    // Select algorithm
    int alg_idx;
    // Order of input tasks
    TaskOrder task_order;
    // load factor
    float f;

    bool operator==(const SchedParams_t &o) const {
        return alg_idx == o.alg_idx && task_order == o.task_order && f == o.f;
    }

    bool operator<(const SchedParams_t &o) const {
        return alg_idx < o.alg_idx || (alg_idx == o.alg_idx && task_order < o.task_order)
                || (alg_idx == o.alg_idx && task_order == o.task_order && f < o.f);
    }
} SchedParams;

} /* namespace scheduler */

#endif /* SCHEDPARAMS_H_ */
