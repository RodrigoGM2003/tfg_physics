#include "simulator.h"

#include <iostream>

#include "glm/gtc/matrix_transform.hpp" 
#include "glm/gtx/transform.hpp"
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtc/random.hpp" 


Simulator::Simulator(
    std::vector<glm::mat4>* transforms, 
    const std::vector<Vertex>* static_vertices, 
    const std::vector<unsigned int>* static_indices
) : sim_transforms(transforms), sim_static_vertices(static_vertices), sim_static_indices(static_indices){
    //Create the objects
    sim_objects.reserve(sim_transforms->size());

    for (int i = 0; i < sim_transforms->size(); i++){
        SimObject object;

        object.transform = &sim_transforms->at(i);

        object.collision_vertices = *sim_static_vertices;

        object.physics.velocity = glm::linearRand(glm::vec3(-0.05f), glm::vec3(0.05f));
        object.physics.acceleration = glm::vec3(0.0f);
        object.physics.angular_velocity = glm::linearRand(glm::vec3(-0.05f), glm::vec3(0.05f));
        object.physics.angular_acceleration = glm::vec3(0.0f);

        sim_objects.push_back(object);
    }   
}

// Simulator::~Simulator(){
//     delete sim_transforms;
//     delete sim_static_vertices;
//     delete sim_static_indices;
// }

void Simulator::update(float delta_time){
    for (auto& object : sim_objects) {
        updateObject(object, delta_time);
    }

    // for (int i = 0; i < sim_objects.size(); i++){
    //     updateObject(sim_objects[i], deltaTime);
    // }
}

void Simulator::updateObject(SimObject& object, float delta_time){
    // Create rotation quaternion
    glm::quat rotation_quat = glm::quat(object.physics.angular_velocity * delta_time);
    
    // Extract current position and rotation from transform
    glm::vec3 position = glm::vec3((*object.transform)[3]);
    glm::mat3 rotation = glm::mat3(*object.transform);
    
    // Apply new rotation to current rotation
    glm::mat3 newRotation = glm::mat3_cast(rotation_quat) * rotation;
    
    // Apply velocity to position
    glm::vec3 newPosition = position + object.physics.velocity * delta_time;
    
    // Build new transform matrix
    glm::mat4 newTransform = glm::mat4(1.0f);
    
    // Set rotation part
    newTransform[0][0] = newRotation[0][0];
    newTransform[0][1] = newRotation[0][1];
    newTransform[0][2] = newRotation[0][2];
    
    newTransform[1][0] = newRotation[1][0];
    newTransform[1][1] = newRotation[1][1];
    newTransform[1][2] = newRotation[1][2];
    
    newTransform[2][0] = newRotation[2][0];
    newTransform[2][1] = newRotation[2][1];
    newTransform[2][2] = newRotation[2][2];
    
    // Set position part
    newTransform[3][0] = newPosition.x;
    newTransform[3][1] = newPosition.y;
    newTransform[3][2] = newPosition.z;
    
    // Update transform
    *object.transform = newTransform;

    // Update the transformed vertices for collision detection
    for(int i = 0; i < object.collision_vertices.size(); i++) {
        // Get original vertex from static data
        const auto& vert = (*sim_static_vertices)[i];

        // Transform the vertex position
        glm::vec4 original_pos = glm::vec4(
            vert.position[0],
            vert.position[1],
            vert.position[2],
            1.0f
        );

        // Apply transform and store in collision vertices
        glm::vec4 world_pos = newTransform * original_pos;
        object.collision_vertices[i].position[0] = world_pos.x;
        object.collision_vertices[i].position[1] = world_pos.y;
        object.collision_vertices[i].position[2] = world_pos.z;

    }


}

