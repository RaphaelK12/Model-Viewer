// Author: Neil Berard. External code and licenses included in the project directory and git submodules.
// This project is intended for learning opengl and not for any real-world applications. Use at your own risk. 


///////////////////// TEST CODE ///////////////////////

//#include "Model.h"
//#include <iostream>
//
//int main()
//{
//	printf("Howdy!");
//	std::cin.get();
//	return 1;
//}

///////////////////// END TEST CODE ///////////////////////


#include "Debugging.h"
#include <iostream>
#include <gl/glew.h>
#include <GLFW/glfw3.h>
#include "Debugging.h"
#include "Model.h"
#include "Shader.h"
#include "glm/gtc/matrix_transform.hpp"
#include "imgui/imgui.h"
#include "imgui/examples/imgui_impl_glfw.h"
#include "imgui/examples/imgui_impl_opengl3.h"
#include <windows.h>
#include <iostream>
#include "Camera.h"
#include "Texture.h"
#include "Log.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif




// UI STATE
namespace Scene
{
	//TODO: Add active camera index.
	Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
	//std::vector<Camera*>cameras = { new Camera(glm::vec3(0.0f, 0.0f, 3.0f)) };
}


namespace Lights
{
	glm::vec4 lightColorA = glm::vec4(1.0f);
	
}

namespace Shading
{
	int mode = 0;
	glm::vec4 diffuseColor = glm::vec4(0.5f);
	float specIntensity = 1.0f;
}



enum DrawMode {
	// Debug shaders
	draw_normal, draw_lit, draw_outline, draw_wireframe_on_shaded
};


const GLuint WIDTH = 1080, HEIGHT = 720;
int SCREEN_WIDTH, SCREEN_HEIGHT;
GLfloat deltaTime = 0.0f;
GLfloat lastFrame = 0.0f;
bool keys[1024];
bool firstMouse = true;
bool processMouse = false;
bool debugDraw = false;
bool wireFrameOnShaded = false;
float far_dist;
float near_dist;
DrawMode drawMode = draw_normal;
Model assimpModel;


// Returns an empty string if dialog is canceled
const char* pfilter = "All Files (*.*)\0*.*\0";

std::string openfilename(const char *filter) {
	OPENFILENAME ofn;
	char fileName[MAX_PATH] = "";
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = NULL;
	ofn.lpstrFilter = (char*)filter;
	ofn.lpstrFile = fileName;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
	ofn.lpstrDefExt = "";
	std::string fileNameStr;
	if (GetOpenFileName(&ofn))
	{
		fileNameStr = fileName;
	}
	return fileNameStr;
}
//int main() {
//	cout << openfilename(pfilter).c_str();
//	cin.ignore();
//}

std::string getexepath()
{
	char result[MAX_PATH];
	return std::string(result, GetModuleFileName(NULL, result, MAX_PATH));
}


void loadFBX()
{
	std::string fileNameStr = openfilename(pfilter).c_str();
	assimpModel = Model(fileNameStr.c_str());
	LOG_DEBUG("DONE LOADING ASSIMP");
}


void ScreenResize(int width, int height, glm::mat4 &proj)
{
	float ratio = float(width) / float(height);

	proj = glm::ortho(-1.0f * ratio, // Left 
		1.0f * ratio, // right
		-1.0f, // bottom 
		1.0f, // Top
		-5.0f, // near 
		5.0f); // far
}

void KeyCallback(GLFWwindow *window, int key, int scanecode, int action, int mode);
void ScrollCallback(GLFWwindow *window, double xOffset, double yOffset);
void MouseCallback(GLFWwindow *window, double xPos, double yPos);
void DisplayCallback();
void DoMovement();
void RenderUI();

GLfloat lastX = WIDTH / 2.0f;
GLfloat lastY = WIDTH / 2.0f;


static bool drawLights = true;

// DRAW MODE OPTIONS TODO: figure out how to convert this to an enum
const char* items[]{ "Diffuse", "Normal", "Wireframe", "ZDepth" };



int main(void)
{
	Log::Init();

	//Log::set_level();
	LOG_INFO("Initialized Log!");


	int width = 1080;
	int height = 720;

	GLFWwindow* window;

	/* Initialize the library */
	if (!glfwInit())
		return -1;

	/* Create a windowed mode window and its OpenGL context */
	window = glfwCreateWindow(WIDTH, HEIGHT, "NB Model Viewer", NULL, NULL);
	if (!window)
	{
		LOG_INFO("Closing Window");
		glfwTerminate();
		return -1;
	}


	/* Make the window's context current */
	glfwMakeContextCurrent(window);
	glfwGetFramebufferSize(window, &SCREEN_WIDTH, &SCREEN_HEIGHT);

	glfwSetKeyCallback(window, KeyCallback);
	glfwSetCursorPosCallback(window, MouseCallback);
	glfwSetScrollCallback(window, ScrollCallback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

	if (glewInit() != GLEW_OK)
	{
		//Log.log("Glew Failed!");

	}
	LOG_DEBUG("Running OPENGL {}", glGetString(GL_VERSION));

	Shader shader("../../resources/shaders/lambert.glsl");
	LightCube lightCube;
	Shader lightShader("../../resources/shaders/lambert.glsl");

	// GET EXE DIRECTORY
	std::string exePath = getexepath();
	LOG_DEBUG("\n EXE PATH:    {}", exePath.c_str());

	glm::uvec4 vp(1.0f, 1.0f, 0.0f, 1.0f);
	glm::mat4 view = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));


	glm::vec3 rotAxis(0.0f, 1.0f, 0.0f);
	glm::vec3 yup(1.0f, 0.0f, 0.0f);
	glm::mat4 tMatrix = glm::mat4(1.0f);

	float r = 1.0f;
	float a = 1.0f;
	float increment = 0.05f;


	// Setup Dear ImGui context
	const char* glsl_version = "#version 130";

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsClassic();

	// Setup Platform/Renderer bindings
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init(glsl_version);

	float scale = 1.0f;
	float spinSpeed = .01f;
	float pulseSpeed = .01f;
	glm::vec3 translate = glm::vec3(0.0f, 0.0f, 0);

	// Light pos 
	glm::mat4 lightScale = glm::scale(glm::mat4(1.0f), glm::vec3(0.2f));
	glm::vec3 lightTranslate = glm::vec3(0.0f, 0.0f, 0.0f);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);


	/* Loop until the user closes the window */
	while (!glfwWindowShouldClose(window))
	{
		const glm::vec3 cpos = Scene::camera.Position();
		//LOG_DEBUG("Position X {} Y {} Z {}", cpos.x, cpos.y, cpos.z);

		/* Render here */
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		DoMovement();

		GLfloat currentFrame = (GLfloat)glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		glCullFace(GL_BACK);
		glEnable(GL_CULL_FACE);

		// DRAW MESH //
		shader.Bind();
		glm::mat4 proj = glm::perspective(Scene::camera.GetZoom(), (GLfloat)SCREEN_WIDTH / (GLfloat)SCREEN_HEIGHT, 0.1f, 1000.0f);
		view = Scene::camera.GetViewMatrix();

		// Diffuse
		if (Shading::mode == 0)
		{
			shader.SetUniform4f("u_LightPosA", lightTranslate.x, lightTranslate.y, lightTranslate.z, 1.0f);
			shader.SetUniform4f("u_LightColorA", Lights::lightColorA.x, Lights::lightColorA.y, Lights::lightColorA.z, 1.0f);
			shader.SetUniform4f("u_Color", Shading::diffuseColor.x, Shading::diffuseColor.y, Shading::diffuseColor.z, Shading::diffuseColor.w);
			shader.SetUniform4f("u_CameraPos", Scene::camera.Position().x, Scene::camera.Position().y, Scene::camera.Position().z, 1.0f);
			shader.SetUniform1i("u_Wireframe", 0);
			shader.SetUniform1i("u_DrawMode", 0);
			shader.SetUniform1f("u_SpecIntensity", Shading::specIntensity);
			glPolygonMode(GL_FRONT, GL_FILL);
			assimpModel.Draw();
		}

		if (Shading::mode == 1)
		{
			shader.SetUniform1i("u_DrawMode", 1);
			shader.SetUniform1i("u_Wireframe", 0);
			glPolygonMode(GL_FRONT, GL_FILL);
			assimpModel.Draw();
		}
		// Wireframe on shaded
		if (Shading::mode == 2 || wireFrameOnShaded)
		{
			shader.SetUniform1i("u_DrawMode", 2);
			shader.SetUniform1i("u_Wireframe", 1);
			shader.SetUniform4f("u_Color", 1.0f, 1.0f, 1.0f, 1.0f);
			glPolygonMode(GL_FRONT, GL_LINE);
			assimpModel.Draw();
		}

		// ZDepth
		if (Shading::mode == 3)
		{
			shader.SetUniform1i("u_DrawMode", 3);
			shader.SetUniform1i("u_Wireframe", 0);
			shader.SetUniform1f("u_Far", .13);
			shader.SetUniform1f("u_Near", .002);
			shader.SetUniform4f("u_Color", 1.0f, 1.0f, 1.0f, 1.0f);
			glPolygonMode(GL_FRONT, GL_FILL);
			assimpModel.Draw();
		}

		

		if (r > 1.0f)
		{
			increment = -1.0f * pulseSpeed;
		}
		else if (r < 0.0f)
			increment = 1.0f * pulseSpeed;

		r += increment;

		a += spinSpeed;

		shader.SetUniform4f("u_Color", 0.5f, 0.5f, 0.5f, 1.0f);

		glm::mat4 myrot = glm::rotate(glm::mat4(1.0f), a, glm::normalize(rotAxis));
		glm::mat4 model = glm::translate(glm::mat4(1.0f), translate);
		//glm::mat4 myrot2 = glm::rotate(glm::mat4(1.0f), a, glm::vec3(0.0f, 1.0f, 0.0f));
		glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(scale, scale, scale));
		glm::mat4 mvp = proj * view * model * scaleMatrix * myrot;

		//shader.SetUniformMat4f("u_MVP", tMatrix * myrot * myrot2 * proj);
		shader.SetUniformMat4f("u_MVP", mvp);
		shader.SetUniformMat4f("u_ModelMatrix", myrot);


		/////////////////  Light Cube /////////////////

		glm::mat4 lightModel = glm::translate(glm::mat4(1.0f), lightTranslate);
		if (drawLights)
		{
			//LOG_DEBUG("Light tanslate x:{}, y:{}, z:{}", lightTranslate.x, lightTranslate.y, lightTranslate.z);
			lightShader.Bind();
			lightShader.SetUniform1i("u_DrawMode", 4);
			lightShader.SetUniform4f("u_LightPosA", lightTranslate.x, lightTranslate.y, lightTranslate.z, 1.0f);
			lightShader.SetUniform4f("u_Color", Lights::lightColorA.x, Lights::lightColorA.y, Lights::lightColorA.z, 1.0f);
			lightShader.SetUniformMat4f("u_MVP", proj * view * lightModel * lightScale);
			lightShader.SetUniformMat4f("u_ModelMatrix", model);
			glPolygonMode(GL_FRONT, GL_FILL);
			lightCube.draw();
		}

		/////////////////  IMGUI  /////////////////

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		{

			ImGui::Begin(" ");                          // Create a window called "Hello, world!" and append into it.
			if (ImGui::Button("Load FBX"))
			{
				loadFBX();
			}
			ImGui::Checkbox("Wireframe on shaded", &wireFrameOnShaded);
			ImGui::Combo("Draw Mode", &Shading::mode, items, IM_ARRAYSIZE(items));
			ImGui::SliderFloat3("Diffuse Color", &Shading::diffuseColor.x, 0.0f, 1.0f);
			ImGui::SliderFloat("Spec Intensity", &Shading::specIntensity, 0.0f, 0.5f);

			ImGui::SliderFloat2("Move", &translate.x, -1.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
			ImGui::SliderFloat("Zoom", &scale, 0.0f, 4.0f);
			ImGui::SliderFloat3("Spin", &rotAxis.x, 0.0f, 1.0f);
			ImGui::SliderFloat("SpinSpeed", &spinSpeed, 0.0f, 0.05f);
			ImGui::SliderFloat("ZNear", &near_dist, 0.001f, 0.1f);
			ImGui::SliderFloat("ZFar", &far_dist, 0.001f, 1.0f);
			ImGui::Separator();

			ImGui::SliderFloat4("Light Color", &Lights::lightColorA.x, 0.0f, 1.0f);
			ImGui::SliderFloat3("LightPos", &lightTranslate.x, -2.0f, 2.0f);

			ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
			ImGui::End();
		}

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		/* Swap front and back buffers */
		glfwSwapBuffers(window);

		/* Poll for and process events */
		glfwPollEvents();

	}

	LOG_INFO("Shutting down");
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	return 0;
}

void DoMovement()
{
	if (keys[GLFW_KEY_W] || keys[GLFW_KEY_UP])
	{
		Scene::camera.ProcessKeyboard(FORWARD, deltaTime);
	}

	if (keys[GLFW_KEY_S] || keys[GLFW_KEY_DOWN])
	{
		Scene::camera.ProcessKeyboard(BACKWARD, deltaTime);
	}

	if (keys[GLFW_KEY_A] || keys[GLFW_KEY_LEFT])
	{
		Scene::camera.ProcessKeyboard(LEFT, deltaTime);
	}

	if (keys[GLFW_KEY_D] || keys[GLFW_KEY_RIGHT])
	{
		Scene::camera.ProcessKeyboard(RIGHT, deltaTime);
	}
}

void KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mode)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(window, GL_TRUE);
	}

	if (key == GLFW_KEY_1 && action == GLFW_PRESS)
	{
		std::string fileNameStr = openfilename(pfilter).c_str();
		assimpModel = Model(fileNameStr.c_str());
	}

	// PROCESS MOUSE
	if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
	{
		if (processMouse)
		{
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			processMouse = false;
		}
		else
		{
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			processMouse = true;
		}
	}


	if (key >= 0 && key < 1024)
	{
		if (GLFW_PRESS == action)
		{
			keys[key] = true;
		}
		else if (GLFW_RELEASE == action)
		{
			keys[key] = false;
		}

	}

}

void MouseCallback(GLFWwindow *window, double pXPos, double pYPos)
{
	if (!processMouse)
	{
		return;
	}

	GLfloat xPos = GLfloat(pXPos);
	GLfloat yPos = GLfloat(pYPos);


	if (firstMouse)
	{
		lastX = xPos;
		lastY = yPos;
		firstMouse = false;
	}


	GLfloat xOffset = xPos - lastX;
	GLfloat yOffset = yPos - lastY;

	lastX = xPos;
	lastY = yPos;

	Scene::camera.ProcessMouseMovement(xOffset, yOffset * -1.0f);

}

void ScrollCallback(GLFWwindow *window, double xOffset, double pYOffset)
{
	GLfloat yOffset = GLfloat(pYOffset);
	Scene::camera.ProcessMouseScroll(yOffset);
}

//TODO: Move while loop logic here and use and register this callback
void DisplayCallback()
{


}

void RenderUI()
{





}