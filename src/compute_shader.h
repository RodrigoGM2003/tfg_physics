#ifndef COMPUTE_SHADER_H
#define COMPUTE_SHADER_H

#pragma once

#include <string>
#include <unordered_map>

#include "glm/glm.hpp"

/**
 * @brief A class to handle shaders
 */
class ComputeShader{
private:
    unsigned int m_renderer_id; //ComputeShader program id
    std::string m_file_path; //File path of the shader
    std::unordered_map<std::string, int> m_uniform_location_cache; //Cache for uniform locations
public:

    /**
     * @brief Default constructor
     */
    ComputeShader();

    /**
     * @brief Constructor
     * @param filename the name of the file containing the shader
     */
    ComputeShader(const std::string& filename);
    
    /**
     * @brief Destructor
     */
    ~ComputeShader();

    /**
     * @brief Binds the shader
     */
    void bind() const;

    /**
     * @brief Unbinds the shader
     */
    void unbind() const;

    /**
     * @brief Sets a shader
     * @param filename the name of the file containing the shader
     */
    void setComputeShader(const std::string &filename);

private:
    /**
     * @brief Reads a shader file
     * @param file The name of the file we want to read (it's name, not it's path)
     * @return ComputeShaderSource containing both the vertex and fragment sources
     */
    std::string readShader(const std::string& file);

    /**
     * @brief Compiles a shader and returns its id
     * @param type the type of the shader (GL_VERTEX_SHADER, GL_FRAGMENT_SHADER...)
     * @param source the string containing the code for the shader
     * @return the id of the shader
     */
    unsigned int compileShader(unsigned int type, const std::string& source);

    /**
     * @brief Creates a shader program
     * @param vertex_shader the vertex shader code
     * @param fragment_shader the fragment shader code
     * @return the id of the program
     */
    unsigned int createShader(const std::string& shader_src);

    /**
     * @brief Gets the location of a uniform
     * @param name the name of the uniform
     * @return the location of the uniform
     */
    int getUniformLocation(const std::string& name);
};


#endif //COMPUTE_SHADER_H