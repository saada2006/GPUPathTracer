#include <iostream>
#include <stdio.h>
#include "misc/Window.h"
#include "core/Renderer.h"
#include "core/Buffer.h"
#include "core/VertexArray.h"
#include "core/Shader.h"
#include "core/Texture.h"
#include "math/Camera.h"
#include "misc/TimeUtil.h"
#include "core/Scene.h"
#include <SOIL2.h>
#include <ctime>
#include <string>
#include <fstream>
#ifdef _WIN32
#include <Windows.h>
#endif

constexpr bool lockCamera = false ;

uint32_t Width = 1280;
uint32_t Height = 720;

// Camera params 
constexpr float kCameraSetting = 0.1f;

constexpr float CameraSpeed = 2000.000f * 0.5f * kCameraSetting;
constexpr float CameraSensitivity = 0.001f;
glm::vec2 LastCursorPosition;


// I need class here because Intellisense is not detecting the camera type
Camera camera((float)Width / Height, glm::radians(45.0f),  900.0f * kCameraSetting, 0.0f * 5.0f * kCameraSetting);

bool needResetSamples = false;

void MouseCallback(GLFWwindow* Window, double X, double Y) {
	if (lockCamera) return;
	glm::vec2 CurrentCursorPosition = glm::vec2(X, Y);

	glm::vec2 DeltaPosition = CurrentCursorPosition - LastCursorPosition;

	DeltaPosition.y = -DeltaPosition.y;
	//DeltaPosition.x = -DeltaPosition.x;

	DeltaPosition *= CameraSensitivity;

	camera.AddRotation(glm::vec3(DeltaPosition, 0.0f));

	LastCursorPosition = CurrentCursorPosition;
	needResetSamples = true;
}

int main(int argc, char** argv) {

#if _WIN32
	SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_AWAYMODE_REQUIRED); // prevent our program from sleeping on windows for long renders
#endif
	std::cout << "Working Directory: " << argv[0] << '\n';

	Window Window;
	Window.Open("OpenGL Light Transport", Width, Height, false);
	Window.SetVisibility(false);

	LastCursorPosition = glm::vec2(Width, Height) / 2.0f;
	Window.SetInputCallback(MouseCallback);

	Renderer* renderer = new Renderer;
	{
		std::ifstream scene_input;
		scene_input.open("scene.txt");

		std::string path, skybox;
		std::getline(scene_input, path);
		std::getline(scene_input, skybox);
		vec3 cam, rot;
		scene_input >> cam.x >> cam.y >> cam.z;
		scene_input >> rot.x >> rot.y >> rot.z;

		renderer->Initialize(&Window, path.c_str(), skybox);
		camera.SetPosition(cam);
		camera.SetRotation(rot);
	}

	//renderer->Initialize(&Window, "res/conference/conference.obj", "GENERATE COLOR WHITE"); // // salle_de_bain.obj //res/sky/ibl/NarrowPath_3k.hdr // Topanga_Forest_B_3k
	//camera.SetPosition(vec3(-4.90816927, 3.45465946f, 2.58675551));
	//camera.SetRotation(vec3(1.09920430, -0.0669997707, 0.0f));
	//camera.SetPosition(vec3(-543.169373, 392.132965, -799.786865));
	//camera.SetRotation(vec3(2.11920309, -0.0949998572, 0.0f));
	//camera.SetPosition(glm::vec3(6.0f, 2.0f, 0.0f));
	//camera.SetRotation(glm::vec3(glm::radians(-90.0f), 0.0f, 0.0f));
	//camera.SetPosition(glm::vec3(-0.25f, 2.79f, 6.0f));
	//camera.SetPosition(glm::vec3(-4.98805332, 1.38741374, 10.1879292));
	//camera.SetRotation(glm::vec3(0.724999964, -0.0800005496, 0.0));

	Timer FrameTimer;

	Window.SetVisibility(true);

	int numFrames = 0;
	auto start = GetCurrentTimeNano64();
	while (!Window.ShouldClose()) {
		FrameTimer.Begin();

		if (Window.GetKey(GLFW_KEY_W)) {
			camera.Move(CameraSpeed * (float)FrameTimer.Delta);
			needResetSamples = true;
		}
		else if (Window.GetKey(GLFW_KEY_S)) {
			camera.Move(-CameraSpeed * (float)FrameTimer.Delta);
			needResetSamples = true;
		}

		if (needResetSamples) {
			renderer->ResetSamples();
			needResetSamples = false;
		}

		camera.GenerateImagePlane();

		renderer->RenderFrame(camera);
		renderer->Present();

		Window.Update();

		// Take screenshot at the end of rendering
		if (Window.GetKey(GLFW_KEY_F2)) {
			renderer->SaveScreenshot("res/screenshots/" + std::to_string(std::time(nullptr)) + ".png");
		}
		else if (Window.GetKey(GLFW_KEY_R)) {
			std::cout << "RENDERING REFERENCE Go grab a cup of coffee. This is going to take a while.\n";
			Timer referenceTimer;
			referenceTimer.Begin();

			renderer->RenderReference(camera);

			referenceTimer.End();
			referenceTimer.DebugTime();
		}

		FrameTimer.End();
		FrameTimer.DebugTime();

		numFrames++;
	}

	auto delta = GetCurrentTimeNano64() - start;
	float average = numFrames / (delta / 1e9f);
	std::cout << "Average FPS was " << average << '\n';

	renderer->CleanUp();
	Window.Close();

	delete renderer;
}