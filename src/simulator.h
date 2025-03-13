#ifndef SIMULATOR_H
#define SIMULATOR_H

#pragma once

#include <vector>

#include "glm/glm.hpp"
#include "vertex_buffer.h"

struct PhysicsProperties{
    glm::vec3 velocity;
    glm::vec3 acceleration;

    glm::vec3 angular_velocity;
    glm::vec3 angular_acceleration;
};


struct SimObject{
    glm::mat4* transform;
    
    std::vector<Vertex> collision_vertices;
    
    PhysicsProperties physics;
};

class Simulator{
private:
    std::vector<glm::mat4>* sim_transforms; 
    const std::vector<Vertex>* sim_static_vertices; //Vertex data
    const std::vector<unsigned int>* sim_static_indices; //Vertex indices

    std::vector<SimObject> sim_objects;
public:
    Simulator(std::vector<glm::mat4>* transforms, const std::vector<Vertex>* static_vertices, const std::vector<unsigned int>* static_indices);
    ~Simulator();

    void update(float delta_time);
    void updateObject(SimObject& object, float delta_time);
};




#endif // SIMULATOR_H