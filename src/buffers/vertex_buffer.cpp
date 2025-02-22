#include "vertex_buffer.h"
#include "../renderer.h"

#include <iostream>

VertexBuffer::VertexBuffer(const void *data, unsigned int size, const unsigned int usage){
    this->setBuffer(data, size, usage);
}

VertexBuffer::~VertexBuffer(){
    GLCall(glDeleteBuffers(1, &m_renderer_id)); //Delete buffer
}

void VertexBuffer::bind() const{
    GLCall(glBindBuffer(GL_ARRAY_BUFFER, m_renderer_id)); //Bind (select) buffer
}

void VertexBuffer::unbind() const{
    GLCall(glBindBuffer(GL_ARRAY_BUFFER, 0)); //Unbind buffer
}

void VertexBuffer::updateData(const void* data, unsigned int size, unsigned int offset){
    // if (m_usage != GL_DYNAMIC_DRAW){
    //     throw std::runtime_error("Buffer usage is not GL_DYNAMIC_DRAW");
    // }
    GLCall(glBufferSubData(GL_ARRAY_BUFFER, offset, size, data)); //Update buffer data
}

void VertexBuffer::setBuffer(const void* data, unsigned int size, const unsigned int usage){
    if (m_renderer_id != 0){
        GLCall(glDeleteBuffers(1, &m_renderer_id)); //Delete buffer
    }

    m_usage = usage;
    GLCall(glGenBuffers(1, &m_renderer_id)); //Generate buffer
    GLCall(glBindBuffer(GL_ARRAY_BUFFER, m_renderer_id)); //Bind (select) buffer
    GLCall(glBufferData(GL_ARRAY_BUFFER, size, data, m_usage)); //Fill buffer with data
}

