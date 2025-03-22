#include "compute_shader.h"

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <filesystem>

#include "renderer.h" // Assuming this includes your OpenGL calls and GLCall macro

ComputeShader::ComputeShader()
    : m_renderer_id(0), m_file_path(""), m_timer(false) {
}

ComputeShader::ComputeShader(const std::string& filename)
    : m_renderer_id(0), m_timer(false) {
    // Get the path of the file
    m_file_path = __FILE__;
    m_file_path = m_file_path.substr(0, m_file_path.find_last_of("\\/"));
    m_file_path += "/../res/shaders/" + filename;

    // Read the shader file and create the shader
    std::string source = this->readComputeShader(m_file_path);
    m_renderer_id = this->createComputeShader(source);

    this->m_timer = time;
}

ComputeShader::~ComputeShader() {
    if(m_timer)
        GLCall(glDeleteQueries(2, m_timer_queries))
    
    GLCall(glDeleteProgram(m_renderer_id));
}

void ComputeShader::bind() const {
    GLCall(glUseProgram(m_renderer_id));
}

void ComputeShader::unbind() const {
    GLCall(glUseProgram(0));
}

void ComputeShader::dispatch(unsigned int num_groups_x, unsigned int num_groups_y, unsigned int num_groups_z) const {
    if (m_timer){
        GLCall(glBeginQuery(GL_TIME_ELAPSED, m_timer_queries[0]));
    }
    GLCall(glDispatchCompute(num_groups_x, num_groups_y, num_groups_z));
}

void ComputeShader::use() const {
    GLCall(glUseProgram(m_renderer_id));
}

void ComputeShader::useTimer(bool time){
    if(!time && m_timer){
        GLCall(glDeleteQueries(2, m_timer_queries));
    }
    else if (time){
        m_timer = time;
        GLCall(glGenQueries(2, m_timer_queries));
    } 
}

unsigned int ComputeShader::waitForCompletion(unsigned int barrier) const {
    GLCall(glMemoryBarrier(barrier));

    unsigned long long time_elapsed = 0;

    if(m_timer){
        GLCall(glEndQuery(GL_TIME_ELAPSED));
        GLCall(glGetQueryObjectui64v(m_timer_queries[0], GL_QUERY_RESULT, &time_elapsed));
        return static_cast<unsigned int>(time_elapsed);
    }

    return 0;
}

void ComputeShader::setUniform1i(const std::string& name, int v0) {
    GLCall(glUniform1i(getUniformLocation(name), v0));
}

void ComputeShader::setUniform1f(const std::string& name, float v0) {
    GLCall(glUniform1f(getUniformLocation(name), v0));
}

void ComputeShader::setUniform2f(const std::string& name, float v0, float v1) {
    GLCall(glUniform2f(getUniformLocation(name), v0, v1));
}

void ComputeShader::setUniform3f(const std::string& name, float v0, float v1, float v2) {
    GLCall(glUniform3f(getUniformLocation(name), v0, v1, v2));
}

void ComputeShader::setUniform4f(const std::string& name, float v0, float v1, float v2, float v3) {
    GLCall(glUniform4f(getUniformLocation(name), v0, v1, v2, v3));
}

void ComputeShader::setUniformVec3f(const std::string& name, const glm::vec3& vector) {
    GLCall(glUniform3f(getUniformLocation(name), vector.x, vector.y, vector.z));
}

void ComputeShader::setUniformVec4f(const std::string& name, const glm::vec4& vector) {
    GLCall(glUniform4f(getUniformLocation(name), vector.x, vector.y, vector.z, vector.w));
}

void ComputeShader::setUniformMat4f(const std::string& name, const glm::mat4& matrix) {
    GLCall(glUniformMatrix4fv(getUniformLocation(name), 1, GL_FALSE, &matrix[0][0]));
}

void ComputeShader::bindSSBO(unsigned int ssbo, unsigned int binding) const {
    GLCall(glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding, ssbo));
}

void ComputeShader::setShader(const std::string& filename) {
    // Delete the current shader if it exists
    if (m_renderer_id != 0)
        GLCall(glDeleteProgram(m_renderer_id));

    // Get the path of the file
    m_file_path = __FILE__;
    m_file_path = m_file_path.substr(0, m_file_path.find_last_of("\\/"));
    m_file_path += "/../res/shaders/compute/" + filename;
    
    // Read the shader file and create the shader
    std::string source = this->readComputeShader(m_file_path);
    m_renderer_id = this->createComputeShader(source);
}

// std::string ComputeShader::readComputeShader(const std::string& file) {
//     std::ifstream stream(file);

//     // Check for error when opening file
//     if (!stream.is_open()) {
//         std::cerr << "Error: Could not open file " << file << std::endl;
//         return "";
//     }

//     std::string line;
//     std::stringstream ss;
//     std::unordered_set<std::string> includes;

//     // Read the file line by line
//     while (getline(stream, line)) {
//         ss << line << '\n';
//     }

//     return ss.str();
// }
std::string ComputeShader::readComputeShader(const std::string& file) {
    std::unordered_set<std::string> includedFiles;
    return this->processShaderIncludes(file, includedFiles);
}

std::string ComputeShader::processShaderIncludes(const std::string& file, std::unordered_set<std::string>& includedFiles) {
    if (includedFiles.find(file) != includedFiles.end()) {
        return "";
    }
    includedFiles.insert(file);

    std::ifstream stream(file);
    if (!stream.is_open()) {
        std::cerr << "Error: Could not open file " << file << std::endl;
        return "";
    }

    std::stringstream shaderCode;
    std::string line;
    std::filesystem::path directory = std::filesystem::path(file).parent_path(); // Get the directory

    while (getline(stream, line)) {
        if (line.find("#include") != std::string::npos) {
            size_t firstQuote = line.find("\"");
            size_t lastQuote = line.find_last_of("\"");

            if (firstQuote != std::string::npos && lastQuote != std::string::npos && firstQuote != lastQuote) {
                std::string includeFile = line.substr(firstQuote + 1, lastQuote - firstQuote - 1);
                std::filesystem::path includePath = directory / includeFile; // Use filesystem for proper path handling

                shaderCode << processShaderIncludes(includePath.string(), includedFiles) << "\n";
            } else {
                std::cerr << "Error: Malformed #include directive in " << file << ": " << line << std::endl;
            }
        } else {
            shaderCode << line << "\n";
        }
    }

    return shaderCode.str();
}

unsigned int ComputeShader::compileComputeShader(const std::string& source) {
    // Assign an id for the shader
    GLCall(unsigned int id = glCreateShader(GL_COMPUTE_SHADER));
    const char* src = source.c_str();

    // Bind the id to the source code
    GLCall(glShaderSource(id, 1, &src, nullptr));

    // Compile the shader
    GLCall(glCompileShader(id));

    // Check for errors
    int result;
    GLCall(glGetShaderiv(id, GL_COMPILE_STATUS, &result));
    if (!result) {
        int length;
        GLCall(glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length));

        char* message = (char*)alloca(length * sizeof(char));

        GLCall(glGetShaderInfoLog(id, length, &length, message));

        std::cerr << "Failed to compile compute shader" << std::endl;
        std::cerr << message << std::endl;
    }

    return id;
}

unsigned int ComputeShader::createComputeShader(const std::string& compute_shader) {
    // Create a program
    unsigned int program = glCreateProgram();
    
    // Create and compile compute shader
    unsigned int cs = compileComputeShader(compute_shader);
    
    // Attach shader to the program, link and validate it
    GLCall(glAttachShader(program, cs));
    GLCall(glLinkProgram(program));
    GLCall(glValidateProgram(program));

    // Delete shader as it is attached to the program
    GLCall(glDeleteShader(cs));

    return program;
}

int ComputeShader::getUniformLocation(const std::string& name) {
    // Check if the uniform location is already in the cache and return it
    if (m_uniform_location_cache.find(name) != m_uniform_location_cache.end())
        return m_uniform_location_cache[name];

    // Get the location of the uniform
    GLCall(int location = glGetUniformLocation(m_renderer_id, name.c_str()));

    // Check for errors
    if (location == -1)
        std::cerr << "Warning: uniform '" << name << "' doesn't exist!" << std::endl;
    
    // Add the uniform location to the cache and return it
    m_uniform_location_cache[name] = location;
    return location;
}