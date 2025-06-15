#include "collision_detector.h"

#include <iostream>
#include <chrono>

#include "glm/gtc/matrix_transform.hpp" 
#include "glm/gtx/transform.hpp"
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtc/random.hpp" 
#include "glm/gtx/component_wise.hpp"
#include "../constants.h"
#include "../utils.h"


CollisionDetector::CollisionDetector(
    std::vector<glm::mat4>* transforms, 
    const std::vector<SimpleVertex>* static_vertices, 
    const std::vector<unsigned int>* static_indices,
    const std::vector<glm::vec4>* object_vertices,
    const std::vector<glm::vec4>* object_normals, 
    const std::vector<glm::vec4>* object_edges,
    std::vector<physics::Properties>* properties
) : sim_transforms(transforms), 
    sim_static_vertices(static_vertices), 
    sim_static_indices(static_indices),
    m_object_vertices(object_vertices),
    m_object_normals(object_normals),
    m_object_edges(object_edges),
    sim_properties(properties)
    {

    initializeData();

    std::cout<<"number of objects: "<<sim_transforms->size()<<std::endl;
    //Transform update shader
    m_transform_shader.setShader("sphere_transforms.glsl");
    m_transform_shader.useTimer(false);
    m_transform_shader.bind();
    
    //Broad phase shader
    m_broad_phase_shader.setShader("collision_naive.glsl");
    m_broad_phase_shader.useTimer(true);
    m_broad_phase_shader.bind();

    //Narrow phase shader
    // m_narrow_phase_shader.setShader("narrow_working.glsl");
    m_narrow_phase_shader.setShader("narrow_color.glsl");
    m_narrow_phase_shader.useTimer(true);
    m_narrow_phase_shader.bind();
    
    //Resolution phase shaders
    m_impulse_phase_shader.setShader("jacobi_friction_impulse.glsl");
    // m_impulse_phase_shader.setShader("jacobi_rotation_impulse.glsl");
    // m_impulse_phase_shader.setShader("constraint_rotation.glsl");
    m_impulse_phase_shader.useTimer(false);
    m_impulse_phase_shader.bind();
    
    m_accumulation_phase_shader.setShader("accumulator.glsl");
    // m_accumulation_phase_shader.setShader("accumulator_rotation.glsl");
    m_accumulation_phase_shader.useTimer(false);
    m_accumulation_phase_shader.bind();


    //SSBOs
    m_transform_ssbo.setBuffer(sim_transforms->data(), sim_transforms->size() * sizeof(glm::mat4), GL_DYNAMIC_DRAW);
    m_transform_ssbo.unbind();

    m_properties_ssbo.setBuffer(sim_properties->data(), sim_properties->size() * sizeof(physics::Properties), GL_DYNAMIC_DRAW);
    m_properties_ssbo.unbind();

    m_results_ssbo.setBuffer(nullptr, sim_transforms->size() * sizeof(int), GL_DYNAMIC_DRAW);
    m_results_ssbo.unbind();

    m_spheres_ssbo.setBuffer(sim_spheres.data(), sim_spheres.size() * sizeof(glm::vec4), GL_DYNAMIC_DRAW);
    m_spheres_ssbo.unbind();

    m_collision_pair_ssbo.setBuffer(nullptr, sim_spheres.size() * 10 * sizeof(glm::ivec2) , GL_DYNAMIC_DRAW);
    m_collision_pair_ssbo.unbind();

    m_collision_count_ssbo.setBuffer(nullptr, sizeof(unsigned int), GL_DYNAMIC_DRAW);
    m_collision_count_ssbo.unbind();

    m_second_results_ssbo.setBuffer(nullptr, sim_transforms->size() * sizeof(int), GL_DYNAMIC_DRAW);
    m_second_results_ssbo.unbind();

    m_object_vertices_ssbo.setBuffer(m_object_vertices->data(), m_object_vertices->size() * sizeof(glm::vec4), GL_STATIC_DRAW);
    m_object_vertices_ssbo.unbind();

    m_object_normals_ssbo.setBuffer(m_object_normals->data(), m_object_normals->size() * sizeof(glm::vec4), GL_STATIC_DRAW);
    m_object_normals_ssbo.unbind();

    m_object_edges_ssbo.setBuffer(m_object_edges->data(), m_object_edges->size() * sizeof(glm::vec4), GL_STATIC_DRAW);
    // m_object_edges_ssbo.setBuffer(m_object_edges.data(), m_object_edges.size() * sizeof(unsigned int), GL_STATIC_DRAW);
    m_object_edges_ssbo.unbind();


    frames = 0;
    broad_accumulator = 0;
    narrow_accumulator = 0;
    avg_broad_time = 0.0f;
    avg_narrow_time = 0.0f;


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

CollisionDetector::~CollisionDetector(){
    sim_transforms = nullptr;
    sim_static_vertices = nullptr;
    sim_static_indices = nullptr;
}

void CollisionDetector::update(float delta_time, glm::vec3 gravity){
    //Transform phase
    int work_groups = (sim_transforms->size() + 256 - 1) / 256;
    m_transform_shader.use();
    m_transform_shader.setUniform1f("delta_time", delta_time);
    m_transform_shader.setUniform3f("gravity", gravity.x, gravity.y, gravity.z);
    m_transform_shader.dispatch(work_groups, 1, 1);
    m_transform_shader.waitForCompletion(GL_SHADER_STORAGE_BARRIER_BIT);

    unsigned int broad_time = 0;
    unsigned int narrow_time = 0;
    //Broad phase
    work_groups = (sim_transforms->size() + 8 - 1) / 8;
    m_broad_phase_shader.use();
    m_broad_phase_shader.dispatch(work_groups * 64, 1, 1);
    broad_time = m_broad_phase_shader.waitForCompletion(GL_SHADER_STORAGE_BARRIER_BIT);

    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);
    m_collision_count_ssbo.bind();
    unsigned int collision_counter = *(unsigned int *)m_collision_count_ssbo.readData();
    m_collision_count_ssbo.unmapBuffer();
    
    // Narrow phase and resolution
    if(collision_counter > 0){
        work_groups = (collision_counter + 512 - 1) / 512;
        m_narrow_phase_shader.use();
        m_narrow_phase_shader.dispatch(work_groups, 1, 1);
        
        narrow_time = m_narrow_phase_shader.waitForCompletion(GL_SHADER_STORAGE_BARRIER_BIT);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);
        m_collision_count_ssbo.bind();
        collision_counter = *(unsigned int *)m_collision_count_ssbo.readData();
        // std::cout<<"Max count: "<<sim_spheres.size() * 10 <<" actual count: "<<collision_counter<<" Work Groups: "<<work_groups<<std::endl;
        m_collision_count_ssbo.unmapBuffer();
        frames++;
        broad_accumulator += broad_time;
        narrow_accumulator += narrow_time;
        
        // if (frames % 100 == 0){
        //     avg_broad_time = static_cast<float>(broad_accumulator) / (frames * 1000000);
        //     avg_narrow_time = static_cast<float>(narrow_accumulator) / (frames * 1000000);
        //     std::cout<<"Average Broad Time: "<<avg_broad_time<<" ms | Average Narrow Time: "<<avg_narrow_time<<" ms"<<std::endl;
        //     broad_accumulator = 0;
        //     narrow_accumulator = 0;
        //     frames = 0;
        // }
    }

}


void CollisionDetector::initializeData(){
    //Create AABBs
    float base_radius = utils::calculateRadius(*sim_static_vertices);
    std::cout<<base_radius<<std::endl;
    sim_spheres.resize(sim_transforms->size(), glm::vec4(0.0f, 0.0f, 0.0f, base_radius));

    std::srand(42);
    for (int i = 0; i < sim_transforms->size(); i++){
        
        auto transform = &sim_transforms->at(i);
        
        glm::vec3 scale = utils::scaleFromTransform(*transform); 
        sim_spheres[i].w *= glm::max(scale.x, glm::max(scale.y, scale.z));

        sim_spheres[i][0] = (*transform)[3][0];
        sim_spheres[i][1] = (*transform)[3][1];
        sim_spheres[i][2] = (*transform)[3][2];

    }
}