#include "renderer.h"
#include <iostream>

#include "glm/glm.hpp"
#include "meshes/mesh.h"


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

void Renderer::draw(const Mesh& mesh, Shader& shader, Camera& camera){
    //Bind the shader
    shader.bind();

    shader.setUniformVec3f("u_cam_pos", camera.getPosition());

    glm::mat4 model = mesh.getModelMatrix();

    shader.setUniformMat4f("u_model", model);
    camera.matrix(shader, "u_cam_matrix");

    //Draw the mesh
    mesh.getVertexArray().bind();
    mesh.getIndexBuffer().bind();
    GLCall(glDrawElements(GL_TRIANGLES, mesh.getIndexBuffer().getCount(), GL_UNSIGNED_INT, nullptr));
}

void Renderer::instancedDraw(const InstancedMesh& instanced_mesh, Shader& shader, Camera& camera, unsigned int instances){
    //Bind the shader
    shader.bind();

    shader.setUniformVec3f("u_cam_pos", camera.getPosition());

    camera.matrix(shader, "u_cam_matrix");

    //Draw the mesh
    instanced_mesh.getVertexArray().bind();
    instanced_mesh.getIndexBuffer().bind();
    GLCall(glDrawElementsInstanced(GL_TRIANGLES, instanced_mesh.getIndexBuffer().getCount(), GL_UNSIGNED_INT, nullptr, instances));
}