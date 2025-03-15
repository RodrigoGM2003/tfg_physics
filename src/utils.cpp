#include "utils.h"

#include <iostream>

namespace utils{


    physics::AABB calculateAABB(const std::vector<Vertex>& vertices){
        physics::AABB aabb;
        aabb.min = glm::vec3(FLT_MAX);
        aabb.max = glm::vec3(-FLT_MAX);

        for (const auto& vertex : vertices){
            const glm::vec3 position = {vertex.position[0], vertex.position[1], vertex.position[2]};
            aabb.min = glm::min(aabb.min, position);
            aabb.max = glm::max(aabb.max, position);
        }

        aabb.extents = (aabb.max - aabb.min) / 2.0f;
        aabb.center = aabb.min + aabb.extents;

        std::cout<<"AABB min: "<<aabb.min.x<<", "<<aabb.min.y<<", "<<aabb.min.z<<std::endl;
        std::cout<<"AABB max: "<<aabb.max.x<<", "<<aabb.max.y<<", "<<aabb.max.z<<std::endl;
        std::cout<<"AABB center: "<<aabb.center.x<<", "<<aabb.center.y<<", "<<aabb.center.z<<std::endl;
        std::cout<<"AABB extents: "<<aabb.extents.x<<", "<<aabb.extents.y<<", "<<aabb.extents.z<<std::endl;

        return aabb;
    }

    glm::vec3 scaleFromTransform(const glm::mat4& matrix){
        return {glm::length(glm::vec3(matrix[0])), glm::length(glm::vec3(matrix[1])), glm::length(glm::vec3(matrix[2]))};
    }

    physics::AABB updateObjectAABB(const physics::Object& object){
        physics::AABB result = object.aabb;
        // Extract position from transform matrix
        glm::vec3 position = glm::vec3((*object.transform)[3]);
    
        // Update AABB center position
        result.center = position;
    
        // Recalculate min and max based on center and extents
        result.min = result.center - result.extents;
        result.max = result.center + result.extents;

        return result;
    }

}