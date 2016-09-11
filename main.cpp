// Simple OpenGL example for CS184 F06 by Nuttapong Chentanez, modified from sample code for CS184 on Sp06
// Modified for Realtime-CG class

#include <vector>
#include <iostream>
#include <fstream>
#include <cmath>

#define _WIN32

#ifdef _WIN32
#	include <windows.h>
#else
#	include <sys/time.h>
#endif

#ifdef OSX
#include <GLUT/glut.h>
#include <OpenGL/glu.h>
#else
#include <GL/glut.h>
#include <GL/glu.h>
#endif

#include <time.h>
#include <math.h>
#include "algebra3.h"
#include "lodepng.h"

#ifdef _WIN32
static DWORD lastTime;
#else
static struct timeval lastTime;
#endif

#define PI 3.14159265

using namespace std;

//****************************************************
// Some Classes
//****************************************************

class Viewport;

class Viewport
{
public:
    int w, h; // width and height
    int drawX;
    int drawY;
};

class Material
{
public:
    vec3 ka; // Ambient color
    vec3 kd; // Diffuse color
    vec3 ks; // Specular color
    float sp; // Power coefficient of specular

    Material() : ka(0.0f), kd(0.0f), ks(0.0f), sp(0.0f)
    {
    }
};

class Light
{
public:
    enum LIGHT_TYPE {POINT_LIGHT, DIRECTIONAL_LIGHT};

    vec3 posDir;  // Position (Point light) or Direction (Directional light)
    vec3 color;   // Color of the light
    LIGHT_TYPE type;

    Light() : posDir(0.0f), color(0.0f), type(POINT_LIGHT)
    {
    }
};

struct GlobalConfig
{
    enum SHAPE {SPHERE, CUBE};

    bool display;
    struct Shading
    {
        bool toon;
    } shading;
    struct ImageSave
    {
        bool save;
        char* filepath;
    } imageSave;
    struct Shape
    {
        SHAPE shape;
    } Shape;
};

const unsigned int RGB_COLOR_SPACE_BIT_COUNT = 3;
const unsigned int safety_res_pre_allocation = 1920*1080*RGB_COLOR_SPACE_BIT_COUNT;
vector<unsigned char> global_frame_buffer(safety_res_pre_allocation);
void getCubePixel(vector<vec3>&,vector<vec3>&);

// Material and lights
Material material;
vector<Light> lights;

//****************************************************
// Global Variables
//****************************************************
Viewport	global_viewport;

GlobalConfig globalConfig =
{
    .display=true,              // will display preview by default,
    .shading={
        .toon=false
    },
    .imageSave={
        .save=false,            // will NOT save preview by default
        .filepath=NULL
    },
    .Shape={
        .shape=GlobalConfig::SPHERE
    }
};

void gl_config_viewport(Viewport viewport)
{
    glViewport (0,0,viewport.w,viewport.h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, viewport.w, 0, viewport.h);
}

void initScene()
{
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f); // Clear to black, fully transparent

    gl_config_viewport(global_viewport);
}

void reshape_viewport(int w, int h, Viewport &viewport)
{
    viewport.w = w;
    viewport.h = h;
    viewport.drawX = (int)(viewport.w*0.5f);
    viewport.drawY = (int)(viewport.h*0.5f);
}

//****************************************************
// reshape viewport if the window is resized
//****************************************************
void gl_window_reshape(int w, int h)
{
    reshape_viewport(w, h, global_viewport);
    gl_config_viewport(global_viewport);
}

void setPixel(int x, int y, GLfloat r, GLfloat g, GLfloat b)
{
    glColor3f(r, g, b);
    glVertex2f(x+0.5, y+0.5);
}

void setPixel(int x, int y, unsigned char r, unsigned char g, unsigned char b)
{
    glColor3ub(r, g, b);
    glVertex2f(x+0.5, y+0.5);
}

vec3 computeShadedColor(vec3 pos,vec3 normal)
{
    vec3 viewerPosition = vec3(0,0,1);
    vec3 result = vec3(0,0,0);
    for(auto l : lights)
    {

        // ambient
        result.r+= material.ka.r * l.color.r;
        result.g+= material.ka.g * l.color.g;
        result.b+= material.ka.b * l.color.b;

        // diffusion
        vec3 lightDir = (l.type == Light::DIRECTIONAL_LIGHT? l.posDir : l.posDir - pos);
        float dotProduct = normal.normalize() * lightDir.normalize();
        if(dotProduct > 0)
        {
            result.r += material.kd.r * l.color.r * dotProduct;
            result.g += material.kd.g * l.color.g * dotProduct;
            result.b += material.kd.b * l.color.b * dotProduct;
        }

        // specular
        vec3 r = (-lightDir) + (2*(lightDir.normalize()*normal.normalize())*normal);
        float specularComp = r*viewerPosition;
        if(specularComp < 0) specularComp = 0;
        specularComp = pow(specularComp,material.sp);
        result.r += material.ks.r * l.color.r * specularComp;
        result.g += material.ks.g * l.color.g * specularComp;
        result.b += material.ks.b * l.color.b * specularComp;

    }

    if(globalConfig.shading.toon)
    {
        const float toon = 5;
        // result.g = floor(result.g * toon) / toon;
        // result.r = floor(result.r * toon) / toon;
        // result.b = floor(result.b * toon) / toon;

        float mean_luminance = (result.r + result.g + result.b)/3;
        float sub = mean_luminance - floor(mean_luminance * toon) / toon;
        result -= sub;
    }

    return result;
}

int renderImageToBuffer(vector<unsigned char> &frame_buffer, Viewport viewport)
{
    frame_buffer.resize( viewport.h * viewport.w * RGB_COLOR_SPACE_BIT_COUNT );
    fill(frame_buffer.begin(), frame_buffer.end(), 0);


    if(globalConfig.Shape.shape == GlobalConfig::SPHERE)
    {
        int drawRadius = min(viewport.w, viewport.h)/2 - 10;  // Make it almost fit the entire window
        float idrawRadius = 1.0f / drawRadius;
        for (int i = -drawRadius; i <= drawRadius; i++)
        {
            int width = floor(sqrt((float)(drawRadius*drawRadius-i*i)));
            for (int j = -width; j <= width; j++)
            {

                // Calculate the x, y, z of the surface of the sphere
                float x = j * idrawRadius;
                float y = i * idrawRadius;
                float z = sqrtf(1.0f - x*x - y*y);
                vec3 pos(x,y,z); // Position on the surface of the sphere

                vec3 col = computeShadedColor(pos,pos);

                // Set the pixel
//			setPixel(drawX + j, drawY + i, col.r, col.g, col.b);
                //printf("%d ", (int)(255*col.r));
                col.r = col.r > 1 ? 1 : col.r;
                col.g = col.g > 1 ? 1 : col.g;
                col.b = col.b > 1 ? 1 : col.b;

                frame_buffer[ (viewport.h -viewport.drawY - i)*viewport.w*RGB_COLOR_SPACE_BIT_COUNT + (viewport.drawX + j)*RGB_COLOR_SPACE_BIT_COUNT +0] = (char)(255*col.r);
                frame_buffer[ (viewport.h -viewport.drawY - i)*viewport.w*RGB_COLOR_SPACE_BIT_COUNT + (viewport.drawX + j)*RGB_COLOR_SPACE_BIT_COUNT +1] = (char)(255*col.g);
                frame_buffer[ (viewport.h -viewport.drawY - i)*viewport.w*RGB_COLOR_SPACE_BIT_COUNT + (viewport.drawX + j)*RGB_COLOR_SPACE_BIT_COUNT +2] = (char)(255*col.b);
            }
        }
    }
    if(globalConfig.Shape.shape == GlobalConfig::CUBE)
    {
        int drawRadius = min(viewport.w, viewport.h)/4 - 10;  // Make it almost fit the entire window
        float idrawRadius = 1.0f / drawRadius;

        vector<vec3> positions;
        vector<vec3> colors;
        getCubePixel(positions,colors);
        int error = 0;
        for(int i=0;i<positions.size();i++){
            vec3 pos = positions[i];
            vec3 col = colors[i];

            col.r = col.r > 1 ? 1 : col.r;
            col.g = col.g > 1 ? 1 : col.g;
            col.b = col.b > 1 ? 1 : col.b;

            if((viewport.h - viewport.drawY - (int)(pos.g*drawRadius)) < 0){
                continue;
            }

            frame_buffer[ (viewport.h - viewport.drawY - (int)(pos.g*drawRadius))*viewport.w*RGB_COLOR_SPACE_BIT_COUNT + (viewport.drawX + (int)(pos.r*drawRadius))*RGB_COLOR_SPACE_BIT_COUNT +0] = (char)(255*col.r);
            frame_buffer[ (viewport.h - viewport.drawY - (int)(pos.g*drawRadius))*viewport.w*RGB_COLOR_SPACE_BIT_COUNT + (viewport.drawX + (int)(pos.r*drawRadius))*RGB_COLOR_SPACE_BIT_COUNT +1] = (char)(255*col.g);
            frame_buffer[ (viewport.h - viewport.drawY - (int)(pos.g*drawRadius))*viewport.w*RGB_COLOR_SPACE_BIT_COUNT + (viewport.drawX + (int)(pos.r*drawRadius))*RGB_COLOR_SPACE_BIT_COUNT +2] = (char)(255*col.b);
        }
    }


    return 0;
}

//****************************************************
// function that does the actual drawing of stuff
//***************************************************
void myDisplay()
{

    glClear(GL_COLOR_BUFFER_BIT);				// clear the color buffer

    glMatrixMode(GL_MODELVIEW);					// indicate we are specifying camera transformations
    glLoadIdentity();							// make sure transformation is "zero'd"


    int drawRadius = min(global_viewport.w, global_viewport.h)/2 - 10;  // Make it almost fit the entire window
    float idrawRadius = 1.0f / drawRadius;

    renderImageToBuffer(global_frame_buffer, global_viewport);

    // Start drawing sphere
    glBegin(GL_POINTS);

    for(int y=0; y<global_viewport.h; y++)
    {
        for(int x=0; x<global_viewport.w; x++)
        {
            const unsigned char
            r = global_frame_buffer[ (global_viewport.h -y)*global_viewport.w*RGB_COLOR_SPACE_BIT_COUNT + (x)*RGB_COLOR_SPACE_BIT_COUNT +0],
            g = global_frame_buffer[ (global_viewport.h -y)*global_viewport.w*RGB_COLOR_SPACE_BIT_COUNT + (x)*RGB_COLOR_SPACE_BIT_COUNT +1],
            b = global_frame_buffer[ (global_viewport.h -y)*global_viewport.w*RGB_COLOR_SPACE_BIT_COUNT + (x)*RGB_COLOR_SPACE_BIT_COUNT +2];
            setPixel(x, y, r, g, b);
        }
    }

    glEnd();

    glFlush();
    glutSwapBuffers();					// swap buffers (we earlier set double buffer)
}

vec3 rotate_vec3(vec3 v,float tm[][3])
{
    float vin[3] = {v.r,v.g,v.b};
    float r[3] = {0,0,0};
    for(int i=0; i<3; i++)
    {
        for(int j=0; j<3; j++)
        {
            r[i] += vin[j] * tm[i][j];
        }
    }
    return vec3(r[0],r[1],r[2]);
}

void getCubePixel(vector<vec3>& positions,vector<vec3>& colors)
{
    int drawRadius = min(global_viewport.w, global_viewport.h)/4 - 10;  // Make it almost fit the entire window
    float idrawRadius = 1.0f / drawRadius;
    // Start drawing sphere
    float sin45 = 1.0f/sqrt(2);

    float tranformation_matrix[3][3] =
    {
        { sin45,sin45,0},  // this matrix for rotation
        {-sin45,sin45,0},
        {0,0,1}
    };

    float tranformation_matrix2[3][3] =
    {
        {1,0,0},  // this matrix for rotation
        {0,sin45,-sin45},
        {0,sin45,sin45}
    };
    vec3 side_normal(0,0,1);
    side_normal = rotate_vec3(side_normal,tranformation_matrix);
    side_normal = rotate_vec3(side_normal,tranformation_matrix2);
    for (int i = -drawRadius; i <= drawRadius; i++)
    {
        for (int j = -drawRadius; j <= drawRadius; j++)
        {
            // Calculate the x, y, z of the surface of the sphere
            float x = j * idrawRadius;
            float y = i * idrawRadius;
            float z = 1;
            vec3 pos(x,y,z);
            pos = rotate_vec3(pos,tranformation_matrix); // Position on the surface of the sphere
            pos = rotate_vec3(pos,tranformation_matrix2);
            vec3 col = computeShadedColor(pos,side_normal);
            positions.push_back(pos);
            colors.push_back(col);
            // Set the red pixel
            //setPixel(global_viewport.drawX + pos.r*drawRadius, global_viewport.drawY + pos.g*drawRadius, col.r, col.g, col.b);
        }
    }

    side_normal = vec3(-1,0,0);
    side_normal = rotate_vec3(side_normal,tranformation_matrix);
    side_normal = rotate_vec3(side_normal,tranformation_matrix2);
    for (int i = -drawRadius; i <= drawRadius; i++)
    {
        for (int j = -drawRadius; j <= drawRadius; j++)
        {
            // Calculate the x, y, z of the surface of the sphere
            float x = -1;
            float y = i * idrawRadius;
            float z = j * idrawRadius;
            vec3 pos(x,y,z);
            pos = rotate_vec3(pos,tranformation_matrix); // Position on the surface of the sphere
            pos = rotate_vec3(pos,tranformation_matrix2);
            vec3 col = computeShadedColor(pos,side_normal);
            positions.push_back(pos);
            colors.push_back(col);
            // Set the red pixel
            //setPixel(global_viewport.drawX + pos.r*drawRadius, global_viewport.drawY + pos.g*drawRadius, col.r, col.g, col.b);
        }
    }

    side_normal = vec3(0,1,0);
    side_normal = rotate_vec3(side_normal,tranformation_matrix);
    side_normal = rotate_vec3(side_normal,tranformation_matrix2);
    for (int i = -drawRadius; i <= drawRadius; i++)
    {
        for (int j = -drawRadius; j <= drawRadius; j++)
        {
            // Calculate the x, y, z of the surface of the sphere
            float x = i * idrawRadius;
            float y = 1;
            float z = j * idrawRadius;
            vec3 pos(x,y,z);
            pos = rotate_vec3(pos,tranformation_matrix); // Position on the surface of the sphere
            pos = rotate_vec3(pos,tranformation_matrix2);
            vec3 col = computeShadedColor(pos,side_normal);
            positions.push_back(pos);
            colors.push_back(col);
            // Set the red pixel
            //setPixel(global_viewport.drawX + pos.r*drawRadius, global_viewport.drawY + pos.g*drawRadius, col.r, col.g, col.b);
        }
    }
}

//****************************************************
// for updating the position of the circle
//****************************************************

void myFrameMove()
{
    float dt;
    // Compute the time elapsed since the last time the scence is redrawn
#ifdef _WIN32
    DWORD currentTime = GetTickCount();
    dt = (float)(currentTime - lastTime)*0.001f;
#else
    timeval currentTime;
    gettimeofday(&currentTime, NULL);
    dt = (float)((currentTime.tv_sec - lastTime.tv_sec) + 1e-6*(currentTime.tv_usec - lastTime.tv_usec));
#endif

    // Store the time
    lastTime = currentTime;
    glutPostRedisplay();
}


void parseArguments(int argc, char* argv[])
{
    int i = 1;
    while (i < argc)
    {
        if (strcmp(argv[i], "-ka") == 0)
        {
            // Ambient color
            material.ka.r = (float)atof(argv[i+1]);
            material.ka.g = (float)atof(argv[i+2]);
            material.ka.b = (float)atof(argv[i+3]);
            i+=4;
        }
        else if (strcmp(argv[i], "-kd") == 0)
        {
            // Diffuse color
            material.kd.r = (float)atof(argv[i+1]);
            material.kd.g = (float)atof(argv[i+2]);
            material.kd.b = (float)atof(argv[i+3]);
            i+=4;
        }
        else if (strcmp(argv[i], "-ks") == 0)
        {
            // Specular color
            material.ks.r = (float)atof(argv[i+1]);
            material.ks.g = (float)atof(argv[i+2]);
            material.ks.b = (float)atof(argv[i+3]);
            i+=4;
        }
        else if (strcmp(argv[i], "-sp") == 0)
        {
            // Specular power
            material.sp = (float)atof(argv[i+1]);
            i+=2;
        }
        else if ((strcmp(argv[i], "-pl") == 0) || (strcmp(argv[i], "-dl") == 0))
        {
            Light light;
            // Specular color
            light.posDir.x = (float)atof(argv[i+1]);
            light.posDir.y = (float)atof(argv[i+2]);
            light.posDir.z = (float)atof(argv[i+3]);
            light.color.r = (float)atof(argv[i+4]);
            light.color.g = (float)atof(argv[i+5]);
            light.color.b = (float)atof(argv[i+6]);
            if (strcmp(argv[i], "-pl") == 0)
            {
                // Point
                light.type = Light::POINT_LIGHT;
            }
            else
            {
                // Directional
                light.type = Light::DIRECTIONAL_LIGHT;
            }
            lights.push_back(light);
            i+=7;
        }
        else if (strcmp(argv[i], "-no-display") == 0)
        {
            globalConfig.display = false;
            i+=1;
        }
        else if (strcmp(argv[i], "-save") == 0)
        {
            globalConfig.imageSave.save = true;
            globalConfig.imageSave.filepath = argv[i+1];
            i+=2;
        }
        else if (strcmp(argv[i], "-toon") == 0)
        {
            globalConfig.shading.toon = true;
            i+=1;
        }
        else if (strcmp(argv[i], "-cube") == 0)
        {
            globalConfig.Shape.shape = GlobalConfig::CUBE;
            i+=1;
        }
        else
        {
            printf("INVALID ARGUMENT : %s\n", argv[i]);
            i++;
        }
    }
}

int initWindow(int argc, char *argv[])
{
    //This initializes glut
    glutInit(&argc, argv);

    //This tells glut to use a double-buffered window with red, green, and blue channels
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);

    //The size and position of the window
    glutInitWindowSize(global_viewport.w, global_viewport.h);
    glutInitWindowPosition(0,0);
    glutCreateWindow(argv[0]);

    // Initialize timer variable
#ifdef _WIN32
    lastTime = GetTickCount();
#else
    gettimeofday(&lastTime, NULL);
#endif

    initScene();							// quick function to set up scene

    glutDisplayFunc(myDisplay);					// function to run when its time to draw something
    glutReshapeFunc(gl_window_reshape);					// function to run when the window gets resized
    glutIdleFunc(myFrameMove);

    return 0;
}

int saveBufferToFile(vector<unsigned char> &frame_buffer, char filepath[], Viewport viewport)
{
    return lodepng_encode24_file(filepath, &frame_buffer[0], viewport.w, viewport.h);
}

//****************************************************
// the usual stuff, nothing exciting here
//****************************************************
int main(int argc, char *argv[])
{

    parseArguments(argc, argv);

    reshape_viewport(400, 400, global_viewport);

    if( globalConfig.imageSave.save )
    {
        renderImageToBuffer(global_frame_buffer, global_viewport);
        printf("File saved to %s", globalConfig.imageSave.filepath);
        saveBufferToFile(global_frame_buffer, globalConfig.imageSave.filepath, global_viewport);
    }

    if( globalConfig.display )
    {
        initWindow(argc, argv);
        glutMainLoop();							// infinite loop that will keep drawing and resizing and whatever else
    }
    return 0;
}
