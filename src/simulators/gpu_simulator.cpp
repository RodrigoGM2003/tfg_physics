#include "gpu_simulator.h"

#include <iostream>
#include <chrono>

#include "glm/gtc/matrix_transform.hpp" 
#include "glm/gtx/transform.hpp"
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtc/random.hpp" 
#include "glm/gtx/component_wise.hpp"


GpuSimulator::GpuSimulator(
    std::vector<glm::mat4>* transforms, 
    const std::vector<SimpleVertex>* static_vertices, 
    const std::vector<unsigned int>* static_indices
) : sim_transforms(transforms), 
    sim_static_vertices(static_vertices), 
    sim_static_indices(static_indices){

    initializeData();

    //Transform update shader
    m_transform_shader.setShader("sphere_transforms.glsl");
    m_transform_shader.useTimer(false);
    m_transform_shader.bind();
    
    //Narrow phase shader
    m_broad_phase_shader.setShader("collision_naive.glsl");
    m_broad_phase_shader.useTimer(false);
    m_broad_phase_shader.bind();

    //Narrow phase shader
    m_narrow_phase_shader.setShader("narrow.glsl");
    m_narrow_phase_shader.useTimer(false);
    m_narrow_phase_shader.bind();

    //SSBOs
    m_transform_ssbo.setBuffer(sim_transforms->data(), sim_transforms->size() * sizeof(glm::mat4), GL_DYNAMIC_DRAW);
    m_transform_ssbo.unbind();

    m_properties_ssbo.setBuffer(sim_properties.data(), sim_properties.size() * sizeof(physics::Properties), GL_DYNAMIC_DRAW);
    m_properties_ssbo.unbind();

    m_results_ssbo.setBuffer(nullptr, sim_transforms->size() * sizeof(int), GL_DYNAMIC_DRAW);
    m_results_ssbo.unbind();

    m_spheres_ssbo.setBuffer(sim_spheres.data(), sim_spheres.size() * sizeof(glm::vec4), GL_DYNAMIC_DRAW);
    m_spheres_ssbo.unbind();

    m_collision_pair_ssbo.setBuffer(nullptr, sim_spheres.size() * sizeof(glm::ivec2), GL_DYNAMIC_DRAW);
    m_collision_pair_ssbo.unbind();

    m_collision_count_ssbo.setBuffer(nullptr, sizeof(unsigned int), GL_DYNAMIC_DRAW);
    m_collision_count_ssbo.unbind();

    m_second_results_ssbo.setBuffer(nullptr, sim_transforms->size() * sizeof(int), GL_DYNAMIC_DRAW);
    m_second_results_ssbo.unbind();

    m_object_vertices = {
        {-0.5f, -0.5f, -0.5f, 1.0f},   
        { 0.5f, -0.5f, -0.5f, 1.0f},   
        { 0.5f, -0.5f,  0.5f, 1.0f},   
        {-0.5f, -0.5f,  0.5f, 1.0f},   

        {-0.5f,  0.5f, -0.5f, 1.0f},   
        { 0.5f,  0.5f, -0.5f, 1.0f},   
        { 0.5f,  0.5f,  0.5f, 1.0f},   
        {-0.5f,  0.5f,  0.5f, 1.0f}   
    };

    m_object_normals = {
        {1.0f, 0.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 1.0f, 0.0f}
    };

    m_object_edges = {
        {1.0f, 0.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 1.0f, 0.0f}
    };

    m_object_vertices_ssbo.setBuffer(m_object_vertices.data(), m_object_vertices.size() * sizeof(glm::vec4), GL_STATIC_DRAW);
    m_object_vertices_ssbo.unbind();

    m_object_normals_ssbo.setBuffer(m_object_normals.data(), m_object_normals.size() * sizeof(glm::vec4), GL_STATIC_DRAW);
    m_object_normals_ssbo.unbind();

    m_object_edges_ssbo.setBuffer(m_object_edges.data(), m_object_edges.size() * sizeof(glm::vec4), GL_STATIC_DRAW);
    m_object_edges_ssbo.unbind();


    m_transform_ssbo.bindToBindingPoint(1);
    m_properties_ssbo.bindToBindingPoint(5);
    m_results_ssbo.bindToBindingPoint(4);
    m_spheres_ssbo.bindToBindingPoint(7);

    m_collision_pair_ssbo.bindToBindingPoint(20);
    m_collision_count_ssbo.bindToBindingPoint(21);
    m_second_results_ssbo.bindToBindingPoint(22);

    m_object_vertices_ssbo.bindToBindingPoint(23);
    m_object_normals_ssbo.bindToBindingPoint(24);
    m_object_edges_ssbo.bindToBindingPoint(25);
}

GpuSimulator::~GpuSimulator(){
    sim_transforms = nullptr;
    sim_static_vertices = nullptr;
    sim_static_indices = nullptr;
}

void GpuSimulator::update(float delta_time){
    //Transform phase
    int work_groups = (sim_transforms->size() + 256 - 1) / 256;
    m_transform_shader.bind();
    m_transform_shader.use();
    m_transform_shader.setUniform1f("delta_time", delta_time);
    m_transform_shader.dispatch(work_groups, 1, 1);
    m_transform_shader.waitForCompletion(GL_SHADER_STORAGE_BARRIER_BIT);

    //Broad phase
    // OPARALLEL SPHERE
    work_groups = (sim_transforms->size() + 8 - 1) / 8;
    m_broad_phase_shader.bind();
    m_broad_phase_shader.use();
    m_broad_phase_shader.dispatch(work_groups * 64, 1, 1);
    m_broad_phase_shader.waitForCompletion(GL_SHADER_STORAGE_BARRIER_BIT);

    // SHARED SPHERE
    // work_groups = (sim_transforms->size() + 256 - 1) / 256;
    // m_broad_phase_shader.bind();
    // m_broad_phase_shader.use();
    // m_broad_phase_shader.dispatch(work_groups, 1, 1);
    // m_broad_phase_shader.waitForCompletion(GL_SHADER_STORAGE_BARRIER_BIT);

    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);
    m_collision_count_ssbo.bind();
    unsigned int collision_counter = *(unsigned int *)m_collision_count_ssbo.readData();
    m_collision_count_ssbo.unmapBuffer();
    
    
    // Narrow phase
    // std::cout<<"Number of collisions: "<<collision_counter<<std::endl;
    work_groups = (collision_counter + 256 - 1) / 256;
    m_narrow_phase_shader.bind();
    m_narrow_phase_shader.use();
    m_narrow_phase_shader.dispatch(work_groups, 1, 1);
    m_narrow_phase_shader.waitForCompletion(GL_SHADER_STORAGE_BARRIER_BIT);

    //Resolution phase

    // std::cout<<time<<std::endl;
}


void GpuSimulator::initializeData(){
    //Create AABBs
    float base_radius = utils::calculateRadius(*sim_static_vertices);
    std::cout<<base_radius<<std::endl;
    sim_spheres.resize(sim_transforms->size(), glm::vec4(0.0f, 0.0f, 0.0f, base_radius));

    sim_properties.resize(sim_transforms->size());

    float mass = 1.0f;
    float side = 1.0f;

    float inertia = (1.0f / 6.0f) * mass * side * side;
    float inverseInertia = 1.0f / inertia;
    glm::mat3 inverseTensor = glm::mat3(inverseInertia);

    std::srand(42);
    for (int i = 0; i < sim_transforms->size(); i++){
        
        auto transform = &sim_transforms->at(i);
        
        glm::vec3 scale = utils::scaleFromTransform(*transform); 
        sim_spheres[i].w *= glm::max(scale.x, glm::max(scale.y, scale.z));

        sim_spheres[i][0] = (*transform)[3][0];
        sim_spheres[i][1] = (*transform)[3][1];
        sim_spheres[i][2] = (*transform)[3][2];

        sim_properties[i].velocity = glm::linearRand(glm::vec3(-0.05f), glm::vec3(0.05f));
        sim_properties[i].acceleration = glm::vec3(0.0f);
        sim_properties[i].angular_velocity = glm::linearRand(glm::vec3(-0.05f), glm::vec3(0.05f));
        sim_properties[i].angular_acceleration = glm::vec3(0.0f);

        sim_properties[i].inverseMass = 1.0f;
        sim_properties[i].inverseInertiaTensor = inverseTensor;
        sim_properties[i].friction = 0.0f;
    }
}