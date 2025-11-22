#include <stdio.h>
#include <stdint.h>
#include <math.h>

// Contains 64 angles, with 16 movements of 2 pixels each, with the delta stored as (dx,dy) for the 4th quadrand (315 to 360 degrees)/
// All other quadrants are calculated by swapping x by y and changing the sign
uint8_t path_angle_lut[64][32][2];
uint8_t path_circle[200][2];
double double_circle[200][2];

void main(){
    double dx[32], dy[32], mdx, mdy, angle, prevx, prevy;
    uint8_t movex, movey;
    for(int i=0;i<64;i++){
        angle = (0.7853981634*(i+1))/64.0;
        if(i==0){
            for(int step=0;step<32;step++){
                dx[step]=2;
                path_angle_lut[0][step][0]=2;
                dy[step]=0;
                path_angle_lut[0][step][1]=0;
            }
        }
        else{
            dx[0]=sin(angle)*2;
            dy[0]=0;
            // Calculates the distances from the origin (0, 0)
            for(int step=0;step<32;step++){
                dx[step]=cos(angle)*2*(step+1);
                dy[step]=sin(angle)*2*(step+1);
            }
            // Calculates the integer deltas
            mdx=0;
            mdy=0;
            movex=0;
            movey=0;
            prevx=0;
            prevy=0;
            for(int step=0;step<32;step++){
                // Rounds, taking into accout what we misseds in the previous rounding
                path_angle_lut[i][step][0]=lround(dx[step]-movex+mdx);
                path_angle_lut[i][step][1]=lround(dy[step]-movey+mdy);
                // Calculates the total integer movement so far
                movex+=path_angle_lut[i][step][0];
                movey+=path_angle_lut[i][step][1];
                // Calculates the missing fraction from the integer movement
                mdx=dx[step]-movex;
                mdx=dy[step]-movey;
                printf("Entry: %d, Angle: %f, Step: %d: (%f, %f), (%d, %d) - Round: (%d, %d)\n", i, angle, step, dx[step], dy[step], movex, movey, path_angle_lut[i][step][0], path_angle_lut[i][step][1]);
            }
        }
        //for(int step=0;step<32;step++)
            //printf("Entry: %d, Angle: %f, Step: %d: (%f, %f) - Round: (%d, %d)\n", i, angle, step, dx[step], dy[step], path_angle_lut[i][step][0], path_angle_lut[i][step][1]);
    }
    // Calculates the circle movements, for several radii: 32, 64, 96, 128, 160, 192, 224
    // Does it in the first 45 degrees in the 4th quadrant, and other quadrants and complementing 45 degrees are calculated by changing signs, and dx by dy
    for(int r=32; r<225; r+=32){
        // Calculates the perimeter for 45 degrees of the circle
        double perimeter = 2*0.7853981634*r;
        // We move two pixels distance each time, then we have perimeter/2 steps to do (the character is already at the initial position)
        int steps=lround(perimeter/2.0);
        for(int i=0;i<steps;i++){
            angle = (0.7853981634*(i+1))/(double)steps;
            double_circle[i][0]=cos(angle)*(double)r; //*2*(i+1);
            double_circle[i][1]=sin(angle)*(double)r; //2*(i+1);
            printf("Radius: %d, Steps: %d, Step: %d, Angle: %f, (%f, %f)\n", r, steps, i, angle, double_circle[i][0], double_circle[i][1]);
        }

        //for(int i=0;i<steps;i++)
          //  printf("Radius: %d, Steps: %d, Step: %d, Angle: %f, (%f, %f)\n", r, steps, i, angle, double_circle[i][0], double_circle[i][1]);
    }
}
