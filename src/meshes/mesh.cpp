#include "mesh.h"
#include <iostream>

Mesh::Mesh(): 
    m_position(glm::vec3(0.0f)), 
    m_orientation(glm::quat(1.0f, 0.0f, 0.0f, 0.0f)), 
    m_scale(glm::vec3(1.0f))
{}

void Mesh::setData(std::vector<Vertex> &vertices, std::vector<unsigned int> &indices){
    m_vertices = vertices;
    m_indices = indices;

    this->setBuffers();
}

glm::mat4 Mesh::getModelMatrix() const{
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, m_position);
    model *= glm::toMat4(m_orientation);
    model = glm::scale(model, m_scale);

    return model;
}


void Mesh::setBuffers(){
    //Create vertex buffer
    VertexBuffer vb(m_vertices.data(), m_vertices.size() * sizeof(Vertex));

    // Define layout and bind buffers
    VertexBufferLayout layout;
    layout.push<float>(3); //Position
    layout.push<float>(4); //Color
    layout.push<float>(3); //Normal
    m_va.addBuffer(vb, layout);

    //Create index buffer
    m_ib.setBuffer(m_indices.data(), m_indices.size());

    //Unbind everything
    m_va.unbind();
    vb.unbind();
    m_ib.unbind();
}