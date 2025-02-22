#include "shader.h"

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <filesystem>

#include "renderer.h"

Shader::Shader()
    : m_renderer_id(0), m_file_path(""){
}


Shader::Shader(const std::string &filename)
: m_renderer_id(0){
    //Get the path of the file
    m_file_path = __FILE__;
    m_file_path = m_file_path.substr(0, m_file_path.find_last_of("\\/"));
    m_file_path += "/../res/shaders/" + filename;

    //Read the shader file and create the shader
    ShaderSource source = this->readShader(m_file_path);
    m_renderer_id = this->createShader(source.vertex_source, source.fragment_source);

}

Shader::~Shader(){
    GLCall(glDeleteProgram(m_renderer_id));
}

void Shader::bind() const{
    GLCall(glUseProgram(m_renderer_id));
}

void Shader::unbind() const{
    GLCall(glUseProgram(0));
}

void Shader::setUniform1f(const std::string &name, float v0){
    GLCall(glUniform1f(getUniformLocation(name), v0));
}
void Shader::setUniform2f(const std::string &name, float v0, float v1){
    GLCall(glUniform2f(getUniformLocation(name), v0, v1));
}
void Shader::setUniform3f(const std::string &name, float v0, float v1, float v2){
    GLCall(glUniform3f(getUniformLocation(name), v0, v1, v2));
}
void Shader::setUniform4f(const std::string &name, float v0, float v1, float v2, float v3){
    GLCall(glUniform4f(getUniformLocation(name), v0, v1, v2, v3));
}

void Shader::setUniformVec3f(const std::string &name, const glm::vec3 &vector){
    GLCall(glUniform3f(getUniformLocation(name), vector.x, vector.y, vector.z));
}

void Shader::setUniformVec4f(const std::string &name, const glm::vec4 &vector){
    GLCall(glUniform4f(getUniformLocation(name), vector.x, vector.y, vector.z, vector.w));
}

void Shader::setUniformMat4f(const std::string &name, const glm::mat4 &matrix){
    GLCall(glUniformMatrix4fv(getUniformLocation(name), 1, GL_FALSE, &matrix[0][0]));
}

void Shader::setUniformPointLight(const std::string& name, 
    const glm::vec3& position, 
    const glm::vec3& color, 
    const float& intensity, 
    const float& constant, 
    const float& linear, 
    const float& quadratic
){
    setUniformVec3f(name + ".position", position);
    setUniformVec3f(name + ".color", color);
    setUniform1f(name + ".intensity", intensity);
    setUniform1f(name + ".constant", constant);
    setUniform1f(name + ".linear", linear);
    setUniform1f(name + ".quadratic", quadratic);
}

void Shader::setShader(const std::string &filename){
    //Delete the current shader if it exists
    if (m_renderer_id != 0)
        GLCall(glDeleteProgram(m_renderer_id));

    //Get the path of the file
    m_file_path = __FILE__;
    m_file_path = m_file_path.substr(0, m_file_path.find_last_of("\\/"));
    m_file_path += "/../res/shaders/" + filename;
    
    //Read the shader file and create the shader
    ShaderSource source = this->readShader(m_file_path);
    m_renderer_id = this->createShader(source.vertex_source, source.fragment_source);
}

ShaderSource Shader::readShader(const std::string& file){
    std::ifstream stream(file);

    //Check for error when opening file
    if (!stream.is_open()) {
        std::cerr << "Error: Could not open file " << file << std::endl;
        return {"", ""};
    }

    enum class ShaderType{
        NONE = -1, VERTEX = 0, FRAGMENT = 1
    };

    //Read the file line by line and separate the vertex and fragment shaders
    std::string line;
    std::stringstream ss[2];
    ShaderType type = ShaderType::NONE;
    while(getline(stream, line)){
        //Check for the shader type
        if (line.find("#shader") != std::string::npos){
            if (line.find("vertex") != std::string::npos)
                type = ShaderType::VERTEX;
            
            else if (line.find("fragment") != std::string::npos)
                type = ShaderType::FRAGMENT;
        }
        else
            ss[(int)type] << line << '\n';
        
    }

    //Return the vertex and fragment shaders
    return {ss[0].str(), ss[1].str()};
}

unsigned int Shader::compileShader(unsigned int type, const std::string &source){
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

unsigned int Shader::createShader(const std::string &vertex_shader, const std::string &fragment_shader){
    // Create a program
    unsigned int program = glCreateProgram();
    // Create shaders (vertex and fragment)
    unsigned int vs = compileShader(GL_VERTEX_SHADER, vertex_shader);
    unsigned int fs = compileShader(GL_FRAGMENT_SHADER, fragment_shader);
    
    //Attach shaders to the program, link and validate it
    GLCall(glAttachShader(program, vs));
    GLCall(glAttachShader(program, fs));
    GLCall(glLinkProgram(program));
    GLCall(glValidateProgram(program));

    //Delete shaders as they are attached to the program
    GLCall(glDeleteShader(vs));

    return program;
}

int Shader::getUniformLocation(const std::string &name){
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
