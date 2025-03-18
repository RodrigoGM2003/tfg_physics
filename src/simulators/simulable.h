#ifndef SIMULABLE_H
#define SIMULABLE_H

#pragma once

#include <vector>

#include "glm/glm.hpp"
#include "vertex_buffer.h"

#include "utils.h"

/**
 * @brief class representation of a simulator running on the cpu
 */
class Simulable{
public:
    /**
     * @brief take a step on the simulation accounting for delta_time seconds
     */
    virtual void update(float delta_time) = 0;
};




#endif // SIMULABLE_H