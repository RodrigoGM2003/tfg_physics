#ifndef SHADER_STORAGE_BUFFER_H
#define SHADER_STORAGE_BUFFER_H

#pragma once

#include <glm/glm.hpp>
#include <GL/glew.h>
    
#include <unordered_set>

class ShaderStorageBuffer{
private:
    static std::unordered_set<unsigned int> taken_binding_points;

    unsigned int m_renderer_id; //ID of the buffer
    unsigned int m_usage; //Usage of the buffer
    unsigned int m_binding_point;

public:
    ShaderStorageBuffer() : m_renderer_id(0), m_binding_point(0) {};

    /**
     * @brief Creates a vertex buffer
     * @param data the data to be stored in the buffer
     * @param size the size of the data
     * @param usage the usage of the buffer
     */
    ShaderStorageBuffer(const void* data, unsigned int size, const unsigned int usage = GL_STATIC_DRAW);
    
    /**
     * @brief Destroys the vertex buffer
     */
    ~ShaderStorageBuffer();

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

    /**
     * @brief Binds the SSBO to a binding point
     * @param binding_point The point to which the SSBO will bet binded
     * @return true if binding was succesfull, false if binding_point whas already taken
     */
    bool bindToBindingPoint(unsigned int binding_point);

    /**
     * @brief Unbinds the SSBO from its binding point
     */
    void unbindFromBindingPoint();

    /**
     * @brief Gets the binding point
     * @return The current binding point (0 if not binded)
     */
    inline unsigned int getBindingPoint() {return m_binding_point;};

    /**
     * @brief Reads the data from the buffer in gpu memory
     * @return pointer to the data buffer
     * @note The buffer must be binded before reading the data (buffer.bind())
     */
    void* readData();

    /**
     * @brief Unmaps the buffer (invalidates the readData pointer)
     * @note This destroys the pointer created by readData, so the memory should be copied befoe calling unmap
     */
    void unmapBuffer();

};

#endif //SHADER_STORAGE_BUFFER_H