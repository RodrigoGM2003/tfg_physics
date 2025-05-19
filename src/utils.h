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

    // Helper typedef for a float4
    // Epsilon for float comparisons


    void normalize3(float v[3]);
    void canonicalize3(glm::vec3 &v);
    bool almostEqual4(const glm::vec4 &a, const glm::vec4 &b);
    std::vector<glm::vec4> extractPositions(const SimpleVertex* verts, size_t count);
    std::vector<glm::vec4> extractNormals(const SimpleVertex* verts, size_t count);
    std::vector<glm::vec4> extractEdges(
            const SimpleVertex* verts,
            const unsigned int* indices,
            size_t idxCount);
}


#endif // UTILS_HPP