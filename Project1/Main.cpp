#include <stdio.h>
#include <vector>
#include <string>
#include <execution>
#include <Windows.h>
#include <gl/GL.h>
#include <GLFW/glfw3.h>

#include "glm/glm.hpp"
#include "glm/ext.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/euler_angles.hpp"
#include "glm/gtx/vector_angle.hpp"

#include "UVAtlas.h"
#include "DirectXMesh.h"

#include "Camera.h"

using namespace DirectX;

GLFWwindow* pWindow;
Camera m_Camera;

float m_fElapsedTime = 0.0f;
float m_fCurrentTime = 0.0f;
int nFPS = 0;
float fSec = 0.0f;

HRESULT __cdecl UVAtlasCallback(float fPercent)
{
	printf("Percent: %.0f\n", (fPercent * 100.0f));

	return S_OK;
}

bool Init()
{
	glEnable(GL_DEPTH_TEST);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glPointSize(10.0f);
	glEnable(GL_POINT_SMOOTH);
	glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);

	// camera
	m_Camera.Init(glm::vec3(5, 5, 7), glm::vec3(0, 0, 0));

	// Eredeti mesh adataink
	std::vector<XMFLOAT3> vertices = 
	{
		XMFLOAT3(-1,-1,0),
		XMFLOAT3(+1,-1,0),
		XMFLOAT3(+1,+1,0),

		XMFLOAT3(+1,+1,0),
		XMFLOAT3(-1,+1,0),
		XMFLOAT3(-1,-1,0),
	};
	std::vector<uint32_t> indices = 
	{
		0,1,2,
		3,4,5
	};

	std::vector<DirectX::XMFLOAT2> outUVs;
	std::vector<uint32_t> facePartitioning;
	std::vector<uint8_t> meshVertices;

	std::vector<uint32_t> adjacency(indices.size());
	float epsilon = 0.0001f;
	HRESULT hr;
	hr = GenerateAdjacencyAndPointReps(
		indices.data(),                   // Index buffer
		indices.size() / 3,               // Number of triangles
		vertices.data(),                  // Vertex buffer
		vertices.size(),                  // Number of vertices
		epsilon,                          // Tolerance for identifying duplicate vertices
		nullptr,                  // Output point representatives
		adjacency.data()                  // Output adjacency
	);
	if (FAILED(hr))
	{
		printf("FAILED: GenerateAdjacencyAndPointReps\n");
		return false;
	}

	// Face remap for output
	std::vector<uint32_t> faceRemap;

	size_t maxCharts = 0;
	float maxStretch = 0.16667f;
	float gutter = 2.f;
	size_t width = 512;
	size_t height = 512;
	UVATLAS uvOptions = UVATLAS_DEFAULT;
	UVATLAS uvOptionsEx = UVATLAS_DEFAULT;
	std::vector<UVAtlasVertex> vb;
	std::vector<uint8_t> ib;
	std::vector<uint32_t> vertexRemapArray;
	float outStretch = 0.f;
	size_t outCharts = 0;

	hr = UVAtlasCreate(vertices.data(), vertices.size(),
		indices.data(), DXGI_FORMAT_R32_UINT, indices.size() / 3,
		maxCharts, maxStretch, width, height, gutter,
		adjacency.data(), nullptr,
		nullptr,
		UVAtlasCallback, UVATLAS_DEFAULT_CALLBACK_FREQUENCY,
		uvOptions | uvOptionsEx, vb, ib,
		&facePartitioning,
		&vertexRemapArray,
		&outStretch, &outCharts);

	if (FAILED(hr)) 
	{
		printf("FAILED: UVAtlasCreate\n");
	}

	return true;
}

void Exit()
{
	// show cursor
	glfwSetInputMode(pWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

void Draw() 
{
}

void Update()
{
	// esc exit
	if (GLFW_PRESS == glfwGetKey(pWindow, GLFW_KEY_ESCAPE))
	{
		glfwSetWindowShouldClose(pWindow, true);
		return;
	}

	// update
	// delta time
	m_fElapsedTime = m_fCurrentTime;
	m_fCurrentTime = (float)glfwGetTime();
	float dt = m_fCurrentTime - m_fElapsedTime;

	if (dt <= 0.0f) { dt = 1.0f / 60.0f; }
	if (dt > (1.0f / 10.0f)) { dt = 1.0f / 10.0f; }

	// fps
	// print fps
	nFPS++;
	fSec += dt;
	if (fSec >= 1.0f)
	{
		std::string strTitle = "FPS: " + std::to_string(nFPS);
		glfwSetWindowTitle(pWindow, strTitle.c_str());

		nFPS = 0;
		fSec = 0.0f;
	}

	// camera
	m_Camera.Update(dt);
	if (GLFW_PRESS == glfwGetKey(pWindow, GLFW_KEY_W))
	{
		m_Camera.MoveW();
	}
	if (GLFW_PRESS == glfwGetKey(pWindow, GLFW_KEY_S))
	{
		m_Camera.MoveS();
	}
	if (GLFW_PRESS == glfwGetKey(pWindow, GLFW_KEY_A))
	{
		m_Camera.MoveA();
	}
	if (GLFW_PRESS == glfwGetKey(pWindow, GLFW_KEY_D))
	{
		m_Camera.MoveD();
	}
	double fMouseX, fMouseY;
	glfwGetCursorPos(pWindow, &fMouseX, &fMouseY);
	//glfwSetCursorPos(pWindow, 320, 240);
	int fDeltaMouseX = (int)fMouseX - 320;
	int fDeltaMouseY = (int)fMouseY - 240;
	m_Camera.Rotate(fDeltaMouseX, fDeltaMouseY);

	glm::vec3 v3CameraPos = m_Camera.GetPos();
	glm::vec3 v3CameraAt = m_Camera.GetAt();
	glm::vec3 v3CameraDir = glm::normalize(v3CameraAt - v3CameraPos);

	// draw
	int nWidth, nHeight;
	glfwGetFramebufferSize(pWindow, &nWidth, &nHeight);
	glViewport(0, 0, nWidth, nHeight);

	glClearColor(0.5f, 0.5f, 1.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// camera
	glm::mat4 matCameraView = glm::lookAtRH(v3CameraPos, v3CameraAt, glm::vec3(0, 1, 0));
	glm::mat4 matCameraProj = glm::perspectiveRH(glm::radians(45.0f), (float)nWidth / (float)nHeight, 0.01f, 1000.0f);

	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(glm::value_ptr(matCameraProj));

	glMatrixMode(GL_MODELVIEW);
	glm::mat4 matWorld = glm::mat4(1.0f);
	glLoadMatrixf(glm::value_ptr(matCameraView * matWorld));

	Draw();

	glfwSwapBuffers(pWindow);
	glfwPollEvents();
}

int main(int argc, char** argv)
{
	glfwInit();

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

	int nWindowWidth = 800;
	int nWindowHeight = 600;
	pWindow = glfwCreateWindow(nWindowWidth, nWindowHeight, "UVAtlas OpenGL", NULL, NULL);
	glfwMakeContextCurrent(pWindow);
	glfwSwapInterval(0);

	// center of monitor
	GLFWmonitor* pPrimaryMonitor = glfwGetPrimaryMonitor();
	const GLFWvidmode* pMode = glfwGetVideoMode(pPrimaryMonitor);
	int xpos = (pMode->width - nWindowWidth) / 2;
	int ypos = (pMode->height - nWindowHeight) / 2;
	glfwSetWindowPos(pWindow, xpos, ypos);

	if (false == Init())
	{
		return EXIT_FAILURE;
	}

	while (false == glfwWindowShouldClose(pWindow))
	{
		Update();
	}

	Exit();

	glfwTerminate();

	return EXIT_SUCCESS;
}