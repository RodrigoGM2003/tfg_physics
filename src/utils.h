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
     * @brief Calculate the AABB of a set of vertices
     * @param vertices The vertices
     * @return The AABB of the vertices
     */
    physics::AABB calculateAABB(const std::vector<SimpleVertex>& vertices);

    /**
     * @brief Calculate the radius of the minimal sphere that encapsulates the vertices
     * @param vertices The vertices
     * @return Radius value
     */
    float calculateRadius(const std::vector<SimpleVertex>& vertices);

    /**
     * @brief Get the scale from a transformation matrix
     * @param matrix The transformation matrix
     * @return The scale of the transformation matrix
     */
    glm::vec3 scaleFromTransform(const glm::mat4& matrix);


    /**
     * @brief Calculate the objects updated AABB
     * @param aabb the original bounding box to update
     * @param transform a pointer to a transform matrix
     * @return The updated AABB
     */
    physics::AABB updateAABB(const physics::AABB& aabb, const glm::mat4* transform);

}


#endif // UTILS_HPP