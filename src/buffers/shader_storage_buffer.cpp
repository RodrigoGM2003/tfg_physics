#include "shader_storage_buffer.h"
#include "../renderer.h"

#include <iostream>

ShaderStorageBuffer::ShaderStorageBuffer(const void *data, unsigned int size, const unsigned int usage){
    this->setBuffer(data, size, usage);
}

ShaderStorageBuffer::~ShaderStorageBuffer(){
    GLCall(glDeleteBuffers(1, &m_renderer_id)); //Delete buffer
}

void ShaderStorageBuffer::bind() const{
    GLCall(glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_renderer_id)); //Bind (select) buffer
}

void ShaderStorageBuffer::unbind() const{
    GLCall(glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0)); //Unbind buffer
}

void ShaderStorageBuffer::updateData(const void* data, unsigned int size, unsigned int offset){
    // if (m_usage != GL_DYNAMIC_DRAW){
    //     throw std::runtime_error("Buffer usage is not GL_DYNAMIC_DRAW");
    // }
    GLCall(glBufferSubData(GL_SHADER_STORAGE_BUFFER, offset, size, data)); //Update buffer data
}

void ShaderStorageBuffer::setBuffer(const void* data, unsigned int size, const unsigned int usage){
    if (m_renderer_id != 0){
        GLCall(glDeleteBuffers(1, &m_renderer_id)); //Delete buffer
    }

    m_usage = usage;
    GLCall(glGenBuffers(1, &m_renderer_id)); //Generate buffer
    GLCall(glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_renderer_id)); //Bind (select) buffer
    GLCall(glBufferData(GL_SHADER_STORAGE_BUFFER, size, data, m_usage)); //Fill buffer with data
}

bool ShaderStorageBuffer::bindToBindingPoint(unsigned int binding_point){
    auto result = ShaderStorageBuffer::taken_binding_points.insert(binding_point);

    if (!result.second) 
        std::cerr << "Warning: binding point " << binding_point << " already bound by a different SSBO" << std::endl; 
    
    else {
        m_binding_point = binding_point;
        GLCall(glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding_point, m_renderer_id));
    }

    return result.second;
}

void ShaderStorageBuffer::unbindFromBindingPoint(){
    ShaderStorageBuffer::taken_binding_points.erase(m_binding_point);

    m_binding_point = 0;
    GLCall(glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_renderer_id));
}


void* ShaderStorageBuffer::readData(){
    GLCall(void* result = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY));
    return result;
}

void ShaderStorageBuffer::unmapBuffer(){
    GLCall(glUnmapBuffer(GL_SHADER_STORAGE_BUFFER));
}
