#ifndef VERTEX_BUFFER_LAYOUT_H
#define VERTEX_BUFFER_LAYOUT_H

#pragma once

#include <vector>
#include "../renderer.h"

/**
 * @brief A struct to store the elements of a vertex buffer
 * @param count the number of elements
 * @param type the type of the elements
 * @param normalized if the elements are normalized
 */
struct VertexBufferElement{
    unsigned int type; //Type of the elements
    unsigned int count; //Number of elements
    unsigned char normalized; //Normalized

    static unsigned int getSizeOfType(unsigned int type){
        switch (type){
            case GL_FLOAT: return sizeof(GLfloat);
            case GL_UNSIGNED_INT: return sizeof(GLuint);
            case GL_UNSIGNED_BYTE: return sizeof(GLubyte);
        }
        ASSERT(false);
        return 0;
    }
};

/**
 * @brief A class to store the layout of a vertex buffer
 */
class VertexBufferLayout{
private:
    std::vector<VertexBufferElement> m_elements; //Elements
    unsigned int m_stride; //Stride
    bool m_is_instanced = false;
public:
    /**
     * @brief Creates a vertex buffer layout
     * @param is_instanced if the layout is instanced
     */
    VertexBufferLayout(bool is_instanced = false)
        : m_stride(0), m_is_instanced(is_instanced) {}

    /**
     * @brief Pushes an element into the buffer
     * @param count the number of elements
     */
    template<typename T>
    void push(unsigned int count){
        static_assert(false);
    }

    /**
     * @brief Pushes a float into the buffer
     * @param count the number of floats
     */
    template<>
    void push<float>(unsigned int count){
        m_elements.push_back({GL_FLOAT, count, GL_FALSE});
        m_stride += VertexBufferElement::getSizeOfType(GL_FLOAT) * count;
    }

    /**
     * @brief Pushes an unsigned int into the buffer
     * @param count the number of unsigned ints
     */
    template<>
    void push<unsigned int>(unsigned int count){
        m_elements.push_back({GL_UNSIGNED_INT, count, GL_FALSE});
        m_stride += VertexBufferElement::getSizeOfType(GL_UNSIGNED_INT) * count;
    }

    /**
     * @brief Pushes an unsigned char into the buffer
     * @param count the number of unsigned chars
     */
    template<>
    void push<unsigned char>(unsigned int count){
        m_elements.push_back({GL_UNSIGNED_BYTE, count, GL_TRUE});
        m_stride += VertexBufferElement::getSizeOfType(GL_UNSIGNED_BYTE) * count;
    }

    /**
     * @brief Returns the elements
     * @return the elements
     */
    inline const std::vector<VertexBufferElement>& getElements() const { return m_elements; }
    
    /**
     * @brief Returns the stride
     */
    inline unsigned int getStride() const { return m_stride; }

    /**
     * @brief Returns if the layout is instanced
     */
    inline bool isInstanced() const { return m_is_instanced; }
};


#endif //VERTEX_BUFFER_LAYOUT_H