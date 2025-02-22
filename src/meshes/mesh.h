#ifndef MESH_H
#define MESH_H

#pragma once

#include <string>
#include <vector>

#include "glm/glm.hpp"

#include "../buffers/vertex_buffer.h"
#include "../buffers/index_buffer.h"
#include "../buffers/vertex_array.h"
#include "../buffers/vertex_buffer_layout.h"
#include "../camera.h"


class Mesh{
private:
    std::vector<Vertex> m_vertices; //Vertex data
    std::vector<unsigned int> m_indices; //Vertex indices

    VertexArray m_va; //Vertex Array
    IndexBuffer m_ib; //Index Buffer

    glm::vec3 m_position; //Position
    glm::quat m_orientation; //Rotation
    glm::vec3 m_scale; //Scale

public:
    /**
     * @brief Construct a new Mesh object
     */
    Mesh();

    /**
     * @brief Get the Vertex Array object
     * @return const VertexArray& the vertex array
     */
    inline const VertexArray& getVertexArray() const { return m_va; }

    /**
     * @brief Get the Index Buffer object
     * @return const IndexBuffer& the index buffer
     */
    inline const IndexBuffer& getIndexBuffer() const { return m_ib; }

    /**
     * @brief Set the data of the mesh
     * @param vertices the vertices
     * @param indices the indices
     */
    void setData(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices);

    /**
     * @brief Get the position
     * @return glm::vec3& the position
     */
    inline glm::vec3& getPosition() { return m_position; }
    
    /**
     * @brief Set the position
     * @param position the position
     */
    inline void setPosition(glm::vec3& position) { m_position = position; }

    /**
     * @brief Get the orientation
     * @return glm::quat& the orientation
     */
    inline glm::quat& getOrientation() { return m_orientation; }

    /**
     * @brief Set the orientation
     * @param orientation the orientation
     */
    inline void setOrientation(glm::quat& orientation) { m_orientation = orientation; }

    /**
     * @brief Get the scale
     * @return glm::vec3& the scale
     */
    inline glm::vec3& getScale() { return m_scale; }

    /**
     * @brief Set the scale
     * @param scale the scale
     */
    inline void setScale(glm::vec3& scale) { m_scale = scale; }

    /**
     * @brief Get the model matrix
     * @return glm::mat4 the model matrix
     * @note Model matrix is computed every time this function is called
     */
    glm::mat4 getModelMatrix() const;

private:
    /**
     * @brief Set the buffers
     */
    void setBuffers();
};



#endif // MESH_H