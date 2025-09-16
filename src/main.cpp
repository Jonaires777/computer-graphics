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
#include "model/Sphere.h"
#include <iostream>


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


	GLFWwindow *window = window = glfwCreateWindow(1280, 720, "Sphere", NULL, NULL);
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

	while (!glfwWindowShouldClose(window))
	{
		int wJanela = 1280;  
		int hJanela = 720;
		float dJanela = 5.0f;
		int nCol = 1280, nLin = 720;
		float rEsfera = 6000.0f;

		Sphere sphere(Point(0.0f, 0.0f, -(dJanela + rEsfera), 1.0f), rEsfera);

		float Dx = wJanela / (float)nCol;
		float Dy = hJanela / (float)nLin;

		glfwGetFramebufferSize(window, &wJanela, &hJanela);
		glViewport(0, 0, wJanela, hJanela);
		glClearColor(0.39f, 0.39f, 0.39f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		
		Point eye(0.0f, 0.0f, 0.0f, 1.0f);
		
		float z = -dJanela;
		for (int l = 0; l < nLin; l++) {
			float y = hJanela / 2.0f - Dy / 2.0f - l * Dy;
			for (int c = 0; c < nCol; c++) {
				float x = -wJanela / 2 + Dx / 2.0f + c * Dx;
				Ray ray(eye, glm::vec4(x, y, z, 0.0f));
				if (sphere.intersects(ray)) {
					glColor3f(1.0f, 0.0f, 0.0f);
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
