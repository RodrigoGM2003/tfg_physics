#include "compute_shader.h"

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <filesystem>

#include "renderer.h"

ComputeShader::ComputeShader()
    : m_renderer_id(0), m_file_path(""){
}


ComputeShader::ComputeShader(const std::string &filename)
: m_renderer_id(0){
    //Get the path of the file
    m_file_path = __FILE__;
    m_file_path = m_file_path.substr(0, m_file_path.find_last_of("\\/"));
    m_file_path += "/../res/shaders/" + filename;

    //Read the shader file and create the shader
    const std::string source = this->readShader(m_file_path);
    m_renderer_id = this->createShader(source);

}

ComputeShader::~ComputeShader(){
    GLCall(glDeleteProgram(m_renderer_id));
}

void ComputeShader::bind() const{
    GLCall(glUseProgram(m_renderer_id));
}

void ComputeShader::unbind() const{
    GLCall(glUseProgram(0));
}

void ComputeShader::setComputeShader(const std::string &filename){
    //Delete the current shader if it exists
    if (m_renderer_id != 0)
        GLCall(glDeleteProgram(m_renderer_id));

    //Get the path of the file
    m_file_path = __FILE__;
    m_file_path = m_file_path.substr(0, m_file_path.find_last_of("\\/"));
    m_file_path += "/../res/shaders/" + filename;
    
    //Read the shader file and create the shader
    const std::string source = this->readShader(m_file_path);
    m_renderer_id = this->createShader(source);
}

std::string ComputeShader::readShader(const std::string& file){
    std::ifstream stream(file);

    //Check for error when opening file
    if (!stream.is_open()) {
        std::cerr << "Error: Could not open file " << file << std::endl;
        return "";
    }

    //Read source
    std::stringstream source;
    source << stream.rdbuf();
    return source.str();
}

unsigned int ComputeShader::compileShader(unsigned int type, const std::string &source){
    // Assign an id for the shader
    GLCall(unsigned int id = glCreateShader(type)); 
    const char* src = source.c_str();

    // Bind the id to the source code
    GLCall(glShaderSource(id, 1, &src, nullptr));

    // Compile the shader
    GLCall(glCompileShader(id));

    // Check for errors
    int result;
    GLCall(glGetShaderiv(id, GL_COMPILE_STATUS, &result));
    if (!result){
        int length;
        GLCall(glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length));

        char* message = (char*)alloca(length * sizeof(char));

        GLCall(glGetShaderInfoLog(id, length, &length, message));

        std::cerr << "Failed to compile " << (type == GL_VERTEX_SHADER ? "vertex" : "fragment") << " shader" << std::endl;
        std::cerr << message << std::endl;
    }

    return id;
}

unsigned int ComputeShader::createShader(const std::string &shader_src){
    // Create a program
    unsigned int program = glCreateProgram();
    // Create shaders (vertex and fragment)
    unsigned int shader = compileShader(GL_COMPUTE_SHADER, shader_src);
    
    //Attach shaders to the program, link and validate it
    GLCall(glAttachShader(program, shader));
    GLCall(glLinkProgram(program));
    GLCall(glValidateProgram(program));

    //Delete shaders as they are attached to the program
    GLCall(glDeleteShader(shader));

    return program;
}

int ComputeShader::getUniformLocation(const std::string &name){
    //Check if the uniform location is already in the cache and return it
    if (m_uniform_location_cache.find(name) != m_uniform_location_cache.end())
        return m_uniform_location_cache[name];

    //Get the location of the uniform
    GLCall(int location = glGetUniformLocation(m_renderer_id, name.c_str()));

    //Check for errors
    if (location == -1)
        std::cerr << "Warning: uniform '" << name << "' doesn't exist!" << std::endl;
    
    //Add the uniform location to the cache and return it
    m_uniform_location_cache[name] = location;
    return location;
}
