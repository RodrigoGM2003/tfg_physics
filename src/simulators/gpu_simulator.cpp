#include "gpu_simulator.h"

#include <iostream>
#include <chrono>

#include "glm/gtc/matrix_transform.hpp" 
#include "glm/gtx/transform.hpp"
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtc/random.hpp" 
#include "glm/gtx/component_wise.hpp"


GpuSimulator::GpuSimulator(
    std::vector<glm::mat4>* transforms, 
    const std::vector<SimpleVertex>* static_vertices, 
    const std::vector<unsigned int>* static_indices
) : sim_transforms(transforms), 
    sim_static_vertices(static_vertices), 
    sim_static_indices(static_indices){

    initializeData();

    //Transform update shader
    m_transform_shader.setShader("sphere_transforms.glsl");
    m_transform_shader.bind();
    
    //Narrow phase shader
    m_broad_phase_shader.setShader("oparallel_naive.glsl");
    m_broad_phase_shader.useTimer(false);
    m_broad_phase_shader.bind();

    //SSBOs
    m_transform_ssbo.setBuffer(sim_transforms->data(), sim_transforms->size() * sizeof(glm::mat4), GL_DYNAMIC_DRAW);
    m_transform_ssbo.unbind();

    m_properties_ssbo.setBuffer(sim_properties.data(), sim_properties.size() * sizeof(physics::Properties), GL_DYNAMIC_DRAW);
    m_properties_ssbo.unbind();

    m_results_ssbo.setBuffer(nullptr, sim_transforms->size() * sizeof(int), GL_DYNAMIC_DRAW);
    m_results_ssbo.unbind();

    m_spheres_ssbo.setBuffer(sim_spheres.data(), sim_spheres.size() * sizeof(glm::vec4), GL_DYNAMIC_DRAW);
    m_spheres_ssbo.unbind();

    m_transform_ssbo.bindToBindingPoint(1);
    m_properties_ssbo.bindToBindingPoint(5);
    m_results_ssbo.bindToBindingPoint(4);
    m_spheres_ssbo.bindToBindingPoint(7);
}

GpuSimulator::~GpuSimulator(){
    sim_transforms = nullptr;
    sim_static_vertices = nullptr;
    sim_static_indices = nullptr;
}

void GpuSimulator::update(float delta_time){
    //Transform phase
    int work_groups = (sim_transforms->size() + 256 - 1) / 256;
    m_transform_shader.bind();
    m_transform_shader.use();
    m_transform_shader.setUniform1f("delta_time", delta_time);
    m_transform_shader.dispatch(work_groups, 1, 1);
    m_transform_shader.waitForCompletion(GL_SHADER_STORAGE_BARRIER_BIT);

    //Broad phase
    // OPARALLEL SPHERE
    // work_groups = (sim_transforms->size() + 8 - 1) / 8;
    // m_broad_phase_shader.bind();
    // m_broad_phase_shader.use();
    // m_broad_phase_shader.dispatch(work_groups * 64, 1, 1);
    // m_broad_phase_shader.waitForCompletion(GL_SHADER_STORAGE_BARRIER_BIT);

    // SHARED SPHERE
    // work_groups = (sim_transforms->size() + 256 - 1) / 256;
    // m_broad_phase_shader.bind();
    // m_broad_phase_shader.use();
    // m_broad_phase_shader.dispatch(work_groups, 1, 1);
    // m_broad_phase_shader.waitForCompletion(GL_SHADER_STORAGE_BARRIER_BIT);

    
    //Narrow phase

    //Resolution phase

    // std::cout<<time<<std::endl;
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


void GpuSimulator::initializeData(){
    //Create AABBs
    float base_radius = utils::calculateRadius(*sim_static_vertices);
    std::cout<<base_radius<<std::endl;
    sim_spheres.resize(sim_transforms->size(), glm::vec4(0.0f, 0.0f, 0.0f, base_radius));

    sim_properties.resize(sim_transforms->size());

    for (int i = 0; i < sim_transforms->size(); i++){
        
        auto transform = &sim_transforms->at(i);
        
        glm::vec3 scale = utils::scaleFromTransform(*transform); 
        sim_spheres[i].w *= glm::max(scale.x, glm::max(scale.y, scale.z));

        sim_spheres[i][0] = (*transform)[3][0];
        sim_spheres[i][1] = (*transform)[3][1];
        sim_spheres[i][2] = (*transform)[3][2];

        sim_properties[i].velocity = glm::linearRand(glm::vec3(-0.05f), glm::vec3(0.05f));
        sim_properties[i].acceleration = glm::vec3(0.0f);
        sim_properties[i].angular_velocity = glm::linearRand(glm::vec3(-0.05f), glm::vec3(0.05f));
        sim_properties[i].angular_acceleration = glm::vec3(0.0f);
    }
}