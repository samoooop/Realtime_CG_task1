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

class Viewport {
public:
	int w, h; // width and height
};

class Material{
public:
	vec3 ka; // Ambient color
	vec3 kd; // Diffuse color
	vec3 ks; // Specular color
	float sp; // Power coefficient of specular

	Material() : ka(0.0f), kd(0.0f), ks(0.0f), sp(0.0f) {
	}
};

class Light{
public:
	enum LIGHT_TYPE{POINT_LIGHT, DIRECTIONAL_LIGHT};

	vec3 posDir;  // Position (Point light) or Direction (Directional light)
	vec3 color;   // Color of the light
	LIGHT_TYPE type;

	Light() : posDir(0.0f), color(0.0f), type(POINT_LIGHT) {
	}
};

// Material and lights
Material material;
vector<Light> lights;

//****************************************************
// Global Variables
//****************************************************
Viewport	viewport;
int 		drawX = 0;
int 		drawY = 0;

void initScene(){
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f); // Clear to black, fully transparent

	glViewport (0,0,viewport.w,viewport.h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0,viewport.w, 0, viewport.h);
}


//****************************************************
// reshape viewport if the window is resized
//****************************************************
void myReshape(int w, int h) {
	viewport.w = w;
	viewport.h = h;

	glViewport (0,0,viewport.w,viewport.h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0, viewport.w, 0, viewport.h);

	drawX = (int)(viewport.w*0.5f);
	drawY = (int)(viewport.h*0.5f);


}

void setPixel(int x, int y, GLfloat r, GLfloat g, GLfloat b) {
	glColor3f(r, g, b);
	glVertex2f(x+0.5, y+0.5);
}

vec3 computeShadedColor(vec3 pos) {

	// TODO: Your shading code mostly go here
	vec3 viewerPosition = vec3(0,0,1);
	vec3 result = vec3(0,0,0);
    for(auto l : lights){
        //result += material.ka * l.color;
        result.r+= material.ka.r * l.color.r;
        result.g+= material.ka.g * l.color.g;
        result.b+= material.ka.b * l.color.b;
        vec3 lightDir = l.type == Light::DIRECTIONAL_LIGHT? l.posDir : l.posDir - pos;
        double dotProduct = pos.normalize() * lightDir.normalize();
        if(dotProduct > 0) {
            result.r += material.kd.r * l.color.r * dotProduct;
            result.g += material.kd.g * l.color.g * dotProduct;
            result.b += material.kd.b * l.color.b * dotProduct;
            //result += material.kd * l.color * dotProduct;
        }
        // specular
        vec3 r = (-lightDir) + (2*(lightDir.normalize()*pos.normalize())*pos);
        double specularComp = r*viewerPosition;
        if(specularComp < 0) specularComp = 0;
        specularComp = pow(specularComp,material.sp);
        result.r += material.ks.r * l.color.r * specularComp;
        result.g += material.ks.g * l.color.g * specularComp;
        result.b += material.ks.b * l.color.b * specularComp;
    }

    const float toon = 5;
    // result.g = floor(result.g * toon) / toon;
    // result.r = floor(result.r * toon) / toon;
    // result.b = floor(result.b * toon) / toon;

    float mean_luminance = (result.r + result.g + result.b)/3;
    float sub = mean_luminance - floor(mean_luminance * toon) / toon;
    result -= sub;
	return result;
}

//****************************************************
// function that does the actual drawing of stuff
//***************************************************
void myDisplay() {

	glClear(GL_COLOR_BUFFER_BIT);				// clear the color buffer

	glMatrixMode(GL_MODELVIEW);					// indicate we are specifying camera transformations
	glLoadIdentity();							// make sure transformation is "zero'd"


	int drawRadius = min(viewport.w, viewport.h)/2 - 10;  // Make it almost fit the entire window
	float idrawRadius = 1.0f / drawRadius;
	// Start drawing sphere
	glBegin(GL_POINTS);

	for (int i = -drawRadius; i <= drawRadius; i++) {
		int width = floor(sqrt((float)(drawRadius*drawRadius-i*i)));
		for (int j = -width; j <= width; j++) {

			// Calculate the x, y, z of the surface of the sphere
			float x = j * idrawRadius;
			float y = i * idrawRadius;
			float z = sqrtf(1.0f - x*x - y*y);
			vec3 pos(x,y,z); // Position on the surface of the sphere

			vec3 col = computeShadedColor(pos);

			// Set the red pixel
			setPixel(drawX + j, drawY + i, col.r, col.g, col.b);
		}
	}
	glEnd();

	glFlush();
	glutSwapBuffers();					// swap buffers (we earlier set double buffer)
}


//****************************************************
// for updating the position of the circle
//****************************************************

void myFrameMove() {
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


void parseArguments(int argc, char* argv[]) {
	int i = 1;
	while (i < argc) {
		if (strcmp(argv[i], "-ka") == 0) {
			// Ambient color
			material.ka.r = (float)atof(argv[i+1]);
			material.ka.g = (float)atof(argv[i+2]);
			material.ka.b = (float)atof(argv[i+3]);
			i+=4;
		} else
		if (strcmp(argv[i], "-kd") == 0) {
			// Diffuse color
			material.kd.r = (float)atof(argv[i+1]);
			material.kd.g = (float)atof(argv[i+2]);
			material.kd.b = (float)atof(argv[i+3]);
			i+=4;
		} else
		if (strcmp(argv[i], "-ks") == 0) {
			// Specular color
			material.ks.r = (float)atof(argv[i+1]);
			material.ks.g = (float)atof(argv[i+2]);
			material.ks.b = (float)atof(argv[i+3]);
			i+=4;
		} else
		if (strcmp(argv[i], "-sp") == 0) {
			// Specular power
			material.sp = (float)atof(argv[i+1]);
			i+=2;
		} else
		if ((strcmp(argv[i], "-pl") == 0) || (strcmp(argv[i], "-dl") == 0)){
			Light light;
			// Specular color
			light.posDir.x = (float)atof(argv[i+1]);
			light.posDir.y = (float)atof(argv[i+2]);
			light.posDir.z = (float)atof(argv[i+3]);
			light.color.r = (float)atof(argv[i+4]);
			light.color.g = (float)atof(argv[i+5]);
			light.color.b = (float)atof(argv[i+6]);
			if (strcmp(argv[i], "-pl") == 0) {
				// Point
				light.type = Light::POINT_LIGHT;
			} else {
				// Directional
				light.type = Light::DIRECTIONAL_LIGHT;
			}
			lights.push_back(light);
			i+=7;
		}
	}
}

//****************************************************
// the usual stuff, nothing exciting here
//****************************************************
int main(int argc, char *argv[]) {

	parseArguments(argc, argv);

  	//This initializes glut
  	glutInit(&argc, argv);

  	//This tells glut to use a double-buffered window with red, green, and blue channels
  	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);

  	// Initalize theviewport size
  	viewport.w = 400;
  	viewport.h = 400;

  	//The size and position of the window
  	glutInitWindowSize(viewport.w, viewport.h);
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
  	glutReshapeFunc(myReshape);					// function to run when the window gets resized
  	glutIdleFunc(myFrameMove);

  	glutMainLoop();							// infinite loop that will keep drawing and resizing and whatever else

  	return 0;
}








