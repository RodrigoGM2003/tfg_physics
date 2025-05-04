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

    std::cout<<"number of objects: "<<sim_transforms->size()<<std::endl;
    //Transform update shader
    m_transform_shader.setShader("sphere_transforms.glsl");
    m_transform_shader.useTimer(false);
    m_transform_shader.bind();
    
    //Broad phase shader
    m_broad_phase_shader.setShader("collision_naive.glsl");
    m_broad_phase_shader.useTimer(false);
    m_broad_phase_shader.bind();

    //Narrow phase shader
    m_narrow_phase_shader.setShader("narrow_working.glsl");
    m_narrow_phase_shader.useTimer(false);
    m_narrow_phase_shader.bind();
    
    //Resolution phase shaders
    m_impulse_phase_shader.setShader("jacobi_baumgarte_impulse.glsl");
    m_impulse_phase_shader.useTimer(false);
    m_impulse_phase_shader.bind();
    
    m_accumulation_phase_shader.setShader("accumulator.glsl");
    m_accumulation_phase_shader.useTimer(false);
    m_accumulation_phase_shader.bind();


    //SSBOs
    m_transform_ssbo.setBuffer(sim_transforms->data(), sim_transforms->size() * sizeof(glm::mat4), GL_DYNAMIC_DRAW);
    m_transform_ssbo.unbind();

    m_properties_ssbo.setBuffer(sim_properties.data(), sim_properties.size() * sizeof(physics::Properties), GL_DYNAMIC_DRAW);
    m_properties_ssbo.unbind();

    m_results_ssbo.setBuffer(nullptr, sim_transforms->size() * sizeof(int), GL_DYNAMIC_DRAW);
    m_results_ssbo.unbind();

    m_spheres_ssbo.setBuffer(sim_spheres.data(), sim_spheres.size() * sizeof(glm::vec4), GL_DYNAMIC_DRAW);
    m_spheres_ssbo.unbind();

    m_collision_pair_ssbo.setBuffer(nullptr, sim_spheres.size() * sizeof(glm::ivec2) * sim_spheres.size(), GL_DYNAMIC_DRAW);
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
        {0.0f, 0.0f, 1.0f, 0.0f},
    };
    // m_object_edges = {
    //     0, 1,
    //     0, 3,
    //     0, 4,
    //     6, 7,
    //     6, 5,
    //     6, 2,
    //     5, 1,
    //     4, 5,
    //     4, 7,
    //     7, 3,
    //     2, 3,
    //     2, 1
    // };

    m_object_vertices_ssbo.setBuffer(m_object_vertices.data(), m_object_vertices.size() * sizeof(glm::vec4), GL_STATIC_DRAW);
    m_object_vertices_ssbo.unbind();

    m_object_normals_ssbo.setBuffer(m_object_normals.data(), m_object_normals.size() * sizeof(glm::vec4), GL_STATIC_DRAW);
    m_object_normals_ssbo.unbind();

    m_object_edges_ssbo.setBuffer(m_object_edges.data(), m_object_edges.size() * sizeof(glm::vec4), GL_STATIC_DRAW);
    // m_object_edges_ssbo.setBuffer(m_object_edges.data(), m_object_edges.size() * sizeof(unsigned int), GL_STATIC_DRAW);
    m_object_edges_ssbo.unbind();

    std::vector<physics::ContactManifold> manifolds(sim_spheres.size() * sim_spheres.size(), physics::ContactManifold());
    m_contact_manifolds_ssbo.setBuffer(manifolds.data(), sim_spheres.size() * sizeof(physics::ContactManifold) * sim_spheres.size(), GL_DYNAMIC_DRAW);
    m_contact_manifolds_ssbo.unbind();
    
    // //---------------------------------
    // m_normal_impulses_ssbo.setBuffer(m_normal_impulses.data(), sim_spheres.size() * sizeof(float) * 2, GL_DYNAMIC_DRAW);
    // m_normal_impulses_ssbo.unbind();

    // m_tangent_impulses_ssbo.setBuffer(m_tangent_impulses.data(), sim_spheres.size() * sizeof(glm::vec3) * 2, GL_DYNAMIC_DRAW);
    // m_tangent_impulses_ssbo.unbind();

    std::vector<glm::vec4> deltaV(sim_transforms->size(), glm::vec4(0.0f));
    m_deltaV_ssbo.setBuffer(deltaV.data(), sim_transforms->size() * sizeof(glm::vec4), GL_DYNAMIC_DRAW);
    m_deltaV_ssbo.unbind();

    // m_delta_angular_impulses_ssbo.setBuffer(m_delta_angular_impulses.data(), sim_transforms->size() * sizeof(glm::vec3), GL_DYNAMIC_DRAW);
    // m_delta_angular_impulses_ssbo.unbind();
    //---------------------------------


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
    m_contact_manifolds_ssbo.bindToBindingPoint(26);

    m_normal_impulses_ssbo.bindToBindingPoint(27);
    m_tangent_impulses_ssbo.bindToBindingPoint(28);
    m_deltaV_ssbo.bindToBindingPoint(29);
    // m_delta_angular_impulses_ssbo.bindToBindingPoint(10);
}

GpuSimulator::~GpuSimulator(){
    sim_transforms = nullptr;
    sim_static_vertices = nullptr;
    sim_static_indices = nullptr;
}

void printVec3(const glm::vec3& v, const std::string& name) {
    std::cout << name << ": (" << v.x << ", " << v.y << ", " << v.z << ")" << std::endl;
}
void GpuSimulator::update(float delta_time){
    //Transform phase
    int work_groups = (sim_transforms->size() + 256 - 1) / 256;
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
    
    std::cout<<"Max count: "<<sim_spheres.size() * sim_spheres.size() <<" actual count: "<<collision_counter<<std::endl;
    // Narrow phase and resolution
    if(collision_counter > 0){
        work_groups = (collision_counter + 256 - 1) / 256;
        m_narrow_phase_shader.use();
        m_narrow_phase_shader.dispatch(work_groups, 1, 1);

        m_narrow_phase_shader.waitForCompletion(GL_SHADER_STORAGE_BARRIER_BIT);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);
        m_collision_count_ssbo.bind();
        collision_counter = *(unsigned int *)m_collision_count_ssbo.readData();
        m_collision_count_ssbo.unmapBuffer();


        for(int i = 0; i < 10 && collision_counter > 0; i++){

            
            // std::cout<<std::endl;
            // std::cout<<"--------------------------------------------------------------------"<<std::endl;
            // std::cout<<"MANIFOLDS: ";
            // m_contact_manifolds_ssbo.bind();
            // physics::ContactManifold* obj3 = (physics::ContactManifold *)m_contact_manifolds_ssbo.readData();
            // for(int i = 0; i < collision_counter; i++){
            //     glm::vec3 velocity = obj3[i].normal;
            //     unsigned int indexA = obj3[i].indexA;
            //     unsigned int indexB = obj3[i].indexB;
            //     float depth = obj3[i].depth;
            //     std::cout<<"Objects "<<indexA<< " and "<< indexB <<std::endl;
            //     std::cout<<"Normal: "<<velocity.x<<" "<<velocity.y<<" "<<velocity.z<<" | ";
            //     std::cout<<"Depth: "<<depth<<" | ";
            //     std::cout<<std::endl;
            // }
            // m_contact_manifolds_ssbo.unmapBuffer();
            // glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);

            // // ------------------------------------------------------------------//
            // m_deltaV_ssbo.bind();
            // glm::vec4* obj = (glm::vec4 *)m_deltaV_ssbo.readData();
            // for(int i = 0; i < sim_spheres.size(); i++){
            //     glm::vec4 velocity = obj[i];
            //     std::cout<<"Object "<<i <<": "<<velocity.x<<" "<<velocity.y<<" "<<velocity.z<<" | ";
            // }
            // m_deltaV_ssbo.unmapBuffer();
            // glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);
            // std::cout<<std::endl;
            // // ------------------------------------------------------------------//


            work_groups = (collision_counter + 256 - 1) / 256;
            m_impulse_phase_shader.use();
            m_impulse_phase_shader.setUniform1f("delta_time", delta_time);
            m_impulse_phase_shader.dispatch(work_groups, 1, 1);
            
            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT); // Essential for SSBO read-after-write
            
            // std::cout<<"VELOCITIES: ";
            // m_properties_ssbo.bind();
            // physics::Properties* obj2 = (physics::Properties *)m_properties_ssbo.readData();
            // for(int i = 0; i < sim_spheres.size(); i++){
            //     glm::vec3 velocity = obj2[i].velocity;
            //     float invMass =  obj2[i].inverseMass;
            //     std::cout<<"Object velocity "<<i <<": "<<velocity.x<<" "<<velocity.y<<" "<<velocity.z<<" InvMass :" <<invMass<<" | ";
            // }
            // m_properties_ssbo.unmapBuffer();
            // glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);
            // std::cout<<std::endl;
            // // ------------------------------------------------------------------//

            // // ------------------------------------------------------------------//
            // m_deltaV_ssbo.bind();
            // obj = (glm::vec4 *)m_deltaV_ssbo.readData();
            // for(int i = 0; i < sim_spheres.size(); i++){
            //     glm::vec4 velocity = obj[i];
            //     std::cout<<"Object "<<i <<": "<<velocity.x<<" "<<velocity.y<<" "<<velocity.z<<" | ";
            // }
            // m_deltaV_ssbo.unmapBuffer();
            // glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);
            // std::cout<<std::endl;
            // // ------------------------------------------------------------------//

            
            work_groups = (sim_transforms->size() + 256 - 1) / 256;
            // std::cout<<"Work: "<<work_groups<<" collision: "<<collision_counter<<std::endl;
            m_accumulation_phase_shader.use();
            m_accumulation_phase_shader.dispatch(work_groups, 1, 1);

            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);

            // //------------------------------------------------------------------//

            // m_deltaV_ssbo.bind();
            // obj = (glm::vec4 *)m_deltaV_ssbo.readData();
            // for(int i = 0; i < sim_spheres.size(); i++){
            //     glm::vec4 velocity = obj[i];
            //     std::cout<<"Object "<<i <<": "<<velocity.x<<" "<<velocity.y<<" "<<velocity.z<<" | ";
            // }
            // m_deltaV_ssbo.unmapBuffer();
            // glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);
            // std::cout<<std::endl;
            // //------------------------------------------------------------------//

            // std::cout<<"VELOCITIES: ";
            // m_properties_ssbo.bind();
            // obj2 = (physics::Properties *)m_properties_ssbo.readData();
            // for(int i = 0; i < sim_spheres.size(); i++){
            //     glm::vec3 velocity = obj2[i].velocity;
            //     std::cout<<"Object "<<i <<": "<<velocity.x<<" "<<velocity.y<<" "<<velocity.z<<" | ";
            // }
            // m_properties_ssbo.unmapBuffer();
            // glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);
            // std::cout<<std::endl<<"--------------------------------------------------------------------"<<std::endl;

            // //------------------------------------------------------------------//


        }
    }
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

        // sim_properties[i].velocity = glm::linearRand(glm::vec3(-0.05f), glm::vec3(0.05f));
        sim_properties[i].velocity = glm::vec3(0.0f);
        // sim_properties[i].velocity = glm::vec3(0.0f, -1.0f, 0.0f);
        sim_properties[i].acceleration = glm::vec3(0.0f);
        // sim_properties[i].angular_velocity = glm::linearRand(glm::vec3(-0.05f), glm::vec3(0.05f));
        sim_properties[i].angular_velocity = glm::vec3(0.0f);
        sim_properties[i].angular_acceleration = glm::vec3(0.0f);

        // sim_properties[i].inverseMass = i % 2 == 0 ? 0.0f : 1.0f;
        sim_properties[i].inverseMass = 1.0f;
        sim_properties[i].inverseInertiaTensor = inverseTensor;
        sim_properties[i].friction = 0.0f;
    }

    sim_properties[sim_transforms->size() - 2].velocity = glm::vec3(3.0f, 0.0f, 0.0f);
    sim_properties[sim_transforms->size() - 2].angular_velocity= glm::linearRand(glm::vec3(-0.05f), glm::vec3(0.05f));
    // sim_properties[sim_transforms->size() - 2].angular_velocity= glm::vec3(0.0f, 0.0f, 0.0f);
    sim_properties[sim_transforms->size() - 2].inverseMass = 1.0f/125.0f;
    sim_properties[sim_transforms->size() - 2].inverseInertiaTensor = inverseTensor;
    sim_properties[sim_transforms->size() - 2].friction = 0.0f;

    sim_properties[sim_transforms->size() - 1].velocity = glm::vec3(0.0f);
    sim_properties[sim_transforms->size() - 1].angular_velocity= glm::vec3(0.0f);
    sim_properties[sim_transforms->size() - 1].inverseMass = 0.0f;
    sim_properties[sim_transforms->size() - 1].inverseInertiaTensor = inverseTensor;
    sim_properties[sim_transforms->size() - 1].friction = 0.0f;


    m_normal_impulses.resize(sim_spheres.size() * sim_spheres.size() , 0);
    m_tangent_impulses.resize(sim_spheres.size() * sim_spheres.size() , glm::vec3(0.0f));
    m_delta_linear_impulses.resize(sim_transforms->size(), glm::vec3(0.0f));
    m_delta_angular_impulses.resize(sim_transforms->size(), glm::vec3(0.0f));
}