#include "gpu_simulator.h"

#include <iostream>

#include "glm/gtc/matrix_transform.hpp" 
#include "glm/gtx/transform.hpp"
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtc/random.hpp" 
#include "glm/gtx/component_wise.hpp"


GpuSimulator::GpuSimulator(
    std::vector<glm::mat4>* transforms, 
    const std::vector<Vertex>* static_vertices, 
    const std::vector<unsigned int>* static_indices
) : sim_transforms(transforms), 
    sim_static_vertices(static_vertices), 
    sim_static_indices(static_indices){

    //Create the objects
    sim_objects.reserve(sim_transforms->size());

    //Create AABBs
    physics::AABB base_aabb = utils::calculateAABB(*sim_static_vertices);
    sim_aabbs.resize(sim_transforms->size(), base_aabb);

    sim_properties.resize(sim_transforms->size());

    for (int i = 0; i < sim_transforms->size(); i++){
        physics::GpuObject object;
        
        object.transform = &sim_transforms->at(i);
        object.static_vertices = sim_static_vertices;
        
        sim_aabbs[i].extents *= utils::scaleFromTransform(*object.transform);
        float max_extent = std::max(std::max(sim_aabbs[i].extents.x, sim_aabbs[i].extents.y), sim_aabbs[i].extents.z);
        sim_aabbs[i].extents = glm::vec3(max_extent * 1.01f);
        sim_aabbs[i] = utils::updateAABB(sim_aabbs[i], object.transform);
        object.aabb = &sim_aabbs[i];

        sim_properties[i].velocity = glm::linearRand(glm::vec3(-0.05f), glm::vec3(0.05f));
        sim_properties[i].acceleration = glm::vec3(0.0f);
        sim_properties[i].angular_velocity = glm::linearRand(glm::vec3(-0.05f), glm::vec3(0.05f));
        sim_properties[i].angular_acceleration = glm::vec3(0.0f);
        
        
        object.physics = &sim_properties[i];
        sim_objects.push_back(object);
    }

    //COMPUTE SHADER
    m_shader.setShader("compute.glsl");
    m_shader.bind();

    //SSBOs
    m_transform_ssbo.setBuffer(sim_transforms->data(), sim_transforms->size() * sizeof(glm::mat4), GL_DYNAMIC_DRAW);
    m_transform_ssbo.unbind();

    m_properties_ssbo.setBuffer(sim_properties.data(), sim_properties.size() * sizeof(physics::Properties), GL_DYNAMIC_DRAW);
    m_properties_ssbo.unbind();

    m_aabbs_ssbo.setBuffer(sim_aabbs.data(), sim_aabbs.size() * sizeof(physics::AABB), GL_DYNAMIC_DRAW);
    m_aabbs_ssbo.unbind();

    // m_transform_ssbo.bindToBindingPoint(1);

    m_transform_ssbo.bindToBindingPoint(1);
    m_properties_ssbo.bindToBindingPoint(2);
    m_aabbs_ssbo.bindToBindingPoint(3);
}

GpuSimulator::~GpuSimulator(){
    sim_transforms = nullptr;
    sim_static_vertices = nullptr;
    sim_static_indices = nullptr;
    sim_objects.clear();
}

void GpuSimulator::update(float delta_time){

    m_shader.bind();
    m_shader.use();
    m_shader.setUniform1f("delta_time", delta_time);

    int work_gropus = (sim_transforms->size() + 256 - 1) / 256;
    m_shader.dispatch(work_gropus, 1, 1);
    m_shader.waitForCompletion(GL_SHADER_STORAGE_BARRIER_BIT);
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
