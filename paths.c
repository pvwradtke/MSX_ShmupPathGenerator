#include <float.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <stdlib.h>

#define DEBUG   false
#define PATH_STEPS      16      // Enough to move through 32 pixels at 2 pixels speed
#define PATH_DIVISIONS  16      // Enough to give us a 3 degrees precision
#define OCTANT          15
#define PATH_ANGLES     (OCTANT*8)     // there should be 120 angles stored in the structure

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
int path_angle_lut[PATH_ANGLES][PATH_STEPS][2];
uint8_t angle_to_lut[360];   // First index is the index into the Lut, the other 2 are the multiplication factor for x and y
int  path_circle[7][1024][2];
double double_circle[1024][2];

int path_circle_steps[7];

void main(){
    double dx[PATH_STEPS], dy[PATH_STEPS], angle;
    int x, y, degree, prevdegree=0, prevx, prevy;
    for(int i=0;i<PATH_ANGLES;i++){
        angle = i*2*M_PI/120;
        degree=lround((180*angle)/M_PI);
        if(DEBUG)
            printf("Degree: %d \n", degree);
        // Calculates the distances from the origin (0, 0)
        for(int step=0;step<PATH_STEPS;step++){
            path_angle_lut[i][step][0]=lround(cos(angle)*((step+1)*2));
            y=lround(sin(angle)*((step+1)*2));
            if(y>=0){
                path_angle_lut[i][step][1] = y%2 ? y+1 : y; 
            }else{
                path_angle_lut[i][step][1] = abs(y)%2 ? y-1 : y;
            }
            if(DEBUG)
               printf("Entry: %d, Angle: %f, Step: %d: (%f, %f), (%d, %d)\n", i, angle, step, cos(angle)*((step+1)*2), sin(angle)*((step+1)*2), 
                path_angle_lut[i][step][0], path_angle_lut[i][step][1]);
        }
    }
    for(int i=0;i<360;i++)
        angle_to_lut[i]=i/3;
    if(DEBUG)
        for(int i=0;i<360;i++)
            printf("Angle: %d, LUT: %d\n", i, angle_to_lut[i]);
    // Calculates the circle movements, for several radii: 32, 64, 96, 128, 160, 192, 224
    // Does it in the first 45 degrees in the 4th quadrant, and other quadrants and complementing 45 degrees are calculated by changing signs, and dx by dy
    int radius_steps[6];
    for(int r=0; r<7; r++){
        // Calculates the perimeter for 360 degrees of the circle
        double perimeter = 2*M_PI*(r+1)*16;
        // We move two pixels distance each time, then we have perimeter/2 steps to do (the character is already at the initial position)
        int steps=lround(perimeter/2.0);
        if(steps%4)
            steps=steps+(4-steps%4);
        path_circle_steps[r]=steps;
        // Calculate the double values
        for(int i=0;i<steps;i++){
            angle = (2*M_PI*(i+1))/(double)steps;
            double_circle[i][0]=cos(angle)*(double)r;//*2*(i+1);
            double_circle[i][1]=sin(angle)*(double)r;//*2*(i+1);
            path_circle[r][i][0]=lround(double_circle[i][0]);
            path_circle[r][i][1]=lround(double_circle[i][1]);
            if(path_circle[r][i][1]%2)
                path_circle[r][i][1]+=1;
            if(DEBUG)
                printf("Radius: %d, Steps: %d, Step: %d, (%f, %f)x(%d, %d)\n", r, steps, i, double_circle[i][0], 
                    double_circle[i][1], path_circle[r][i][0], path_circle[r][i][1]);

        }
    }
    if(!DEBUG){
        // Prints the paths

        printf("#define PATH_STEPS  16\n#define PATH_SLICES 120\n");
        printf("#enum   CIRCLE_RADII    {RADIUS16, RADIUS32, RADIUS48, RADIUS64, RADIUS80, RADIUS96, RADIUS112, MAX_CIRCLE_RADII};\n\n");
        
        printf("#ifndef  PATHS_H\n#define PATHS_H\n\n");

        // Print the linear PATHS
        printf("const   i8  PathAngleLut[PATH_SLICES][PATH_STEPS][2] ={\n\n");
        for(int i=0;i<PATH_ANGLES;i++){
            printf("    { ");
            printf("// Angle: %d\n", i*3);
            for(int j=0;j<PATH_STEPS;j++){
                printf("{ %d, %d}", path_angle_lut[i][j][0], path_angle_lut[i][j][1]);
                if(j!=PATH_STEPS-1)
                    printf(", ");
            }
            if(i!=PATH_ANGLES-1)
                printf("},\n");
            else
                printf("}\n");
        }
        printf("};\n\n");
        printf("const   u8  DegreeToPathLut[360] ={\n");
        for(int i=0;i<18;i++){
            printf("    ");
            for(int j=0;j<20;j++){
                if(i==17 && j==19)
                    printf("%3d", angle_to_lut[i*20+j]);
                else
                    printf("%3d, ", angle_to_lut[i*20+j]);
            }
            printf("\n");
        }
        printf("},\n\n");
        
        
        printf("const  u8  CirclePathRadius[MAX_CIRCLE_RADII] = {32, 64, 96, 128, 160, 192, 224};\n");
        printf("const  u8  CirclePathSteps[MAX_CIRCLE_RADII] = {");
        for(int i=0; i<7;i++)
            if(i!=6)
                printf(" %d,", path_circle_steps[i]);
            else
                printf(" %d ", path_circle_steps[i]);
        printf("};\n");
        
        for(int r=0;r<7;r++){
            printf("const i8 CirclePathLut%d[%d]={ \n}", (r+1)*16, path_circle_steps[r]);
            for(int step=0;step<path_circle_steps[r];step++){
                if(step!=path_circle_steps[r]-1)
                    printf("{ %d, %d}, ", path_circle[r][step][0], path_circle[r][step][1]);
                else
                    printf("{ %d, %d} ", path_circle[r][step][0], path_circle[r][step][1]);
            }
            printf("};\n\n");
        }
        printf("const i8 *CirclePathLutReferences[MAX_CIRCLE_RADII]={\n");
        for(int r=0;r<7;r++){
            if(r!=6)
                printf("    CirclePath%d,\n",(r+1)*16);
            else
                printf("    CirclePath%d\n",(r+1)*16);
        }
        printf("};\n\n");
        printf("#else\n\n");
        printf("extern const i8 PathAngleLut[PATH_SLICES][PATH_STEPS][2];\n");
        printf("extern const u8 DegreeToPathLut[360];\n");
        printf("extern const u8 CirclePathRadius[MAX_CIRCLE_RADII];\n");
        printf("extern const u8 CirclePathSteps[MAX_CIRCLE_RADII];\n");
        for(int r=0;r<7;r++)
            printf("extern const i8 CirclePathLut%d[%d];\n", (r+1)*16, path_circle_steps[r]);
        printf("extern const i8 *CirclePathLutReferences[MAX_CIRCLE_RADII];\n\n");

        printf("#endif\n");
    }
}
