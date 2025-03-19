#ifndef RENDERER_H
#define RENDERER_H

#pragma once

#ifndef ASSERT
#define ASSERT(x) if (!(x)) __debugbreak();
#endif

#ifndef GLCall
    #define GLCall(x) GLClearError();\
    x;\
    ASSERT(GLLogCall(#x, __FILE__, __LINE__))
#endif


#include <cassert>
#include <GL/glew.h>

#include "camera.h"
#include "meshes/mesh.h"
#include "meshes/instanced_mesh.h"

// Forward declarations
class Mesh;
class InstancedMesh;
class Shader;
class Camera;


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
     * @param mesh the mesh
     * @param shader the shader
     * @param camera the camera
     */
    void draw(const Mesh& mesh, Shader& shader, Camera& camera);

    /**
     * @brief Draws the vertex array
     * @param va the vertex array
     * @param ib the index buffer
     * @param shader the shader
     */
    void instancedDraw(const InstancedMesh& instanced_mesh, Shader& shader, Camera& camera, unsigned int instances = 1);
    void instancedDraw(const VertexArray& vertex_array, const IndexBuffer& index_buffer, Shader& shader, Camera& camera, unsigned int instances = 1);

};

#endif // RENDERER_H