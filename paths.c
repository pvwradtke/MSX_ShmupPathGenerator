#include <float.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <stdlib.h>
#include <stdbool.h>

#define DEBUG   false
#define PATH_STEPS      16      // Enough to move through 32 pixels at 1 pixels speed
#define PATH_ANGLES     128     // there should be 120 angles stored in the structure
#define ANGLES_PER_QUADRANT 32

/*

The targetting system requires a 16x16 pixels based lookup table. Instead of finding the actual angle, an approximation is made by taking the abselute dx and 
dy values (player to enemy) and dividin the values by 16 (>>4) until both tdx and dy are smaller than 16 (between 0 and 15). Then an index is chosen in the 
lookup table table by (dy<<4) + dx. This will be an angle between 0 to 32. Then according to quadrants, angles are adjusted:
     |
  2  |  3
-----|-----
  1  |  0 
     |

    if (dx < 0) {
        if (dy < 0) return 64 + angle;   // Top-Left
        else        return 64 - angle;   // Bottom-Left
    } else {
        if (dy < 0) return (128 - angle) & 127; // Top-Right (wraps to 127)
        else        return angle;               // Bottom-Right
    }

Viewport:
---------------> x
|
|
|
|
y

Then a lookup table with 128 different angles is created for each 2 pixels movement by frame (path_angle_lut), and a lookup table for angles is creatred by
dividing 360 degrees in 128 parts (angle_to_lut), which is useful for getting fixed angle values

*/


// Contains 64 angles, with 16 movements of 2 pixels each, with the delta stored as (dx,dy) for the 4th quadrand (315 to 360 degrees)/
// All other quadrants are calculated by swapping x by y and changing the sign
int path_angle_lut[PATH_ANGLES][PATH_STEPS][2];
uint8_t angle_to_lut[360];   // First index is the index into the Lut, the other 2 are the multiplication factor for x and y
uint8_t targeting16x16[256];
int  path_circle[7][1024][2];
double double_circle[1024][2];

int path_circle_steps[7];

void main(){
    double dx[PATH_STEPS], dy[PATH_STEPS], angle;
    int x, y, degree, prevdegree=0, prevx, prevy;

    
    for (int dy = 0; dy < 16; dy++) {
        printf("    ");
        for (int dx = 0; dx < 16; dx++) {
            int angle_index;
            
            if (dx == 0 && dy == 0) {
                // The undefined center point. Defaulting to 32 (Straight Down)
                angle_index = 32; 
            } else {
                // Calculate angle in radians
                double radians = atan2((double)dy, (double)dx);
                
                // Convert to 0-32 index and add 0.5 to round to nearest integer
                angle_index = (int)(((radians / (M_PI/2)) * ANGLES_PER_QUADRANT) + 0.5);
            }
            
            // Format output to maintain the visual grid
            targeting16x16[dx+16*dy]=angle_index;
            if(DEBUG)
                printf("Index %02d = %02d \n", dx+16*dy, angle_index);
        }
    }

    int px, py, cx, cy;
    for(int i=0;i<PATH_ANGLES;i++){
        px=0;
        py=0;
        angle = i*2*M_PI/PATH_ANGLES;
        degree=lround((180*angle)/M_PI);
        if(DEBUG)
            printf("Degree: %d \n", degree);
        // Calculates the distances from the origin (0, 0)
        for(int step=0;step<PATH_STEPS;step++){

            cx = lround(cos(angle)*((step+1)));
            path_angle_lut[i][step][0]= cx - px;
            px = cx;
            cy=lround(sin(angle)*((step+1)));
            path_angle_lut[i][step][1] = cy-py;
            py=cy;
            if(DEBUG)
               printf("Entry: %d, Angle: %d, Step: %d: (%f, %f), (%d, %d)\n", i, degree, step, cos(angle)*((step+1)), sin(angle)*((step+1)), 
                path_angle_lut[i][step][0], path_angle_lut[i][step][1]);
        }
    }
    for(int i=0;i<360;i++)
        angle_to_lut[i]=(128*i)/360; 
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
        int cx, cy, px=(r+1)*16, py=0;
        for(int i=0;i<steps;i++){
            angle = (2*M_PI*(i+1))/(double)steps;
/*            double_circle[i][0]=cos(angle)*(double)(r+1)*16;
            double_circle[i][1]=sin(angle)*(double)(r+1)*16;
            path_circle[r][i][0]=lround(double_circle[i][0]);
            path_circle[r][i][1]=lround(double_circle[i][1]);
            if(path_circle[r][i][1]%2)
                path_circle[r][i][1]+=1;*/
            double_circle[i][0]=cos(angle)*(double)(r+1)*16;
            double_circle[i][1]=sin(angle)*(double)(r+1)*16;
            cx=lround(double_circle[i][0]);
            cy=lround(double_circle[i][1]);
            path_circle[r][i][0]=cx-px;
            path_circle[r][i][1]=cy-py;
            px=cx;
            py=cy;
            if(DEBUG)
                printf("Radius: %d, Steps: %d, Step: %d, (%f, %f)x(%d, %d)\n", (r+1)*16, steps, i, double_circle[i][0], 
                    double_circle[i][1], path_circle[r][i][0], path_circle[r][i][1]);

        }
    }
    if(!DEBUG){
        // Prints the paths

        printf("#define PATH_STEPS  %d\n#define PATH_ANGLES %d\n", PATH_STEPS, PATH_ANGLES);
        printf("enum   CIRCLE_RADII    {RADIUS16, RADIUS32, RADIUS48, RADIUS64, RADIUS80, RADIUS96, RADIUS112, MAX_CIRCLE_RADII};\n\n");
        printf("typedef struct CirclePath{\n");
        printf("    i8  (*path)[2];\n");
        printf("    u8  radius;\n");
        printf("    u16 steps;\n");
        printf("}CirclePath;\n\n");

        printf("#ifndef  PATHS_H\n#define PATHS_H\n\n");

        // Print the targeting 256 bytes lookup table
        printf("// 16x16 Aiming Matrix for 128-Angle System\n");
        printf("// Layout: Left to Right (dx 0-15), Top to Bottom (dy 0-15)\n");
        printf("const u8 aim_matrix[256] = {\n");
        for (int dy = 0; dy < 16; dy++) {
            printf("    ");
            for (int dx = 0; dx < 16; dx++) {
                printf("%02d,", targeting16x16[dx+16*dy]);
            }
            printf("\n");
        }
        printf("};\n\n"); 

        

        // Print the linear PATHS
        printf("const   i8  PathAngleLUT[PATH_ANGLES][PATH_STEPS][2] ={\n\n");
        for(int i=0;i<PATH_ANGLES;i++){
            printf("    { ");
            printf("// Angle: %d\n", (360*i)/128);
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
        printf("const   u8  DegreeToPathAngleLUT[360] ={\n");
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
        printf("};\n\n");
              
        for(int r=0;r<7;r++){
            printf("const i8 CircleR%d[%d][2]={ \n", (r+1)*16, path_circle_steps[r]);
            for(int step=0;step<path_circle_steps[r];step++){
                if(step!=path_circle_steps[r]-1)
                    printf("{ %d, %d}, ", path_circle[r][step][0], path_circle[r][step][1]);
                else
                    printf("{ %d, %d} ", path_circle[r][step][0], path_circle[r][step][1]);
            }
            printf("\n};\n\n");
        }
        printf("const CirclePath     CirclePathLUT[MAX_CIRCLE_RADII]={\n");
        for(int r=0;r<7;r++){
            if(r!=6)
                printf("    { CircleR%d, %d, %d},\n",(r+1)*16, (r+1)*16, path_circle_steps[r]);
            else
                printf("    { CircleR%d, %d, %d}\n",(r+1)*16, (r+1)*16, path_circle_steps[r]);
        }
        printf("};\n\n");
        printf("#else\n\n");
        printf("extern const i8 PathAngleLUT[PATH_ANGLES][PATH_STEPS][2];\n");
        printf("extern const u8 DegreeToPathAngleLUT[360];\n");
        printf("extern const CirclePath CirclePathLUT[MAX_CIRCLE_RADII];\n\n");
        printf("#endif\n");
    }
}
