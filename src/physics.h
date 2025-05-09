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
        alignas(16) glm::vec3 min;
        alignas(16) glm::vec3 max;
        alignas(16) glm::vec3 center;
        alignas(16) glm::vec3 extents; 
    };

    /**
     * @brief Physics properties of an object
     */
    // struct Properties{
    //     alignas(16) glm::vec3 velocity;
    //     alignas(16) glm::vec3 acceleration;

    //     alignas(16) glm::vec3 angular_velocity;
    //     alignas(16) glm::vec3 angular_acceleration;

    //     alignas(16) float inverseMass;
    //     alignas(16) glm::mat3 inverseInertiaTensor;
        
    //     alignas(16) double friction;
    // };
    struct Properties {
        // Corresponds to GLSL: vec4 velocity;
        // Use glm::vec4 for direct 16-byte mapping. alignas(16) is good practice/often default.
        alignas(16) glm::vec3 velocity;             // Offset 0, Size 16
    
        // Corresponds to GLSL: vec4 acceleration;
        alignas(16) glm::vec3 acceleration;         // Offset 16, Size 16
    
        // Corresponds to GLSL: vec4 angular_velocity;
        alignas(16) glm::vec3 angular_velocity;     // Offset 32, Size 16
    
        // Corresponds to GLSL: vec4 angular_acceleration;
        alignas(16) glm::vec3 angular_acceleration; // Offset 48, Size 16
    
        // Corresponds to GLSL: float inverseMass;
        // GLSL offset: 64. Current C++ offset is 64.
        alignas(16) float inverseMass;              // Offset 64, Size 4 (alignas helps if compiler tried to pack differently)
                                                   // Next C++ offset: 68
    
        // Padding to match GLSL's 12-byte padding before mat3
        // GLSL inverseInertiaTensor starts at offset 80. Current C++ offset is 68.
        // Need 80 - 68 = 12 bytes of padding.
        unsigned char padding0[12];                // Offset 68, Size 12. Next C++ offset: 80.
    
        // Corresponds to GLSL: mat3 inverseInertiaTensor;
        // GLSL representation: 3 columns, each 16 bytes.
        // We need to represent this in C++ by manually laying out the columns.
        // Each column of the glm::mat3 will go into the .xyz of a glm::vec4.
        alignas(16) glm::vec4 invTensorCol0;        // Offset 80, Size 16
        alignas(16) glm::vec4 invTensorCol1;        // Offset 96, Size 16
        alignas(16) glm::vec4 invTensorCol2;        // Offset 112, Size 16
                                                   // Next C++ offset: 128
    
        // Corresponds to GLSL: float friction;
        // GLSL offset: 128. Current C++ offset is 128.
        alignas(4) float friction;                 // Offset 128, Size 4 (was double, changed to float)
                                                   // Next C++ offset: 132.
    
        // Final padding to make the total struct size a multiple of 16 (for arrays of structs)
        // GLSL struct stride will be 144 bytes. Current C++ size is 132.
        // Need 144 - 132 = 12 bytes of padding.
        unsigned char finalPadding[12];            // Offset 132, Size 12.
                                                   // Total C++ struct size: 144 bytes.
    
        // Helper function to set the matrix columns correctly
        void setInverseInertiaTensor(const glm::mat3& original_mat) {
            // glm::mat3 is column-major: original_mat[0] is the first column, etc.
            invTensorCol0 = glm::vec4(original_mat[0], 0.0f); // Store column 0 in .xyz, .w = 0
            invTensorCol1 = glm::vec4(original_mat[1], 0.0f); // Store column 1 in .xyz, .w = 0
            invTensorCol2 = glm::vec4(original_mat[2], 0.0f); // Store column 2 in .xyz, .w = 0
        }
    
        // Default constructor to zero out padding for safety, if desired
        Properties() : velocity(0.0f), acceleration(0.0f), angular_velocity(0.0f), angular_acceleration(0.0f),
                       inverseMass(0.0f), invTensorCol0(0.0f), invTensorCol1(0.0f), invTensorCol2(0.0f),
                       friction(0.0f) {
            for(int i=0; i<12; ++i) padding0[i] = 0;
            for(int i=0; i<12; ++i) finalPadding[i] = 0;
        }
    };

    /**
     * @brief Physics properties of an object
     */
    struct alignas(16) ContactManifold{
        unsigned int indexA;
        unsigned int indexB;
        float padding1[2];
        glm::vec4 normal;          
        float depth;          
        float padding2[3];                
        glm::vec4 rAWorld;              
        glm::vec4 rBWorld;            


        // ContactManifold()
        // : indexA(0), indexB(0), normal(0.0f), depth(0.0f) {}
          ContactManifold()
          : indexA(0), indexB(0), normal(0.0f), depth(0.0f),
            rAWorld(0.0f), rBWorld(0.0f) {}
    };

    /**
     * @brief Representation of an object
     */
    struct Object{
        glm::mat4* transform; /* Transformation matrix of the object */
        const std::vector<Vertex>* static_vertices; /* Non-transformed vertices */
        
        AABB aabb; /* Static axis aligned bounding box (for broad phase collision detection) */

        Properties physics; /* Physical properties of an object*/
    };

    /**
     * @brief Representation of an object using gpu
     */
    struct GpuObject{
        glm::mat4* transform; /* Transformation matrix of the object */
        const std::vector<Vertex>* static_vertices; /* Non-transformed vertices */
        
        AABB* aabb; /* Static axis aligned bounding box (for broad phase collision detection) */

        Properties* physics; /* Physical properties of an object*/
    };
}


#endif //PHYSICS_H