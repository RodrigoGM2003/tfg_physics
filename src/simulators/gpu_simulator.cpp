#include "gpu_simulator.h"

#include <iostream>
#include <chrono>

#include "glm/gtc/matrix_transform.hpp" 
#include "glm/gtx/transform.hpp"
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtc/random.hpp" 
#include "glm/gtx/component_wise.hpp"


const glm::vec3 WORLD_MAX = glm::vec3(300.0f,300.0f,300.0f);
const glm::vec3 WORLD_MIN = glm::vec3(-300.0f,-300.0f,-300.0f);


GpuSimulator::GpuSimulator(
    std::vector<glm::mat4>* transforms, 
    const std::vector<SimpleVertex>* static_vertices, 
    const std::vector<unsigned int>* static_indices
) : sim_transforms(transforms), 
    sim_static_vertices(static_vertices), 
    sim_static_indices(static_indices){

    //Create AABBs
    physics::AABB base_aabb = utils::calculateAABB(*sim_static_vertices);
    sim_aabbs.resize(sim_transforms->size(), base_aabb);

    sim_properties.resize(sim_transforms->size());

    for (int i = 0; i < sim_transforms->size(); i++){
        
        auto transform = &sim_transforms->at(i);
        
        sim_aabbs[i].extents *= utils::scaleFromTransform(*transform);
        float max_extent = std::max(std::max(sim_aabbs[i].extents.x, sim_aabbs[i].extents.y), sim_aabbs[i].extents.z);
        sim_aabbs[i].extents = glm::vec3(max_extent);
        sim_aabbs[i] = utils::updateAABB(sim_aabbs[i], transform);

        sim_properties[i].velocity = glm::linearRand(glm::vec3(-0.05f), glm::vec3(0.05f));
        sim_properties[i].acceleration = glm::vec3(0.0f);
        sim_properties[i].angular_velocity = glm::linearRand(glm::vec3(-0.05f), glm::vec3(0.05f));
        sim_properties[i].angular_acceleration = glm::vec3(0.0f);
    }

    //Transform update shader
    m_transform_shader.setShader("go_back.glsl");
    m_transform_shader.bind();
    // m_transform_shader.setUniform1f("u_boundary_min", -27.5f);
    // m_transform_shader.setUniform1f("u_boundary_max", 27.5f);

    m_physics_shaders.resize(10, ComputeShader());
    m_physics_shaders.at(0).setShader("naive.glsl");
    m_physics_shaders.at(0).useTimer(true);
    m_physics_shaders.at(0).bind();

    //SSBOs
    m_transform_ssbo.setBuffer(sim_transforms->data(), sim_transforms->size() * sizeof(glm::mat4), GL_DYNAMIC_DRAW);
    m_transform_ssbo.unbind();

    m_properties_ssbo.setBuffer(sim_properties.data(), sim_properties.size() * sizeof(physics::Properties), GL_DYNAMIC_DRAW);
    m_properties_ssbo.unbind();

    m_aabbs_ssbo.setBuffer(sim_aabbs.data(), sim_aabbs.size() * sizeof(physics::AABB), GL_DYNAMIC_DRAW);
    m_aabbs_ssbo.unbind();

    m_results_ssbo.setBuffer(nullptr, sim_transforms->size() * sizeof(int), GL_DYNAMIC_DRAW);


    m_transform_ssbo.bindToBindingPoint(1);
    m_properties_ssbo.bindToBindingPoint(2);
    m_aabbs_ssbo.bindToBindingPoint(3);
    m_results_ssbo.bindToBindingPoint(4);
}

GpuSimulator::~GpuSimulator(){
    sim_transforms = nullptr;
    sim_static_vertices = nullptr;
    sim_static_indices = nullptr;
}

void GpuSimulator::update(float delta_time){

    
    m_transform_shader.bind();
    m_transform_shader.use();
    m_transform_shader.setUniform1f("delta_time", delta_time);
    
    int work_gropus = (sim_transforms->size() + 256 - 1) / 256;
    m_transform_shader.dispatch(work_gropus, 1, 1);
    m_transform_shader.waitForCompletion(GL_SHADER_STORAGE_BARRIER_BIT);
    
    unsigned int time = 0;
    for(int i = 0; i < 1; i++){
        m_physics_shaders.at(i).bind();
        m_physics_shaders.at(i).use();
        
        m_physics_shaders.at(i).dispatch(work_gropus, 1, 1);
        time = m_physics_shaders.at(i).waitForCompletion(GL_SHADER_STORAGE_BARRIER_BIT);
    }
    // float f_time = time;
    // f_time /= 1000000;
    // std::cout<<f_time<<std::endl;
}

void GpuSimulator::updateObject(physics::GpuObject& object, float delta_time){
    // Create rotation quaternion
    glm::quat rotation_quat = glm::quat(object.physics->angular_velocity * delta_time);
    
    // Extract current position and rotation from transform
    glm::vec3 position = glm::vec3((*object.transform)[3]);
    glm::mat3 rotation = glm::mat3(*object.transform);
    
    // Apply new rotation to current rotation
    glm::mat3 new_rotation = glm::mat3_cast(rotation_quat) * rotation;
    
    // Apply velocity to position
    glm::vec3 new_position = position + object.physics->velocity * delta_time;
    
    // Build new transform matrix
    glm::mat4 new_transform = glm::mat4(1.0f);
    
    // Set rotation part
    new_transform[0][0] = new_rotation[0][0];
    new_transform[0][1] = new_rotation[0][1];
    new_transform[0][2] = new_rotation[0][2];
    
    new_transform[1][0] = new_rotation[1][0];
    new_transform[1][1] = new_rotation[1][1];
    new_transform[1][2] = new_rotation[1][2];
    
    new_transform[2][0] = new_rotation[2][0];
    new_transform[2][1] = new_rotation[2][1];
    new_transform[2][2] = new_rotation[2][2];
    
    // Set position part
    new_transform[3][0] = new_position.x;
    new_transform[3][1] = new_position.y;
    new_transform[3][2] = new_position.z;
    
    // Update transform
    *object.transform = new_transform;

    *object.aabb = utils::updateAABB(*object.aabb, object.transform);
}
