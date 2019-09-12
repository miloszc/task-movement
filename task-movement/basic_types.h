/*
 * basic_types.h
 *
 *  Created on: 1 maj 2016
 *      Author: miloszc
 */

#ifndef BASIC_TYPES_H_
#define BASIC_TYPES_H_

namespace scheduler {

typedef struct Vector3i_t {

	Vector3i_t() : x(0), y(0), z(0) {};
	Vector3i_t(int _x, int _y, int _z): x(_x), y(_y), z(_z) {};

    int x;
    int y;
    int z;
} Vector3i;

} /* namespace scheduler */

#endif /* BASIC_TYPES_H_ */
