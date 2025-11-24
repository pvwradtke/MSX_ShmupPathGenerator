#include <float.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>

#define DEBUG   false

/*
Quadrants in the games are viewed like this for simplicity, because the 2D viewport is in the actual 3rq quadrant in the cartesian plane
 
     |
  2  |  3
-----|-----
  1  |  0 
     |

Viewport:
---------------> x
|
|
|
|
y

As all pre-calculations are made in octants, we need to consider:

  \ 5 | 6 / 
 4 \  |  /  7
    \ | /
-----\|/-----
     /|\   
 3  / | \   0
   /  |  \
  / 2 | 1 \

If we need to determine to which octant an angle belongs to, we simply divide it by 8 and get the integer part (angle/8)

As for the way those are used, the table contains the absolute values for the movements for octant 0. To get the numbers
for the remainder octants, we swap x an y, and change signals accordingly:

0: x, y
1: y, x
2: y, -x
3: -x, y
4: -x, -y
5: -y, -x
6: -y, x
7: x, y

*/


// Contains 64 angles, with 16 movements of 2 pixels each, with the delta stored as (dx,dy) for the 4th quadrand (315 to 360 degrees)/
// All other quadrants are calculated by swapping x by y and changing the sign
uint8_t path_angle_lut[3][64][32][2];
uint8_t angle_to_lut[360];   // First index is the index into the Lut, the other 2 are the multiplication factor for x and y
uint8_t path_circle[200][2];
double double_circle[200][2];

void main(){
    double dx[32], dy[32], mdx, mdy, angle;
    uint8_t movex, movey, degree, prevdegree=30, prevx, prevy;
    for(int i=0;i<64;i++){
        angle = (M_PI_4*i)/63.0;
        degree=lround((180*angle)/M_PI);
        if(prevdegree!=degree){
            
            angle_to_lut[degree]=i;
            angle_to_lut[90-degree]=i;
            angle_to_lut[90+degree]=i;
            angle_to_lut[180-degree]=i;
            angle_to_lut[180+degree]=i;
            angle_to_lut[270-degree]=i;
            angle_to_lut[270+degree]=i;
            if(degree!=0)
                angle_to_lut[360-degree]=i;
            prevdegree=degree;
        }
        if(DEBUG)
            printf("Degree: %d \n", degree);
        dx[0]=sin(angle)*2;
        dy[0]=0;
        // Calculates the distances from the origin (0, 0)
        for(int step=0;step<32;step++){
            dx[step]=cos(angle)*(step+1);
            dy[step]=sin(angle)*(step+1);
        }
            // Calculates the integer deltas
        mdx=0;
        mdy=0;
        movex=0;
        movey=0;

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
            if(DEBUG)
                printf("Entry: %d, Angle: %f, Step: %d: (%f, %f), (%d, %d) - Round: (%d, %d)\n", i, angle, step, dx[step], dy[step], movex, movey, path_angle_lut[i][step][0], path_angle_lut[i][step][1]);
        }
    }
    // Calculates the circle movements, for several radii: 32, 64, 96, 128, 160, 192, 224
    // Does it in the first 45 degrees in the 4th quadrant, and other quadrants and complementing 45 degrees are calculated by changing signs, and dx by dy
    for(int r=32; r<225; r+=32){
        // Calculates the perimeter for 45 degrees of the circle
        double perimeter = 2*M_PI_4*r;
        // We move two pixels distance each time, then we have perimeter/2 steps to do (the character is already at the initial position)
        int steps=lround(perimeter/2.0);
        // Calculates the integer deltas
        mdx=0;
        mdy=0;
        movex=0;
        movey=0;

        // Calculate the double values
        for(int i=0;i<steps;i++){
            angle = (M_PI_4*(i+1))/(double)steps;
            double_circle[i][0]=cos(angle)*(double)r; //*2*(i+1);
            double_circle[i][1]=sin(angle)*(double)r; //2*(i+1);
            //printf("Radius: %d, Steps: %d, Step: %d, Angle: %f, (%f, %f)\n", r, steps, i, angle, double_circle[i][0], double_circle[i][1]);
        }
        // The origin point value
        prevx=r;
        prevy=0;
        // Creates the integer deltas
        for(int step=0;step<steps;step++){
            path_circle[step][0]=lround(double_circle[step][0]);
            path_circle[step][1]=lround(double_circle[step][1]);
            movex=prevx-path_circle[step][0];
            movey=path_circle[step][1]-prevy;
            // Calculates the missing fraction from the integer movement
            mdx=double_circle[step][0]-path_circle[step][0];
            mdy=double_circle[step][1]-path_circle[step][1];

            prevx=path_circle[step][0];
            prevy=path_circle[step][1];

            if(DEBUG)
                printf("Radius: %d, Steps: %d, Step: %d, (%f, %f)x(%d, %d) - Move: (%d, %d)\n", r, steps, step, double_circle[step][0], 
                    double_circle[step][1], path_circle[step][0], path_circle[step][1], movex, movey);
        }
    }
    if(!DEBUG){
        // Prints the paths
        printf("#ifndef  PATHS_H\n#define PATHS_H\n\n");

        // Print the linear PATHS
        printf("#define PATH_STEPS  32\n#define PATH_SLICES 64\n\n");
        printf("#enum   CIRCLE_MOVEMENTS    {RADIUS32, RADIUS64, RADIUS96, RADIUS128, RADIUS160, RADIUS192, RADIUS224, MAX_CIRCLE_MOVEMENTS}\\n");
        printf("const   u8  PathAngleLut[PATH_SLICES][PATH_STEPS][2] ={\n");
        for(int i=0;i<64;i++){
            printf("    { ");
            for(int j=0;j<32;j++){
                printf("{ %d, %d}", path_angle_lut[i][j][0], path_angle_lut[i][j][1]);
                if(j!=31)
                    printf(", ");
            }
            if(i!=63)
                printf("},\n");
            else
                printf("}\n");
        }
        printf("};\n\n");
        printf("const   u8  DegreeToPathLut[360] ={\n");
        for(int i=0;i<10;i++){
            printf("    ");
            for(int j=0;j<36;j++){
                if(i==9 && j==35)
                    printf("%d", angle_to_lut[i*36+j]);
                else
                    printf("%d, ", angle_to_lut[i*36+j]);
            }
            printf("\n");
        }
        printf("},\n\n");
        
        
        printf("#const  u8  CirclePathValues[MAX_CIRCLE_MOVEMENTS] = {32, 64, 96, 128, 160, 192, 224};\n\n");
        
        printf("#endif\n");
    }
}
