#ifndef COMPUTE_SHADER_H
#define COMPUTE_SHADER_H

#pragma once

#include <string>
#include <unordered_map>

#include "glm/glm.hpp"

/**
 * @brief A class to handle compute shaders
 */
class ComputeShader {
private:
    unsigned int m_renderer_id; // Shader program id
    std::string m_file_path; // File path of the shader
    std::unordered_map<std::string, int> m_uniform_location_cache; // Cache for uniform locations

public:
    /**
     * @brief Default constructor
     */
    ComputeShader();

    /**
     * @brief Constructor
     * @param filename the name of the file containing the compute shader
     */
    ComputeShader(const std::string& filename);
    
    /**
     * @brief Destructor
     */
    ~ComputeShader();

    /**
     * @brief Binds the compute shader
     */
    void bind() const;

    /**
     * @brief Unbinds the compute shader
     */
    void unbind() const;

    /**
     * @brief Launches the compute shader with x*y*z working groups
     * @param num_groups_x Number of work groups in X dimension
     * @param num_groups_y Number of work groups in Y dimension
     * @param num_groups_z Number of work groups in Z dimension
     */
    void dispatch(unsigned int num_groups_x, unsigned int num_groups_y, unsigned int num_groups_z) const;

    /**
     * @brief Uses the shader
     */
    void use() const;

    /**
     * @brief Waits for compute shader to finish
     * @param barrier barrier to wait for
     */
    void waitForCompletion(unsigned int barrier) const;

    /**
     * @brief Sets a uniform of type int
     * @param name the name of the uniform
     * @param v0 the value of the uniform
     */
    void setUniform1i(const std::string& name, int v0);

    /**
     * @brief Sets a uniform of type float
     * @param name the name of the uniform
     * @param v0 the value of the uniform
     */
    void setUniform1f(const std::string& name, float v0);

    /**
     * @brief Sets a uniform of type float
     * @param name the name of the uniform
     * @param v0 the value of the uniform
     * @param v1 the value of the uniform
     */
    void setUniform2f(const std::string& name, float v0, float v1);

    /**
     * @brief Sets a uniform of type float
     * @param name the name of the uniform
     * @param v0 the value of the uniform
     * @param v1 the value of the uniform
     * @param v2 the value of the uniform
     */
    void setUniform3f(const std::string& name, float v0, float v1, float v2);

    /**
     * @brief Sets a uniform of type float
     * @param name the name of the uniform
     * @param v0 the value of the uniform
     */
    void setUniform4f(const std::string& name, float v0, float v1, float v2, float v3);

    /**
     * @brief Sets a uniform of type float
     * @param name the name of the uniform
     * @param vector the value of the uniform
     */
    void setUniformVec3f(const std::string& name, const glm::vec3& vector);
    
    /**
     * @brief Sets a uniform of type float
     * @param name the name of the uniform
     * @param vector the value of the uniform
     */
    void setUniformVec4f(const std::string& name, const glm::vec4& vector);

    /**
     * @brief Sets a uniform of type mat4
     * @param name the name of the uniform
     * @param matrix the value of the uniform
     */
    void setUniformMat4f(const std::string& name, const glm::mat4& matrix);

    /**
     * @brief Binds a shader storage buffer object to a binding point
     * @param ssbo The SSBO ID
     * @param binding The binding point index
     */
    void bindSSBO(unsigned int ssbo, unsigned int binding) const;

    /**
     * @brief Sets a compute shader
     * @param filename the name of the file containing the shader
     */
    void setShader(const std::string& filename);

private:
    /**
     * @brief Reads a compute shader file
     * @param file The name of the file we want to read (it's path)
     * @return std::string containing the compute shader source
     */
    std::string readComputeShader(const std::string& file);

    /**
     * @brief Compiles a compute shader and returns its id
     * @param source the string containing the code for the shader
     * @return the id of the shader
     */
    unsigned int compileComputeShader(const std::string& source);

    /**
     * @brief Creates a compute shader program
     * @param compute_shader the compute shader code
     * @return the id of the program
     */
    unsigned int createComputeShader(const std::string& compute_shader);

    /**
     * @brief Gets the location of a uniform
     * @param name the name of the uniform
     * @return the location of the uniform
     */
    int getUniformLocation(const std::string& name);
};

#endif