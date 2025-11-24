#include <float.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>

#define DEBUG   true
#define PATH_STEPS   32

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
uint8_t path_angle_lut[64][PATH_STEPS][2];
uint8_t angle_to_lut[360];   // First index is the index into the Lut, the other 2 are the multiplication factor for x and y
int  path_circle[1024][2];
double double_circle[1024][2];

void main(){
    double dx[PATH_STEPS], dy[PATH_STEPS], mdx, mdy, angle;
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
        else {
            angle_to_lut[360-degree]=prevdegree;
        }
        if(DEBUG)
            printf("Degree: %d \n", degree);
        // Calculates the distances from the origin (0, 0)
        for(int step=0;step<PATH_STEPS;step++){
            dx[step]=cos(angle)*(step+1);
            dy[step]=sin(angle)*(step+1);
        }
        prevy=0;
        for(int step=0;step<PATH_STEPS;step++){
            path_angle_lut[i][step][0]=lround(dx[step]);
            path_angle_lut[i][step][1]=lround(dy[step]);
            // Correct in case it has moved less than 2 pixels in the y axis (issue with sprites on line 216, so we only have sprites on odd lines)
            if(path_angle_lut[i][step][1]-prevy==2){
                prevy=path_angle_lut[i][step][1];
            }else {
                path_angle_lut[i][step][1]=prevy;
            }
            if(DEBUG)
                printf("Entry: %d, Angle: %f, Step: %d: (%f, %f), (%d, %d)\n", i, angle, step, dx[step], dy[step], path_angle_lut[i][step][0], path_angle_lut[i][step][1]);
        }
    }
    // Calculates the circle movements, for several radii: 32, 64, 96, 128, 160, 192, 224
    // Does it in the first 45 degrees in the 4th quadrant, and other quadrants and complementing 45 degrees are calculated by changing signs, and dx by dy
    int radius_steps[6];
    for(int r=32; r<225; r+=32){
        // Calculates the perimeter for 360 degrees of the circle
        double perimeter = 2*M_PI*r;
        // We move two pixels distance each time, then we have perimeter/2 steps to do (the character is already at the initial position)
        int steps=lround(perimeter/2.0);
        if(steps%4)
            steps=steps+(4-steps%4);
        // Calculates the integer deltas
        mdx=0;
        mdy=0;
        movex=0;
        movey=0;
        radius_steps[r/32 - 1] = steps;
        // Calculate the double values
        for(int i=0;i<steps;i++){
            angle = (2*M_PI*(i+1))/(double)steps;
            double_circle[i][0]=cos(angle)*(double)r;//*2*(i+1);
            double_circle[i][1]=sin(angle)*(double)r;//*2*(i+1);
            path_circle[i][0]=lround(double_circle[i][0]);
            path_circle[i][1]=lround(double_circle[i][1]);            
            //printf("Radius: %d, Steps: %d, Step: %d, Angle: %f, (%f, %f)\n", r, steps, i, angle, double_circle[i][0], double_circle[i][1]);
            if(DEBUG)
                printf("Radius: %d, Steps: %d, Step: %d, (%f, %f)x(%d, %d)\n", r, steps, i, double_circle[i][0], 
                    double_circle[i][1], path_circle[i][0], path_circle[i][1]);

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
        
        
        printf("#const  u8  CirclePathValues[MAX_CIRCLE_MOVEMENTS] = {32, 64, 96, 128, 160, 192, 224};\n");
        printf("#const  u8  CirclePathSteps[MAX_CIRCLE_MOVEMENTS] = {");
            for(int i=0;i<6;i++)
                if(i!=5)    
                    printf(" %d,", radius_steps[i]);
                else
                 printf(" %d ", radius_steps[i]);
        printf("};\n");
        for(int r=32;r<225;r++){
            printf("const u8 Circl2Path%d={ \n}");
            for(int steps=0;steps<radius_steps[r/32-1];steps++){
                if(steps!=radius_steps[r/32-1]-1)
                    printf("{ %d, %d}, ");
                else
                    printf("{ %d, %d} ");
            }
            printf("};\n");
        }
        
        printf("#endif\n");
    }
}
