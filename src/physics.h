#ifndef PHYSICS_H
#define PHYSICS_H

#pragma once

#include <vector>

#include "glm/glm.hpp"

#include "vertex_buffer.h"

namespace physics{

    /**
     * @brief Axis Aligned Bounding Box
     */
    struct AABB {
        glm::vec3 min;
        glm::vec3 max;
        glm::vec3 center;
        glm::vec3 extents; 
    };

    /**
     * @brief Physics properties of an object
     */
    struct Properties{
        glm::vec3 velocity;
        glm::vec3 acceleration;

        glm::vec3 angular_velocity;
        glm::vec3 angular_acceleration;
    };

    /**
     * @brief Representation of an object
     */
    struct Object{
        glm::mat4* transform; /* Transformation matrix of the object */
        const std::vector<Vertex>* static_vertices; /* Non-transformed vertices */
        
        AABB aabb; /* Static axis aligned bounding box (for broad phase collision detection) */

        glm::vec3 scale; /* Scale of the object */

        Properties physics; /* Physical properties of an object*/
    };
}


#endif //PHYSICS_H