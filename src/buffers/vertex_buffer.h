#ifndef VERTEX_BUFFER_H
#define VERTEX_BUFFER_H

#pragma once

#include <glm/glm.hpp>
#include <GL/glew.h>

struct Vertex{
public:
    glm::vec3 position;
    glm::vec4 color;
    glm::vec3 normal;
};
    

class VertexBuffer{
private:
    unsigned int m_renderer_id; //ID of the buffer
    unsigned int m_usage; //Usage of the buffer

public:
    VertexBuffer() : m_renderer_id(0) {};

    /**
     * @brief Creates a vertex buffer
     * @param data the data to be stored in the buffer
     * @param size the size of the data
     * @param usage the usage of the buffer
     */
    VertexBuffer(const void* data, unsigned int size, const unsigned int usage = GL_STATIC_DRAW);
    
    /**
     * @brief Destroys the vertex buffer
     */
    ~VertexBuffer();

    /**
     * @brief Binds the buffer
     */
    void bind() const;

    /**
     * @brief Unbinds the buffer
     */
    void unbind() const;

    /**
     * @brief Updates the buffer data
     * @param data the data to be stored in the buffer
     * @param size the size of the data
     * @param offset the offset in the buffer where the data should be stored
     */
    void updateData(const void* data, unsigned int size, unsigned int offset = 0);

    /**
     * @brief Sets the buffer data
     * @param data the data to be stored in the buffer
     * @param size the size of the data
     * @param usage the usage of the buffer
     */
    void setBuffer(const void* data, unsigned int size, const unsigned int usage = GL_STATIC_DRAW);
};

#endif //VERTEX_BUFFER_H