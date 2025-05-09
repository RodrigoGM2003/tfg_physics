#version 440 core
#extension GL_NV_shader_atomic_float : enable

#include "common_structs.glsl"

layout(std430, binding = 1) buffer TransformBuffer {
    mat4 transforms[];
};
layout(std430, binding = 5) buffer PropertiesBuffer {
    PropertiesStruct properties[];
};
layout(std430, binding = 21) buffer CollisionCountBuffer {
    uint collisionCount;
};

// The ContactManifold struct definition remains the same
struct ContactManifold {
    uint indexA;
    uint indexB;
    vec4 normal;          // World space, consistent direction (e.g., A->B)
    float depth;          // Penetration depth

    vec4 rALocal;
    vec4 rBLocal;
};
layout(std430, binding = 26) buffer ContactManifoldBuffer {
    ContactManifold manifolds[];
};
layout(std430, binding = 29) buffer DeltaVBuffer {
    vec4 deltaVs[]; // .xyz accumulates Dv
};
layout(std430, binding = 30) buffer DeltaWBuffer {
    vec4 deltaWs[]; // .xyz accumulates Dw
};

uniform float delta_time;

layout(local_size_x = 256) in;

void main() {
    uint gid = gl_GlobalInvocationID.x;
    if (gid >= collisionCount || delta_time < 1e-3) return;

    // --- Fetch data ---
    ContactManifold c = manifolds[gid];
    uint iA = c.indexA, iB = c.indexB;
    PropertiesStruct A = properties[iA];
    PropertiesStruct B = properties[iB];
    mat3 R_A = mat3(transforms[iA]);
    mat3 R_B = mat3(transforms[iB]);
    mat3 IinvA = R_A * A.inverseInertiaTensor * transpose(R_A);
    mat3 IinvB = R_B * B.inverseInertiaTensor * transpose(R_B);

    vec3 vA = A.velocity.xyz,  vB = B.velocity.xyz;
    vec3 wA = A.angular_velocity.xyz, wB = B.angular_velocity.xyz;
    vec3 n  = normalize(c.normal.xyz);
    vec3 rA = c.rALocal.xyz,    rB = c.rBLocal.xyz;
    float invM_A = A.inverseMass, invM_B = B.inverseMass;
    // float e = 0.1, mu = 0.3;
    // const float beta = 0.1, slop = 0.01, eps = 1e-5;

    float e = 0.0;          //Restitution
    float mu = 0.1;         //Friction
    const float beta = 0.1; //Baumgarte Beta
    const float slop = 0.01;      //Baumgarte slop
    const float eps = 1e-5;        //Epsilon

    float depth = c.depth;
    // --- Relative velocity at contact ---
    vec3 vCA = vA + cross(wA, rA);
    vec3 vCB = vB + cross(wB, rB);
    vec3 relV = vCB - vCA;
    float vn = dot(relV, n);

    vec3 DvA = vec3(0), DvB = vec3(0);
    vec3 DwA = vec3(0), DwB = vec3(0);

    if (depth > 0.0) {
        // ---- Normal impulse ----
        vec3 ra_n = cross(rA, n);
        vec3 rb_n = cross(rB, n);
        float K = invM_A + invM_B
                + dot(n, cross(IinvA * ra_n, rA))
                + dot(n, cross(IinvB * rb_n, rB));

        float jn = 0.0;
        if (K > eps) {
            float inv_dt = 1.0 / delta_time;
            float bias = -beta * inv_dt * max(0.0, depth - slop);
            jn = -(vn * (1.0 + e) + bias) / K;
        }
        jn = max(0.0, jn);

        vec3 Pn = jn * n;
        DvA -= Pn * invM_A;  DvB += Pn * invM_B;
        DwA -= IinvA * cross(rA, Pn);
        DwB += IinvB * cross(rB, Pn);

        // ---- Friction ----
        vec3 vt = relV - vn * n;
        float vt_len = length(vt);
        if (vt_len > eps) {
            vec3 t = vt / vt_len;
            vec3 ra_t = cross(rA, t);
            vec3 rb_t = cross(rB, t);
            float Kt = invM_A + invM_B
                     + dot(t, cross(IinvA * ra_t, rA))
                     + dot(t, cross(IinvB * rb_t, rB));

            float jt = (Kt > eps) ? vt_len / Kt : 0.0;
            jt = clamp(jt, -mu * jn, mu * jn);

            vec3 Pt = jt * t;
            DvA -= Pt * invM_A;  
            DvB += Pt * invM_B;
            DwA -= IinvA * cross(rA, Pt);
            DwB += IinvB * cross(rB, Pt);
        }
    }

    // --- Atomic add to accumulators ---
    atomicAdd(deltaVs[iA].x, DvA.x);
    atomicAdd(deltaVs[iA].y, DvA.y);
    atomicAdd(deltaVs[iA].z, DvA.z);
    atomicAdd(deltaVs[iB].x, DvB.x);
    atomicAdd(deltaVs[iB].y, DvB.y);
    atomicAdd(deltaVs[iB].z, DvB.z);

    atomicAdd(deltaWs[iA].x, -DwA.x);
    atomicAdd(deltaWs[iA].y, -DwA.y);
    atomicAdd(deltaWs[iA].z, -DwA.z);
    atomicAdd(deltaWs[iB].x, -DwB.x);
    atomicAdd(deltaWs[iB].y, -DwB.y);
    atomicAdd(deltaWs[iB].z, -DwB.z);
}
