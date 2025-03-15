#ifndef UTILS_HPP
#define UTILS_HPP

#pragma once

#include <vector>

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include "physics.h"

namespace utils{



    /**
     * @brief Calculate the AABB of a set of vertices
     * @param vertices The vertices
     * @return The AABB of the vertices
     */
    physics::AABB calculateAABB(const std::vector<Vertex>& vertices);

    /**
     * @brief Get the scale from a transformation matrix
     * @param matrix The transformation matrix
     * @return The scale of the transformation matrix
     */
    glm::vec3 scaleFromTransform(const glm::mat4& matrix);


    /**
     * @brief Calculate the objects updated AABB
     * @param object the simulation object we want to update
     * @return The updated AABB
     */
    physics::AABB updateObjectAABB(const physics::Object& object);

}


#endif // UTILS_HPP