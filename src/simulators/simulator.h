#ifndef SIMULATOR_H
#define SIMULATOR_H

#pragma once

#include <vector>

#include "glm/glm.hpp"
#include "vertex_buffer.h"

#include "simulators/simulable.h"
#include "utils.h"

/**
 * @brief class representation of a simulator running on the cpu
 */
class Simulator : public Simulable{
private:
    std::vector<glm::mat4>* sim_transforms; /* Transform matrices for each object */
    const std::vector<Vertex>* sim_static_vertices; //Vertex data
    const std::vector<unsigned int>* sim_static_indices; //Vertex indices

    std::vector<physics::Object> sim_objects; /* Vector containing each object in the simulation */
public:
    /**
     * @brief Constructor
     * @param transforms pointer to the transform matrix of the objects
     * @param static_vertices pointer to the original vertices of the geometry
     * @param static_indices pointer to the order in which each triangle is being drawn
     */
    Simulator(std::vector<glm::mat4>* transforms, const std::vector<Vertex>* static_vertices, const std::vector<unsigned int>* static_indices);
    
    /**
     * @brief class destroyer. The memory is not dereferenced.
     */
    ~Simulator();

    /**
     * @brief take a step on the simulation accounting for delta_time seconds
     */
    void update(float delta_time) override;

private:
    /**
     * @brief take a step for each object
     */
    void updateObject(physics::Object& object, float delta_time);
};




#endif // SIMULATOR_H