#include "utils.h"

#include <iostream>


namespace utils{


    physics::AABB calculateAABB(const std::vector<Vertex>& vertices) {
        physics::AABB aabb;
        aabb.min = glm::vec3(FLT_MAX);
        aabb.max = glm::vec3(-FLT_MAX);
    
        // Find min and max points as before
        for (const auto& vertex : vertices) {
            const glm::vec3 position = {vertex.position[0], vertex.position[1], vertex.position[2]};
            aabb.min = glm::min(aabb.min, position);
            aabb.max = glm::max(aabb.max, position);
        }
    
        // Calculate center point
        aabb.center = (aabb.min + aabb.max) / 2.0f;
        
        // Calculate initial axis-aligned extents
        glm::vec3 initial_extents = (aabb.max - aabb.min) / 2.0f;
        
        // For rotation invariance, find the maximum distance from center to any vertex
        float max_distance_squared = 0.0f;
        for (const auto& vertex : vertices) {
            const glm::vec3 position = {vertex.position[0], vertex.position[1], vertex.position[2]};
            float dist_squared = glm::dot(position - aabb.center, position - aabb.center);
            max_distance_squared = std::max(max_distance_squared, dist_squared);
        }
        
        // Set all extents to the max distance (radius of bounding sphere)
        float radius = std::sqrt(max_distance_squared);
        aabb.extents = glm::vec3(radius);
        
        // std::cout<<"AABB min: "<<aabb.min.x<<", "<<aabb.min.y<<", "<<aabb.min.z<<std::endl;
        // std::cout<<"AABB max: "<<aabb.max.x<<", "<<aabb.max.y<<", "<<aabb.max.z<<std::endl;
        // std::cout<<"AABB center: "<<aabb.center.x<<", "<<aabb.center.y<<", "<<aabb.center.z<<std::endl;
        // std::cout<<"AABB extents: "<<aabb.extents.x<<", "<<aabb.extents.y<<", "<<aabb.extents.z<<std::endl;
        
        return aabb;
    }
    
    physics::AABB calculateAABB(const std::vector<SimpleVertex>& vertices) {
        physics::AABB aabb;
        aabb.min = glm::vec3(FLT_MAX);
        aabb.max = glm::vec3(-FLT_MAX);
    
        // Find min and max points as before
        for (const auto& vertex : vertices) {
            const glm::vec3 position = {vertex.position[0], vertex.position[1], vertex.position[2]};
            aabb.min = glm::min(aabb.min, position);
            aabb.max = glm::max(aabb.max, position);
        }
    
        // Calculate center point
        aabb.center = (aabb.min + aabb.max) / 2.0f;
        
        // Calculate initial axis-aligned extents
        glm::vec3 initial_extents = (aabb.max - aabb.min) / 2.0f;
        
        // For rotation invariance, find the maximum distance from center to any vertex
        float max_distance_squared = 0.0f;
        for (const auto& vertex : vertices) {
            const glm::vec3 position = {vertex.position[0], vertex.position[1], vertex.position[2]};
            float dist_squared = glm::dot(position - aabb.center, position - aabb.center);
            max_distance_squared = std::max(max_distance_squared, dist_squared);
        }
        
        // Set all extents to the max distance (radius of bounding sphere)
        float radius = std::sqrt(max_distance_squared);
        aabb.extents = glm::vec3(radius);
        
        std::cout<<"AABB min: "<<aabb.min.x<<", "<<aabb.min.y<<", "<<aabb.min.z<<std::endl;
        std::cout<<"AABB max: "<<aabb.max.x<<", "<<aabb.max.y<<", "<<aabb.max.z<<std::endl;
        std::cout<<"AABB center: "<<aabb.center.x<<", "<<aabb.center.y<<", "<<aabb.center.z<<std::endl;
        std::cout<<"AABB extents: "<<aabb.extents.x<<", "<<aabb.extents.y<<", "<<aabb.extents.z<<std::endl;
        
        return aabb;
    }

    glm::vec3 scaleFromTransform(const glm::mat4& matrix){
        return {glm::length(glm::vec3(matrix[0])), glm::length(glm::vec3(matrix[1])), glm::length(glm::vec3(matrix[2]))};
    }

    physics::AABB updateAABB(const physics::AABB& aabb, const glm::mat4* transform){
        physics::AABB result = aabb;         
    
        // Update AABB center position
        result.center = glm::vec3((*transform)[3]);
    
        // Recalculate min and max based on center and extents
        result.min = result.center - result.extents;
        result.max = result.center + result.extents;

        return result;
    }

}