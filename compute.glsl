#version 430
layout (local_size_x = 1, local_size_y = 1) in; // invocation size per work group
layout (rgba8, binding = 0) uniform writeonly image2D imgOutput;

uniform uint WIDTH;
uniform uint HEIGHT;

const float dt = 0.01;
const float g = 1000; // might have to tweak
const bool damp = true;
const float damping = 1; // no damping

const float m1 = 0.1;
const float m2 = 0.1;

float L1 = 200;
float L2 = 200;

float error = 0.005;
float max_time = 8; // x seconds max time allotted

const float PI = 3.14159265359;

vec2 screenSpaceToAngleSpace(float x, float y) {
    return vec2(PI/(WIDTH/6) * (x - WIDTH), PI/(HEIGHT/6) * (y - HEIGHT));
};

vec2 accelerations(float theta1, float theta2, float theta1_v, float theta2_v) {
    float delta = theta1 - theta2;
    float cos2d = cos(2 * delta);
    float denom_common = (2 * m1 + m2 - m2 * cos2d);

    float denom1 = L1 * denom_common;
    float denom2 = L2 * denom_common;

    if (denom1 == 0 || denom2 == 0) {
        return vec2(0, 0);
    }

    // alpha1 numerator (matches standard formula)
    float a1_num =
    -g * (2 * m1 + m2) * sin(theta1)
    - m2 * g * sin(theta1 - 2 * theta2)
    - 2 * sin(delta) * m2 * (theta2_v * theta2_v * L2 + theta1_v * theta1_v * L1 * cos(delta));
    float alpha1 = a1_num / denom1;

    // alpha2 numerator: 2*sin(delta) * ( ... whole parenthesis ... )
    float inner = theta1_v * theta1_v * L1 * (m1 + m2)
    + g * (m1 + m2) * cos(theta1)
    + theta2_v * theta2_v * L2 * m2 * cos(delta);

    float alpha2 = (2 * sin(delta) * inner) / denom2;

    // if (isinf(alpha1) || isinf(alpha2) || isnan(alpha1) || isnan(alpha2)) {
    //     imageStore(imgOutput, ivec2(gl_GlobalInvocationID.xy), vec4(0, 0, 0, 0));
    //     // do something else ig idk
    // }

    return vec2(alpha1, alpha2);
}

void main() {
    ivec2 coord = ivec2(gl_GlobalInvocationID.xy);
    vec2 angles = screenSpaceToAngleSpace(coord.x, coord.y);
    float angle1 = angles[0];
    float angle2 = angles[1];

    // pendulum A - exact angles, pendulum B - slight error
    float theta1A = angle1;
    float theta1_vA = 0;
    float theta2A = angle2;
    float theta2_vA = 0;

    float theta1B = angle1 + error;
    float theta1_vB = 0;
    float theta2B = angle2 + error;
    float theta2_vB = 0;

    float diff = 0;
    float elapsed = 0;

    while (true) {
        vec2 accelsA = accelerations(theta1A, theta2A, theta1_vA, theta2_vA);
        float alpha1A = accelsA.x;
        float alpha2A = accelsA.y;
        theta1_vA += alpha1A * dt;
        theta2_vA += alpha2A * dt;
        theta1A += theta1_vA * dt;
        theta2A += theta2_vA * dt;

        vec2 accelsB = accelerations(theta1B, theta2B, theta1_vB, theta2_vB);
        float alpha1B = accelsB.x;
        float alpha2B = accelsB.y;
        theta1_vB += alpha1B * dt;
        theta2_vB += alpha2B * dt;
        theta1B += theta1_vB * dt;
        theta2B += theta2_vB * dt;

        if (damping > 0) {
            theta1_vA *= damping;
            theta1_vB *= damping;
            theta2_vA *= damping;
            theta2_vB *= damping;
        }

        diff += (abs(theta1A - theta1B) + abs(theta2A - theta2B));

        elapsed += dt;
        if (elapsed > max_time) {
            break;
        }
    }

    // float d = log(diff/255);
    float d = diff/255;

    // first color scheme 0.5d, 0.5d, d

    imageStore(imgOutput, ivec2(gl_GlobalInvocationID.xy), vec4(0.5*d, 0.5*d, d, 1.0));
    
    // reminder how to set color
    // imageStore(imgOutput, ivec2(gl_GlobalInvocationID.xy), vec4(0, 0, 0, 1.0));
}