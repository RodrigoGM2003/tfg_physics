#include "instanced_mesh.h"
#include <iostream>

void InstancedMesh::updateModelMatrices(std::vector<glm::mat4> &model_matrices){
    m_model_matrices = model_matrices;
    m_ub.bind();
    m_ub.updateData(m_model_matrices.data(), m_model_matrices.size() * sizeof(glm::mat4));
    m_ub.unbind();
}

void InstancedMesh::setData(std::vector<Vertex> &vertices, std::vector<unsigned int> &indices, 
                            std::vector<glm::mat4> &model_matrices, unsigned int instances){

    m_vertices = vertices;
    m_indices = indices;
    m_model_matrices = model_matrices;
    m_instances = instances;

    this->setBuffers();
}

void InstancedMesh::setBuffers(){
    //Create vertex buffer
    VertexBuffer vb(m_vertices.data(), m_vertices.size() * sizeof(Vertex));
    VertexBufferLayout layout(false);
    layout.push<float>(3); //Position
    layout.push<float>(4); //Color
    layout.push<float>(3); //Normal
    m_va.addBuffer(vb, layout);

    m_ub.setBuffer(m_model_matrices.data(), m_model_matrices.size() * sizeof(glm::mat4), GL_DYNAMIC_DRAW);
    VertexBufferLayout u_layout(true);
    u_layout.push<float>(4); //Column 1
    u_layout.push<float>(4); //Column 2
    u_layout.push<float>(4); //Column 3
    u_layout.push<float>(4); //Column 4
    m_va.addBuffer(m_ub, u_layout);

    //Create index buffer
    m_ib.setBuffer(m_indices.data(), m_indices.size());

    //Unbind everything
    m_va.unbind();
    vb.unbind();
    m_ub.unbind();
    m_ib.unbind();
}
