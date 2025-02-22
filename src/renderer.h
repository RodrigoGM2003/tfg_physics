#ifndef RENDERER_H
#define RENDERER_H

#pragma once

#include <cassert>
#include <GL/glew.h>

#include "buffers/vertex_array.h"
#include "buffers/index_buffer.h"
#include "shader.h"

#define ASSERT(x) if (!(x)) __debugbreak();
#define GLCall(x) GLClearError();\
    x;\
    ASSERT(GLLogCall(#x, __FILE__, __LINE__))
// #define GLCall(x) do { GLClearError();\
//     x;\
//     ASSERT(GLLogCall(#x, __FILE__, __LINE__));\
// } while(0)

/**
 * @brief Clears the OpenGL error buffer
 */
void GLClearError();

/**
 * @brief Logs an OpenGL call
 * @param function the function that was called
 * @param file the file where the function was called
 * @param line the line where the function was called
 * @return true if there was no error, false otherwise
 */
bool GLLogCall(const char* function, const char* file, int line);


class Renderer{
public:
    /**
     * @brief Clears the screen
     */
    void clear() const;

    /**
     * @brief Sets the clear color
     * @param r the red component
     * @param g the green component
     * @param b the blue component
     * @param a the alpha component
     */
    void setClearColor(float r, float g, float b, float a) const;
    
    /**
     * @brief Draws the vertex array
     * @param va the vertex array
     * @param ib the index buffer
     * @param shader the shader
     */
    void draw(const VertexArray& va, const IndexBuffer& ib, const Shader& shader) const;

        /**
     * @brief Draws the vertex array
     * @param va the vertex array
     * @param ib the index buffer
     * @param shader the shader
     */
    void instancedDraw(const VertexArray& va, const IndexBuffer& ib, const Shader& shader, unsigned int instances = 1) const;
};

#endif // RENDERER_H