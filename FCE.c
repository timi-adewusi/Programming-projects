#include <stdio.h>

#define DIM 2
#define STEPS 100

typedef struct {
    float pos[DIM];
    float vel[DIM];
} Entity;

// The Flow Function F(x): Defines where the entity 'wants' to go
void compute_flow(const float* x, float* f_out) {
    // Example: Simple attractor at (10, 10)
    float target[2] = {10.0f, 10.0f};
    f_out[0] = target[0] - x[0];
    f_out[1] = target[1] - x[1];
}

// The Constraint Function C(x): Modulates flow based on boundaries [0, 5]
void compute_constraints(const float* x, float* c_out) {
    float limit = 5.0f;
    for (int i = 0; i < DIM; i++) {
        // Soft-threshold: C drops to 0 as x approaches 'limit'
        // Using a simple parabolic decay for performance
        float dist_to_boundary = limit - x[i];
        if (dist_to_boundary <= 0) c_out[i] = 0.0f;
        else if (dist_to_boundary > 1.0f) c_out[i] = 1.0f;
        else c_out[i] = dist_to_boundary; // Linear ramp between 0 and 1
    }
}

int main() {
    Entity player = {{0.0f, 0.0f}, {0.0f, 0.0f}};
    float alpha = 0.1f; // Step size (Time.deltaTime)

    printf("Step | Position X | Position Y\n");
    printf("-----------------------------\n");

    for (int t = 0; t < STEPS; t++) {
        float F[DIM];
        float C[DIM];

        compute_flow(player.pos, F);
        compute_constraints(player.pos, C);

        // FCE Update Rule: x_{t+1} = x_t + alpha * (F * C)
        for (int i = 0; i < DIM; i++) {
            player.pos[i] += alpha * (F[i] * C[i]);
        }

        if (t % 10 == 0) {
            printf("%4d | %10.3f | %10.3f\n", t, player.pos[0], player.pos[1]);
        }
    }

    return 0;
}
