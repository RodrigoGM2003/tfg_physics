#ifndef COLLISION_DETECTOR_H
#define COLLISION_DETECTOR_H

#pragma once

#include <vector>

#include "glm/glm.hpp"
#include "vertex_buffer.h"

#include "simulators/simulable.h"
#include "utils.h"
#include "../buffers/shader_storage_buffer.h"
#include "compute_shader.h"

/**
 * @brief class representation of a simulator running on the cpu
 */
class CollisionDetector : public Simulable{ 
private:
    std::vector<glm::mat4>* sim_transforms;
    const std::vector<SimpleVertex>* sim_static_vertices; //SimpleVertex data
    const std::vector<unsigned int>* sim_static_indices; //SimpleVertex indices
    const std::vector<glm::vec4>* m_object_vertices; 
    const std::vector<glm::vec4>* m_object_normals; 
    const std::vector<glm::vec4>* m_object_edges; 

    std::vector<physics::AABB> sim_aabbs;
    std::vector<glm::vec4> sim_spheres;
    std::vector<physics::Properties>* sim_properties;

    // std::vector<unsigned int> m_object_edges; 

    std::vector<float> m_normal_impulses;
    std::vector<glm::vec3> m_tangent_impulses;
    std::vector<glm::vec3> m_delta_linear_impulses;
    std::vector<glm::vec3> m_delta_angular_impulses;

    ComputeShader m_transform_shader;
    ComputeShader m_broad_phase_shader;
    ComputeShader m_narrow_phase_shader;
    ComputeShader m_impulse_phase_shader;
    ComputeShader m_accumulation_phase_shader;

    ShaderStorageBuffer m_transform_ssbo;
    ShaderStorageBuffer m_aabbs_ssbo;
    ShaderStorageBuffer m_properties_ssbo;
    ShaderStorageBuffer m_results_ssbo;
    ShaderStorageBuffer m_spheres_ssbo;
    ShaderStorageBuffer m_collision_pair_ssbo;
    ShaderStorageBuffer m_collision_count_ssbo;
    ShaderStorageBuffer m_second_results_ssbo;
    ShaderStorageBuffer m_object_vertices_ssbo;
    ShaderStorageBuffer m_object_normals_ssbo;
    ShaderStorageBuffer m_object_edges_ssbo;

    unsigned int frames = 0;
    unsigned int broad_accumulator = 0;
    unsigned int narrow_accumulator = 0;
    float avg_narrow_time = 0;
    float avg_broad_time = 0;

    unsigned int m_zero = 0;

public:
    /**
     * @brief Constructor
     * @param transforms pointer to the transform matrix of the objects
     * @param static_vertices pointer to the original vertices of the geometry
     * @param static_indices pointer to the order in which each triangle is being drawn
     */
    CollisionDetector(
        std::vector<glm::mat4>* transforms, 
        const std::vector<SimpleVertex>* static_vertices, 
        const std::vector<unsigned int>* static_indices,
        const std::vector<glm::vec4>* object_vertices,
        const std::vector<glm::vec4>* object_normals, 
        const std::vector<glm::vec4>* object_edges,
        std::vector<physics::Properties>* properties
    );
    
    /**
     * @brief class destroyer. The memory is not dereferenced.
     */
    ~CollisionDetector();

    /**
     * @brief take a step on the simulation accounting for delta_time seconds
     */
    void update(float delta_time, glm::vec3 gravity = glm::vec3(0.0f, 0.0f, 0.0f)) override;

private:

    /**
     * @brief Initializes the data for the sumulation
     */
    void initializeData();
};




#endif // COLLISION_DETECTOR_H