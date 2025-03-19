#include "gpu_mesh.h"
#include <iostream>


void GpuMesh::setData(const std::vector<Vertex> &vertices, const std::vector<unsigned int> &indices, 
                            const std::vector<glm::mat4> &model_matrices, unsigned int instances){

    m_vertices = &vertices;
    m_indices = &indices;
    m_model_matrices = &model_matrices;
    m_instances = instances;

    this->setBuffers();
}

void GpuMesh::setBuffers(){
    //Create vertex buffer
    VertexBuffer vb(m_vertices->data(), m_vertices->size() * sizeof(Vertex));
    VertexBufferLayout layout(false);
    layout.push<float>(3); //Position
    layout.push<float>(4); //Color
    layout.push<float>(3); //Normal
    m_va.addBuffer(vb, layout);

    //Create index buffer
    m_ib.setBuffer(m_indices->data(), m_indices->size());

    //Unbind everything
    m_va.unbind();
    vb.unbind();
    m_ib.unbind();
}
