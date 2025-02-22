#ifndef VERTEX_ARRAY_H
#define VERTEX_ARRAY_H

#pragma once

#include "vertex_buffer.h"

class VertexBufferLayout;

/**
 * @brief A class that represents a vertex array
 */
class VertexArray{
private:
    unsigned int m_renderer_id;
    unsigned int m_current_index = 0;

public:
    /**
     * @brief Constructor
     */
    VertexArray();

    /**
     * @brief Destructor
     */
    ~VertexArray();

    /**
     * @brief Adds a buffer to the vertex array
     * @param vb the vertex buffer
     * @param layout the vertex buffer layout
     */
    void addBuffer(const VertexBuffer& vb, const VertexBufferLayout& layout);

    /**
     * @brief Binds the vertex array
     */
    void bind() const;

    /**
     * @brief Unbinds the vertex array
     */
    void unbind() const;

};

#endif //VERTEX_ARRAY_H