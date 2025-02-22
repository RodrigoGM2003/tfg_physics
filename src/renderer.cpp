#include "renderer.h"
#include <iostream>

void GLClearError(){
    while (glGetError() != GL_NO_ERROR);
}

bool GLLogCall(const char* function, const char* file, int line){
    while (GLenum error = glGetError()){
        std::string fileStr(file);
        std::string filename = fileStr.substr(fileStr.find_last_of("\\/") + 1);
        std::cerr << "[OpenGL Error] (" << error << "): " << function << " " << filename << ":" << line << std::endl;
        return false;
    }
    return true;
}

void Renderer::clear() const{
    GLCall(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
}

void Renderer::setClearColor(float r, float g, float b, float a) const{
    GLCall(glClearColor(r, g, b, a));
}

void Renderer::draw(const VertexArray& va, const IndexBuffer& ib, const Shader& shader) const{
    shader.bind();

    //AQUI DA EL ERROR
    va.bind();
    ib.bind();

    GLCall(glDrawElements(GL_TRIANGLES, ib.getCount(), GL_UNSIGNED_INT, nullptr));
}

void Renderer::instancedDraw(const VertexArray& va, const IndexBuffer& ib, const Shader& shader, unsigned int instances) const{
    shader.bind();

    va.bind();
    ib.bind();

    GLCall(glDrawElementsInstanced(GL_TRIANGLES, ib.getCount(), GL_UNSIGNED_INT, nullptr, instances));
}