#ifndef GPU_SIMULATOR_H
#define GPU_SIMULATOR_H

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
class GpuSimulator : public Simulable{ 
private:
    std::vector<glm::mat4>* sim_transforms; //SSBO 3
    const std::vector<SimpleVertex>* sim_static_vertices; //SimpleVertex data
    const std::vector<unsigned int>* sim_static_indices; //SimpleVertex indices

    std::vector<physics::AABB> sim_aabbs; //SSBO 1
    std::vector<physics::Properties> sim_properties; //SSBO2

    ComputeShader m_transform_shader;
    std::vector<ComputeShader> m_physics_shaders;

    ShaderStorageBuffer m_transform_ssbo;
    ShaderStorageBuffer m_aabbs_ssbo;
    ShaderStorageBuffer m_properties_ssbo;
    ShaderStorageBuffer m_results_ssbo;

public:
    /**
     * @brief Constructor
     * @param transforms pointer to the transform matrix of the objects
     * @param static_vertices pointer to the original vertices of the geometry
     * @param static_indices pointer to the order in which each triangle is being drawn
     */
    GpuSimulator(std::vector<glm::mat4>* transforms, const std::vector<SimpleVertex>* static_vertices, const std::vector<unsigned int>* static_indices);
    
    /**
     * @brief class destroyer. The memory is not dereferenced.
     */
    ~GpuSimulator();

    /**
     * @brief take a step on the simulation accounting for delta_time seconds
     */
    void update(float delta_time) override;

private:
    /**
     * @brief take a step for each object
     */
    void updateObject(physics::GpuObject& object, float delta_time);
};




#endif // GPU_SIMULATOR_H