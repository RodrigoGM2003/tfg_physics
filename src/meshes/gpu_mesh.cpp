#include "gpu_mesh.h"
#include <iostream>

void GpuMesh::setData(const std::vector<SimpleVertex> &vertices, const std::vector<unsigned int> &indices, 
                            const std::vector<glm::mat4> &model_matrices, const std::vector<glm::vec4> &model_colors,
                            unsigned int instances){

    m_vertices = &vertices;
    m_indices = &indices;
    m_model_matrices = &model_matrices;
    m_model_colors = &model_colors;
    m_instances = instances;

    this->setBuffers();
}


void GpuMesh::setBuffers(){
    //Create vertex buffer
    VertexBuffer vb(m_vertices->data(), m_vertices->size() * sizeof(Vertex));
    VertexBufferLayout layout(false);
    layout.push<float>(3); //Position
    layout.push<float>(3); //Normal
    m_va.addBuffer(vb, layout);

    //Create index buffer
    m_ib.setBuffer(m_indices->data(), m_indices->size());

    m_color_ssbo.setBuffer(m_model_colors->data(), m_model_colors->size() * sizeof(glm::vec4), GL_STATIC_DRAW);
    m_color_ssbo.bindToBindingPoint(10);

    //Unbind everything
    m_va.unbind();
    vb.unbind();
    m_ib.unbind();
    m_color_ssbo.unbind();
}
