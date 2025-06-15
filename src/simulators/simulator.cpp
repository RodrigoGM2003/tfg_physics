#include "simulator.h"

#include <iostream>

#include "glm/gtc/matrix_transform.hpp" 
#include "glm/gtx/transform.hpp"
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtc/random.hpp" 
#include "glm/gtx/component_wise.hpp"


Simulator::Simulator(
    std::vector<glm::mat4>* transforms, 
    const std::vector<Vertex>* static_vertices, 
    const std::vector<unsigned int>* static_indices
) : sim_transforms(transforms), 
    sim_static_vertices(static_vertices), 
    sim_static_indices(static_indices){
    //Create the objects
    sim_objects.reserve(sim_transforms->size());

    physics::AABB base_aabb = utils::calculateAABB(*sim_static_vertices);

    for (int i = 0; i < sim_transforms->size(); i++){
        physics::Object object;

        object.transform = &sim_transforms->at(i);
        object.static_vertices = sim_static_vertices;

        object.aabb = base_aabb;
        object.aabb.extents *= utils::scaleFromTransform(*object.transform);
        float max_extent = std::max(std::max(object.aabb.extents.x, object.aabb.extents.y), object.aabb.extents.z);
        object.aabb.extents = glm::vec3(max_extent * 1.01f);
        object.aabb = utils::updateAABB(object.aabb, object.transform);

        object.physics.velocity = glm::linearRand(glm::vec3(-0.05f), glm::vec3(0.05f));
        object.physics.acceleration = glm::vec3(0.0f);
        object.physics.angular_velocity = glm::linearRand(glm::vec3(-0.05f), glm::vec3(0.05f));
        object.physics.angular_acceleration = glm::vec3(0.0f);

        
        sim_objects.push_back(object);
    }   
}

Simulator::~Simulator(){
    sim_transforms = nullptr;
    sim_static_vertices = nullptr;
    sim_static_indices = nullptr;
    sim_objects.clear();
}

void Simulator::update(float delta_time, glm::vec3 gravity){
    for (auto& object : sim_objects) {
        updateObject(object, delta_time);
    }
}

void Simulator::updateObject(physics::Object& object, float delta_time){
    // Create rotation quaternion
    glm::quat rotation_quat = glm::quat(object.physics.angular_velocity * delta_time);
    
    // Extract current position and rotation from transform
    glm::vec3 position = glm::vec3((*object.transform)[3]);
    glm::mat3 rotation = glm::mat3(*object.transform);
    
    // Apply new rotation to current rotation
    glm::mat3 new_rotation = glm::mat3_cast(rotation_quat) * rotation;
    
    // Apply velocity to position
    glm::vec3 new_position = position + object.physics.velocity * delta_time;
    
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

    object.aabb = utils::updateAABB(object.aabb, object.transform);
}
