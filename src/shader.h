#ifndef SHADER_H
#define SHADER_H

#pragma once

#include <string>
#include <unordered_map>

#include "glm/glm.hpp"

struct ShaderSource{
    std::string vertex_source;
    std::string fragment_source;
};

/**
 * @brief A class to handle shaders
 */
class Shader{
private:
    unsigned int m_renderer_id; //Shader program id
    std::string m_file_path; //File path of the shader
    std::unordered_map<std::string, int> m_uniform_location_cache; //Cache for uniform locations
public:

    /**
     * @brief Default constructor
     */
    Shader();

    /**
     * @brief Constructor
     * @param filename the name of the file containing the shader
     */
    Shader(const std::string& filename);
    
    /**
     * @brief Destructor
     */
    ~Shader();

    /**
     * @brief Binds the shader
     */
    void bind() const;

    /**
     * @brief Unbinds the shader
     */
    void unbind() const;

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
     * @brief Sets a point light
     * @param name the name of the light
     * @param position the position of the light
     * @param color the color of the light
     * @param intensity the intensity of the light
     * @param constant the constant of the light
     * @param linear the linear of the light
     * @param quadratic the quadratic of the light
     */
    void setUniformPointLight(const std::string& name, 
        const glm::vec3& position, 
        const glm::vec3& color, 
        const float& intensity, 
        const float& constant, 
        const float& linear, 
        const float& quadratic
    );

    /**
     * @brief Sets a shader
     * @param filename the name of the file containing the shader
     */
    void setShader(const std::string &filename);

private:
    /**
     * @brief Reads a shader file
     * @param file The name of the file we want to read (it's name, not it's path)
     * @return ShaderSource containing both the vertex and fragment sources
     */
    ShaderSource readShader(const std::string& file);

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
    unsigned int createShader(const std::string& vertex_shader, const std::string& fragment_shader);

    /**
     * @brief Gets the location of a uniform
     * @param name the name of the uniform
     * @return the location of the uniform
     */
    int getUniformLocation(const std::string& name);
};


#endif