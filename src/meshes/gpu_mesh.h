#ifndef GPU_MESH_H
#define GPU_MESH_H

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
#include "buffers/shader_storage_buffer.h"

class GpuMesh{
protected:
    unsigned int m_instances; // Number of instances

    const std::vector<SimpleVertex>* m_vertices; //Vertex data
    const std::vector<unsigned int>* m_indices; //Vertex indices

    const std::vector<glm::mat4>* m_model_matrices; 
    const std::vector<glm::vec4>* m_model_colors; 

    VertexArray m_va; //Vertex Array
    IndexBuffer m_ib; //Index Buffer
    ShaderStorageBuffer m_color_ssbo;

public:
    /**
     * @brief Construct a new Instanced Mesh object
     */
    GpuMesh(){};

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
    void setData(const std::vector<SimpleVertex>& vertices, const std::vector<unsigned int>& indices, 
                const std::vector<glm::mat4>& model_matrices, const std::vector<glm::vec4>& model_colors,
                unsigned int instances);

private:
    /**
     * @brief Set the buffers
     */
    void setBuffers();
};



#endif // GPU_MESH_H