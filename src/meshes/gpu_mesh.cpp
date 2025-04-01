#include "gpu_mesh.h"
#include <iostream>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/glm.hpp>

void GpuMesh::setData(const std::vector<SimpleVertex> &vertices, const std::vector<unsigned int> &indices, 
                       const std::vector<glm::mat4> &model_matrices, const std::vector<glm::vec4> &model_colors,
                       unsigned int instances){

    m_vertices = &vertices;
    m_indices = &indices;
    m_model_matrices = &model_matrices;
    m_model_colors = &model_colors;
    m_instances = instances;

    // Generate texture coordinates
    generateTextureCoordinates();
    
    this->setBuffers();
}

void GpuMesh::generateTextureCoordinates() {
    // Create texture coordinates for an icosahedron
    // This is a simple approach where we generate UV coordinates based on vertex positions
    m_texCoords.clear();
    m_texCoords.reserve(m_vertices->size());
    
    for (const auto& vertex : *m_vertices) {
        // Normalize the position to get a direction vector
        glm::vec3 position = glm::vec3(vertex.position[0], vertex.position[1], vertex.position[2]);
        glm::vec3 dir = glm::normalize(position);
        
        // Map spherical coordinates to UV
        // This is a simple spherical mapping - could be replaced with better methods
        float u = 0.5f + (atan2(dir.z, dir.x) / (2.0f * 3.14159f));
        float v = 0.5f - (asin(dir.y) / 3.14159f);
        
        m_texCoords.push_back(glm::vec2(u, v));
    }
}

void GpuMesh::setBuffers(){
    // Create vertex buffer
    VertexBuffer vb(m_vertices->data(), m_vertices->size() * sizeof(SimpleVertex));
    VertexBufferLayout layout(false);
    layout.push<float>(3); // Position
    layout.push<float>(3); // Normal
    m_va.addBuffer(vb, layout);

    // Create index buffer
    m_ib.setBuffer(m_indices->data(), m_indices->size());

    // Create texcoord SSBO
    m_texcoord_ssbo.setBuffer(m_texCoords.data(), m_texCoords.size() * sizeof(glm::vec2), GL_STATIC_DRAW);
    m_texcoord_ssbo.bindToBindingPoint(11); // Using binding point 5 for texture coordinates

    // Color SSBO
    m_color_ssbo.setBuffer(m_model_colors->data(), m_model_colors->size() * sizeof(glm::vec4), GL_STATIC_DRAW);
    m_color_ssbo.bindToBindingPoint(10);

    // Unbind everything
    m_va.unbind();
    vb.unbind();
    m_ib.unbind();
    m_texcoord_ssbo.unbind();
    m_color_ssbo.unbind();
}