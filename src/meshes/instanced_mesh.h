#ifndef INSTANCED_MESH_H
#define INSTANCED_MESH_H

#pragma once

#include <string>
#include <vector>

#include "glm/glm.hpp"

#include "../buffers/vertex_buffer.h"
#include "../buffers/index_buffer.h"
#include "../buffers/vertex_array.h"
#include "../buffers/vertex_buffer_layout.h"
#include "../shader.h"
#include "../camera.h"


class InstancedMesh{
private:
    unsigned int m_instances; // Number of instances

    std::vector<Vertex> m_vertices; //Vertex data
    std::vector<unsigned int> m_indices; //Vertex indices
    std::vector<glm::mat4> m_model_matrices; //Model matrices

    VertexArray m_va; //Vertex Array
    IndexBuffer m_ib; //Index Buffer
    VertexBuffer m_ub; //Uniform Buffer

public:
    /**
     * @brief Construct a new Instanced Mesh object
     */
    InstancedMesh(){};

    /**
     * @brief Update the model matrices
     * @param model_matrices the model matrices
     */
    void updateModelMatrices(std::vector<glm::mat4>& model_matrices);

    /**
     * @brief Get the vertex array
     */
    inline const VertexArray& getVertexArray() const { return m_va; }
    
    /**
     * @brief Get the index buffer
     */
    inline const IndexBuffer& getIndexBuffer() const { return m_ib; }
    
    /**
     * @brief Set the data
     * @param vertices the vertices
     * @param indices the indices
     * @param model_matrices the model matrices
     * @param instances the number of instances
     */
    void setData(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices, 
                std::vector<glm::mat4>& model_matrices, unsigned int instances);

private:
    /**
     * @brief Set the buffers
     */
    void setBuffers();
};



#endif // INSTANCED_MESH_H