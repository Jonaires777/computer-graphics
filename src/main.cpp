#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <openglDebug.h>
#include <demoShaderLoader.h>
#include <Eigen/Dense>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "model/Point.h"
#include "model/Ray.h"
#include "model/LightSource.h"
#include "model/Objects/Sphere.h"
#include <iostream>


#define MAX_HEIGHT 500
#define MAX_WIDHT 500

#define USE_GPU_ENGINE 0
extern "C"
{
	__declspec(dllexport) unsigned long NvOptimusEnablement = USE_GPU_ENGINE;
	__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = USE_GPU_ENGINE;
}


static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GLFW_TRUE);
}

int main(void)
{

	if (!glfwInit())
		return -1;


#pragma region report opengl errors to std
	//enable opengl debugging output.
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
#pragma endregion


	//glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	//glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
	//glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); //you might want to do this when testing the game for shipping


	GLFWwindow *window = window = glfwCreateWindow(MAX_HEIGHT, MAX_WIDHT, "Sphere", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		return -1;
	}

	glfwSetKeyCallback(window, key_callback);

	glfwMakeContextCurrent(window);
	gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
	glfwSwapInterval(1);


#pragma region report opengl errors to std
	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glDebugMessageCallback(glDebugOutput, 0);
	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
#pragma endregion

	//shader loading example
	Shader s;
	s.loadShaderProgramFromFile(RESOURCES_PATH "vertex.vert", RESOURCES_PATH "fragment.frag");
	s.bind();

	GLint pixelColorLocation = glGetUniformLocation(s.id, "u_PixelColor");

	while (!glfwWindowShouldClose(window))
	{
		int wJanela = 2;
		int hJanela = 2;
		float dJanela = 5.0f;
		int nCol = MAX_WIDHT, nLin = MAX_HEIGHT;
		float rEsfera = 1.0f;
		
		Sphere sphere(Point(0.0f, 0.0f, -(dJanela + rEsfera), 1.0f), rEsfera);

		// material properties
		sphere.K_ambient = glm::vec3(0.05f, 0.01f, 0.01f);
		sphere.K_diffuse = glm::vec3(0.8f, 0.1f, 0.1f);
		sphere.K_specular = glm::vec3(0.2f, 0.2f, 0.2f);
		sphere.shininess = 8.0f;

		float Dx = wJanela / (float)nCol;
		float Dy = hJanela / (float)nLin;

		glViewport(0, 0, 500, 500);
		glClearColor(0.39f, 0.39f, 0.39f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		
		LightSource light(glm::vec3(1.0f, 1.0f, 1.0f), Point(0.0f, 3.0f, -dJanela, 1.0f));

		Point eye(0.0f, 0.0f, 0.0f, 1.0f);
		
		s.bind();

		float z = -dJanela;
		for (int l = 0; l < nLin; l++) {
			float y = hJanela / 2.0f - Dy / 2.0f - l * Dy;
			for (int c = 0; c < nCol; c++) {
				float x = -wJanela / 2 + Dx / 2.0f + c * Dx;
				Ray ray(eye, glm::vec4(x, y, z, 0.0f));
				glm::vec3 color;
				if (sphere.shade(ray, light, color)) {
					glUniform3f(pixelColorLocation, color.r, color.g, color.b);
					glBegin(GL_POINTS);
					glVertex2f(x / (wJanela / 2.0f), y / (hJanela / 2.0f));
					glEnd();
				}
			}
		}

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	//there is no need to call the clear function for the libraries since the os will do that for us.
	//by calling this functions we are just wasting time.
	//glfwDestroyWindow(window);
	//glfwTerminate();
	return 0;
}
