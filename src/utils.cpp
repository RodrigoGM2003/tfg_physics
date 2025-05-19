#include "utils.h"

#include <iostream>
#include <array>
#include <cmath>
#include <algorithm>


namespace utils{
    static constexpr float EPS = 1e-6f;

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
        
        // std::cout<<"AABB min: "<<aabb.min.x<<", "<<aabb.min.y<<", "<<aabb.min.z<<std::endl;
        // std::cout<<"AABB max: "<<aabb.max.x<<", "<<aabb.max.y<<", "<<aabb.max.z<<std::endl;
        // std::cout<<"AABB center: "<<aabb.center.x<<", "<<aabb.center.y<<", "<<aabb.center.z<<std::endl;
        // std::cout<<"AABB extents: "<<aabb.extents.x<<", "<<aabb.extents.y<<", "<<aabb.extents.z<<std::endl;
        
        return aabb;
    }

    float calculateRadius(const std::vector<SimpleVertex>& vertices){
        float radius = 0;
        for (const auto& vertex : vertices) {
            const glm::vec3 position = {vertex.position[0], vertex.position[1], vertex.position[2]};
            float distance = glm::distance(position, glm::vec3(0));
            if(radius < distance)
                radius = distance;
        }
        return radius;
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



    // Normalize a 3-component vector in place
    inline void normalize3(float v[3]) {
        float len = std::sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
        if (len > EPS) {
            v[0] /= len;  v[1] /= len;  v[2] /= len;
        }
    }

    // Canonicalize a direction so that the first non-zero component is positive
    void canonicalize(glm::vec3 &v) {
        for (int i = 0; i < 3; ++i) {
            if (std::abs(v[i]) > EPS) {
                if (v[i] < 0.0f) v = -v;
                break;
            }
        }
    }

    // Compare two float4s for “almost equality”
    bool almostEqual(const glm::vec4 &a, const glm::vec4 &b) {
        return glm::all(glm::lessThanEqual(glm::abs(a - b), glm::vec4(EPS)));
    }

    // 1) Extract all positions as vec4(x,y,z,1)
    std::vector<glm::vec4> extractPositions(const SimpleVertex* verts, size_t count) {
        std::vector<glm::vec4> pos;
        pos.reserve(count);
        for (size_t i = 0; i < count; ++i) {
            pos.emplace_back(verts[i].position[0],
                            verts[i].position[1],
                            verts[i].position[2],
                            1.0f);
        }
        return pos;
    }

    // 2) Extract unique normals as vec4(nx,ny,nz,0)
    std::vector<glm::vec4> extractNormals(const SimpleVertex* verts, size_t count) {
        std::vector<glm::vec4> normals;
        normals.reserve(count);
        for (size_t i = 0; i < count; ++i) {
            glm::vec3 n(verts[i].normal[0],
                        verts[i].normal[1],
                        verts[i].normal[2]);
            n = glm::normalize(n);
            canonicalize(n);
            glm::vec4 fn(n, 0.0f);

            bool dup = std::any_of(normals.begin(), normals.end(),
                                [&](auto &ex){ return almostEqual(ex, fn); });
            if (!dup) normals.push_back(fn);
        }
        return normals;
    }

    // 3) Extract unique edge-directions as vec4(dx,dy,dz,0)
    std::vector<glm::vec4> extractEdges(const SimpleVertex* verts,
                                        const unsigned int* indices,
                                        size_t idxCount)
    {
        std::vector<glm::vec4> edges;
        edges.reserve(idxCount);
        for (size_t i = 0; i + 2 < idxCount; i += 3) {
            unsigned int tri[3] = {
                indices[i+0],
                indices[i+1],
                indices[i+2]
            };
            for (int e = 0; e < 3; ++e) {
                auto a = tri[e];
                auto b = tri[(e+1)%3];
                glm::vec3 d = glm::vec3(
                    verts[b].position[0] - verts[a].position[0],
                    verts[b].position[1] - verts[a].position[1],
                    verts[b].position[2] - verts[a].position[2]
                );
                d = glm::normalize(d);
                canonicalize(d);
                glm::vec4 fe(d, 0.0f);

                bool dup = std::any_of(edges.begin(), edges.end(),
                                    [&](auto &ex){ return almostEqual(ex, fe); });
                if (!dup) edges.push_back(fe);
            }
        }
        return edges;
    }

}