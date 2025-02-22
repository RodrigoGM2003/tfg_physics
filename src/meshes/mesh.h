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
#include "../shader.h"
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
    Mesh();

    Mesh(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices);

    void draw(Shader& shader, Camera& camera);

    inline const VertexArray& getVertexArray() const { return m_va; }
    inline const IndexBuffer& getIndexBuffer() const { return m_ib; }

    void setData(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices);

    inline glm::vec3& getPosition() { return m_position; }
    inline void setPosition(glm::vec3& position) { m_position = position; }

    inline glm::quat& getOrientation() { return m_orientation; }
    inline void setOrientation(glm::quat& orientation) { m_orientation = orientation; }

private:
    void setBuffers();
};



#endif // MESH_H