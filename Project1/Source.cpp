#include <iostream>         // cout, cerr
#include <cstdlib>          // EXIT_FAILURE
#include <GL/glew.h>        // GLEW library
#include <GLFW/glfw3.h>     // GLFW library

// GLM Math Header inclusions
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb-master/stb_image.h> // Image loading Utility functions
#include <learnOpengl/camera.h>
#include "meshes.h"

// Uses the standard namespace for debug output
using namespace std;

// Shader program Macro //
#ifndef GLSL
#define GLSL(Version, Source) "#version " #Version " core \n" #Source
#endif

// Unnamed namespace for C++ defines //
namespace
{
	// Macro for OpenGL window title
	const char* const WINDOW_TITLE = "CS330 - Ice Cream";

	// Variables for window width and height
	const int WINDOW_WIDTH = 800;
	const int WINDOW_HEIGHT = 800;

	// Main GLFW window
	GLFWwindow* gWindow = nullptr;
	// Shader program
	GLuint gProgramId1;
	GLuint gLampProgramId;
	// Texture Ids
	GLuint gTextureIdBlue;
	GLuint gTextureIdRed;
	GLuint gTextureIdBrown;
	GLuint gTextureIdGreen;
	GLuint gTextureIdYellow;
	GLuint gTextureIdWhite;
	GLuint gTextureIdSilver;

	Meshes meshes;

	// camera
	Camera gCamera(glm::vec3(0.0f, 0.0f, 3.0f));
	float gLastX = WINDOW_WIDTH / 2.0f;
	float gLastY = WINDOW_HEIGHT / 2.0f;
	glm::mat4 _Projection;
	bool gFirstMouse = true;

	// timing
	float gDeltaTime = 0.0f; // time between current frame and last frame
	float gLastFrame = 0.0f;

	// Perspective flag
	bool perspectiveMode = true;

}

/* User-defined Function prototypes to:
 * initialize the program, set the window size,
 * redraw graphics on the window when resized,
 * and render graphics on the screen
 */
bool Initialize(int, char* [], GLFWwindow** window);
void UResizeWindow(GLFWwindow* window, int width, int height);
void ProcessInput(GLFWwindow* window);
void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos);
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
void UMouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void Render();
bool CreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId);
void DestroyShaderProgram(GLuint programId);
bool CreateTexture(const char* filename, GLuint& textureId);
void DestroyTexture(GLuint textureId);


///////////////////////////////////////////////////////////////////////////////////////////////////////
/* Surface Vertex Shader Source Code*/
const GLchar* surfaceVertexShaderSource = GLSL(440,

	layout(location = 0) in vec3 vertexPosition; // VAP position 0 for vertex position data
layout(location = 1) in vec3 vertexNormal; // VAP position 1 for normals
layout(location = 2) in vec2 textureCoordinate;

out vec3 vertexFragmentNormal; // For outgoing normals to fragment shader
out vec3 vertexFragmentPos; // For outgoing color / pixels to fragment shader
out vec2 vertexTextureCoordinate;

//Uniform / Global variables for the  transform matrices
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
	gl_Position = projection * view * model * vec4(vertexPosition, 1.0f); // Transforms vertices into clip coordinates

	vertexFragmentPos = vec3(model * vec4(vertexPosition, 1.0f)); // Gets fragment / pixel position in world space only (exclude view and projection)

	vertexFragmentNormal = mat3(transpose(inverse(model))) * vertexNormal; // get normal vectors in world space only and exclude normal translation properties
	vertexTextureCoordinate = textureCoordinate;
}
);
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
/* Surface Fragment Shader Source Code*/
const GLchar* surfaceFragmentShaderSource = GLSL(440,

	in vec3 vertexFragmentNormal; // For incoming normals
in vec3 vertexFragmentPos; // For incoming fragment position
in vec2 vertexTextureCoordinate;

out vec4 fragmentColor; // For outgoing cube color to the GPU

// Uniform / Global variables for object color, light color, light position, and camera/view position
uniform vec4 objectColor;
uniform vec3 ambientColor;
uniform vec3 light1Color;
uniform vec3 light1Position;
uniform vec3 light2Color;
uniform vec3 light2Position;
uniform vec3 viewPosition;
uniform sampler2D uTexture; // Useful when working with multiple textures
uniform vec2 uvScale;
uniform bool ubHasTexture;
uniform float ambientStrength = 0.1f; // Set ambient or global lighting strength
uniform float specularIntensity1 = 0.8f;
uniform float highlightSize1 = 16.0f;
uniform float specularIntensity2 = 0.8f;
uniform float highlightSize2 = 16.0f;

void main()
{
	/*Phong lighting model calculations to generate ambient, diffuse, and specular components*/

	//Calculate Ambient lighting
	vec3 ambient = ambientStrength * ambientColor; // Generate ambient light color

	//**Calculate Diffuse lighting**
	vec3 norm = normalize(vertexFragmentNormal); // Normalize vectors to 1 unit
	vec3 light1Direction = normalize(light1Position - vertexFragmentPos); // Calculate distance (light direction) between light source and fragments/pixels on cube
	float impact1 = max(dot(norm, light1Direction), 0.0);// Calculate diffuse impact by generating dot product of normal and light
	vec3 diffuse1 = impact1 * light1Color; // Generate diffuse light color
	vec3 light2Direction = normalize(light2Position - vertexFragmentPos); // Calculate distance (light direction) between light source and fragments/pixels on cube
	float impact2 = max(dot(norm, light2Direction), 0.0);// Calculate diffuse impact by generating dot product of normal and light
	vec3 diffuse2 = impact2 * light2Color; // Generate diffuse light color

	//**Calculate Specular lighting**
	vec3 viewDir = normalize(viewPosition - vertexFragmentPos); // Calculate view direction
	vec3 reflectDir1 = reflect(-light1Direction, norm);// Calculate reflection vector
	//Calculate specular component
	float specularComponent1 = pow(max(dot(viewDir, reflectDir1), 0.0), highlightSize1);
	vec3 specular1 = specularIntensity1 * specularComponent1 * light1Color;
	vec3 reflectDir2 = reflect(-light2Direction, norm);// Calculate reflection vector
	//Calculate specular component
	float specularComponent2 = pow(max(dot(viewDir, reflectDir2), 0.0), highlightSize2);
	vec3 specular2 = specularIntensity2 * specularComponent2 * light2Color;

	//**Calculate phong result**
	//Texture holds the color to be used for all three components
	vec4 textureColor = texture(uTexture, vertexTextureCoordinate * uvScale);
	vec3 phong1;
	vec3 phong2;

	if (ubHasTexture == true)
	{
		phong1 = (ambient + diffuse1 + specular1) * textureColor.xyz;
		phong2 = (ambient + diffuse2 + specular2) * textureColor.xyz;
	}
	else
	{
		phong1 = (ambient + diffuse1 + specular1) * objectColor.xyz;
		phong2 = (ambient + diffuse2 + specular2) * objectColor.xyz;
	}

	fragmentColor = vec4(phong1 + phong2, 1.0); // Send lighting results to GPU
	//fragmentColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);
}
);
/////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////




// main function. Entry point to the OpenGL program //
int main(int argc, char* argv[])
{
	if (!Initialize(argc, argv, &gWindow))
		return EXIT_FAILURE;

	// Create the mesh, send data to VBO
	meshes.CreateMeshes();

	// Create the shader program
	if (!CreateShaderProgram(surfaceVertexShaderSource, surfaceFragmentShaderSource, gProgramId1))
		return EXIT_FAILURE;


	//Load texture data from file
	const char * texFilename1 = "../resources/textures/vanilla.jpg";
	if (!CreateTexture(texFilename1, gTextureIdBlue))
	{
		cout << "Failed to load texture " << texFilename1 << endl;
		return EXIT_FAILURE;
	}

	texFilename1 = "../resources/textures/wood_table.jpg";
	if (!CreateTexture(texFilename1, gTextureIdRed))
	{
		cout << "Failed to load texture " << texFilename1 << endl;
		return EXIT_FAILURE;
	}

	texFilename1 = "../resources/textures/cork_texture.jpg";
	if (!CreateTexture(texFilename1, gTextureIdBrown))
	{
		cout << "Failed to load texture " << texFilename1 << endl;
		return EXIT_FAILURE;
	}

	texFilename1 = "../resources/textures/label.jpg";
	if (!CreateTexture(texFilename1, gTextureIdGreen))
	{
		cout << "Failed to load texture " << texFilename1 << endl;
		return EXIT_FAILURE;
	}

	texFilename1 = "../resources/textures/bottle.jpg";
	if (!CreateTexture(texFilename1, gTextureIdYellow))
	{
		cout << "Failed to load texture " << texFilename1 << endl;
		return EXIT_FAILURE;
	}

	texFilename1 = "../resources/textures/marble.jpg";
	if (!CreateTexture(texFilename1, gTextureIdWhite))
	{
		cout << "Failed to load texture " << texFilename1 << endl;
		return EXIT_FAILURE;
	}

	texFilename1 = "../resources/textures/spoon.jpg";
	if (!CreateTexture(texFilename1, gTextureIdSilver))
	{
		cout << "Failed to load texture " << texFilename1 << endl;
		return EXIT_FAILURE;
	}

	// Activate the program that will reference the texture
	glUseProgram(gProgramId1);
	// We set the texture as texture unit 0
	glUniform1i(glGetUniformLocation(gProgramId1, "uTextureBase"), 0);

	// Sets the background color of the window to black (it will be implicitely used by glClear)
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);


	// Render loop
	while (!glfwWindowShouldClose(gWindow))
	{
		// per-frame timing

		float currentFrame = glfwGetTime();
		gDeltaTime = currentFrame - gLastFrame;
		gLastFrame = currentFrame;

		// Process keyboard input before rendering
		ProcessInput(gWindow);

		// Render this frame
		Render();

		glfwPollEvents();
	}

	// Release mesh data
	meshes.DestroyMeshes();
	// Release shader program
	DestroyShaderProgram(gProgramId1);
	DestroyShaderProgram(gLampProgramId);
	// Release the textures
	DestroyTexture(gTextureIdBlue);
	DestroyTexture(gTextureIdRed);
	DestroyTexture(gTextureIdBrown);
	DestroyTexture(gTextureIdGreen);

	exit(EXIT_SUCCESS); // Terminates the program successfully
}

// Initialize GLFW, GLEW, and create a window //
bool Initialize(int argc, char* argv[], GLFWwindow** window)
{
	// GLFW: initialize and configure
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// GLFW: create OpenGL output window, return error if fails
	*window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, NULL, NULL);
	if (*window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return false;
	}

	// Set the context for the current window
	glfwMakeContextCurrent(*window);
	glfwSetFramebufferSizeCallback(*window, UResizeWindow);
	glfwSetCursorPosCallback(*window, UMousePositionCallback);
	glfwSetScrollCallback(*window, UMouseScrollCallback);
	glfwSetMouseButtonCallback(*window, UMouseButtonCallback);

	// tell GLFW to capture our mouse
	glfwSetInputMode(*window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// GLEW: initialize
	// ----------------
	// Note: if using GLEW version 1.13 or earlier
	glewExperimental = GL_TRUE;
	GLenum GlewInitResult = glewInit();
	// If init fails, output error string, return error
	if (GLEW_OK != GlewInitResult)
	{
		std::cerr << glewGetErrorString(GlewInitResult) << std::endl;
		return false;
	}

	// Displays GPU OpenGL version
	cout << "INFO: OpenGL Version: " << glGetString(GL_VERSION) << endl;

	return true;
}

// Process keyboard input
void ProcessInput(GLFWwindow* window)
{
	static const float cameraSpeed = 2.5f;

	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		gCamera.ProcessKeyboard(FORWARD, gDeltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		gCamera.ProcessKeyboard(BACKWARD, gDeltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		gCamera.ProcessKeyboard(LEFT, gDeltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		gCamera.ProcessKeyboard(RIGHT, gDeltaTime);

	// Add stubs for Q/E Upward/Downward movement
	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
		gCamera.ProcessKeyboard(UP, gDeltaTime);
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
		gCamera.ProcessKeyboard(DOWN, gDeltaTime);

	// Add stubs to change view
	if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS)
	{
		// Reset the flag
		if (perspectiveMode)
			perspectiveMode = false;
		else
			perspectiveMode = true;
	}
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
void UResizeWindow(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos)
{
	if (gFirstMouse)
	{
		gLastX = xpos;
		gLastY = ypos;
		gFirstMouse = false;
	}

	float xoffset = xpos - gLastX;
	float yoffset = gLastY - ypos; // reversed since y-coordinates go from bottom to top

	gLastX = xpos;
	gLastY = ypos;

	gCamera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
	gCamera.ProcessMouseScroll(yoffset);
}

// glfw: handle mouse button events
// --------------------------------
void UMouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	switch (button)
	{
	case GLFW_MOUSE_BUTTON_LEFT:
	{
		if (action == GLFW_PRESS)
			cout << "Left mouse button pressed" << endl;
		else
			cout << "Left mouse button released" << endl;
	}
	break;

	case GLFW_MOUSE_BUTTON_MIDDLE:
	{
		if (action == GLFW_PRESS)
			cout << "Middle mouse button pressed" << endl;
		else
			cout << "Middle mouse button released" << endl;
	}
	break;

	case GLFW_MOUSE_BUTTON_RIGHT:
	{
		if (action == GLFW_PRESS)
			cout << "Right mouse button pressed" << endl;
		else
			cout << "Right mouse button released" << endl;
	}
	break;

	default:
		cout << "Unhandled mouse button event" << endl;
		break;
	}
}

// Function called to render a frame
// --------------------------------
void SwitchPerspective()
{
	// Return the correct perspective
	if (perspectiveMode)
	{
		// Perspective
		_Projection = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);
	}
	else
	{
		// Org
		_Projection = glm::ortho(-5.0f, 5.0f, -5.0f, 5.0f, 0.1f, 100.0f);
	}
}

// Render the next frame to the OpenGL viewport //
void Render()
{
	GLuint uHasTextureLoc;
	bool ubHasTextureVal;
	glm::mat4 scale;
	glm::mat4 rotation;
	glm::mat4 translation;
	glm::mat4 model;
	GLint modelLoc;
	GLint viewLoc;
	GLint projLoc;
	GLint viewPosLoc;
	GLint ambStrLoc;
	GLint ambColLoc;
	GLint light1ColLoc;
	GLint light1PosLoc;
	GLint light2ColLoc;
	GLint light2PosLoc;
	GLint objColLoc;
	GLint specInt1Loc;
	GLint highlghtSz1Loc;
	GLint specInt2Loc;
	GLint highlghtSz2Loc;

	// Enable z-depth
	glEnable(GL_DEPTH_TEST);

	// Clear the background
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	

	// camera/view transformation
	glm::mat4 view = gCamera.GetViewMatrix();

	// Creates a perspective projection
	glm::mat4 projection = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);

	// Set the program to be used
	glUseProgram(gProgramId1);

	// Get the has texture location
	uHasTextureLoc = glGetUniformLocation(gProgramId1, "ubHasTexture");


	ubHasTextureVal = false;
	glUniform1i(uHasTextureLoc, ubHasTextureVal);

	// Retrieves and passes transform matrices to the Shader program
	modelLoc = glGetUniformLocation(gProgramId1, "model");
	viewLoc = glGetUniformLocation(gProgramId1, "view");
	projLoc = glGetUniformLocation(gProgramId1, "projection");
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

	viewPosLoc = glGetUniformLocation(gProgramId1, "viewPosition");
	ambStrLoc = glGetUniformLocation(gProgramId1, "ambientStrength");
	ambColLoc = glGetUniformLocation(gProgramId1, "ambientColor");
	light1ColLoc = glGetUniformLocation(gProgramId1, "light1Color");
	light1PosLoc = glGetUniformLocation(gProgramId1, "light1Position");
	light2ColLoc = glGetUniformLocation(gProgramId1, "light2Color");
	light2PosLoc = glGetUniformLocation(gProgramId1, "light2Position");
	objColLoc = glGetUniformLocation(gProgramId1, "objectColor");
	specInt1Loc = glGetUniformLocation(gProgramId1, "specularIntensity1");
	highlghtSz1Loc = glGetUniformLocation(gProgramId1, "highlightSize1");
	specInt2Loc = glGetUniformLocation(gProgramId1, "specularIntensity2");
	highlghtSz2Loc = glGetUniformLocation(gProgramId1, "highlightSize2");
	uHasTextureLoc = glGetUniformLocation(gProgramId1, "ubHasTexture");


	//////BOWL PARTS/////

	// Bottom of bowl
	glBindVertexArray(meshes.gCylinderMesh.vao);

	// Set the mesh transfomation values
	scale = glm::scale(glm::vec3(0.3f, 0.06f, 0.3f));
	rotation = glm::rotate(0.0f, glm::vec3(1.0f, 1.0f, 1.0f));
	translation = glm::translate(glm::vec3(0.0f, 0.0f, 0.5f));
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	//set the camera view location
	glUniform3f(viewPosLoc, gCamera.Position.x, gCamera.Position.y, gCamera.Position.z);
	//set ambient lighting strength
	glUniform1f(ambStrLoc, 0.0f);
	//set ambient color
	glUniform3f(ambColLoc, 0.1f, 0.1f, 0.1f);
	glUniform3f(light1ColLoc, 1.0f, 0.8f, 0.8f);
	glUniform3f(light1PosLoc, -0.5f, 1.0f, -1.0f);
	//glUniform3f(light2ColLoc, 0.8f, 1.0f, 0.8f);
	//glUniform3f(light2PosLoc, 0.5f, 1.0f, -1.0f);
	//set specular intensity
	glUniform1f(specInt1Loc, .4f);
	glUniform1f(specInt2Loc, .4f);
	//set specular highlight size
	glUniform1f(highlghtSz1Loc, 2.0f);
	glUniform1f(highlghtSz2Loc, 32.0f);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gTextureIdWhite);

	glUniform2f(glGetUniformLocation(gProgramId1, "uvScale"), 1.0f, 1.0f);

	glDrawElements(GL_TRIANGLES, 720, GL_UNSIGNED_INT, (void*)0);

	ubHasTextureVal = true;
	glUniform1i(uHasTextureLoc, ubHasTextureVal);

	//	glDrawArrays(GL_TRIANGLE_FAN, 0, 36);		//bottom
	//	glDrawArrays(GL_TRIANGLE_FAN, 36, 72);		//top
	glDrawArrays(GL_TRIANGLE_STRIP, 72, 146);	//sides

	// Deactivate the Vertex Array Object
	glBindVertexArray(0);

	// Bowl
	glBindVertexArray(meshes.gSphereMesh.vao);

	// Set the mesh transfomation values
	scale = glm::scale(glm::vec3(1.0f, 0.4f, 1.0f));
	rotation = glm::rotate(3.142f, glm::vec3(1.0f, 0.0f, 0.0f));
	translation = glm::translate(glm::vec3(0.0f, 0.42f, 0.5f));
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	//set the camera view location
	glUniform3f(viewPosLoc, gCamera.Position.x, gCamera.Position.y, gCamera.Position.z);
	//set ambient lighting strength
	glUniform1f(ambStrLoc, 0.5f);
	//set ambient color
	glUniform3f(ambColLoc, 0.1f, 0.1f, 0.1f);
	glUniform3f(light1ColLoc, 1.0f, 0.8f, 0.8f);
	glUniform3f(light1PosLoc, -0.5f, 1.0f, -1.0f);
	//glUniform3f(light2ColLoc, 0.0f, 0.0f, 0.0f);
	//glUniform3f(light2PosLoc, 0.5f, 1.0f, -1.0f);
	//set specular intensity
	glUniform1f(specInt1Loc, .4f);
	glUniform1f(specInt2Loc, .4f);
	//set specular highlight size
	glUniform1f(highlghtSz1Loc, 2.0f);
	glUniform1f(highlghtSz2Loc, 32.0f);

	ubHasTextureVal = true;
	glUniform1i(uHasTextureLoc, ubHasTextureVal);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gTextureIdWhite);

	glUniform2f(glGetUniformLocation(gProgramId1, "uvScale"), 1.0f, 1.0f);

	glDrawElements(GL_TRIANGLES, 720, GL_UNSIGNED_INT, (void*)0);

	// Deactivate the Vertex Array Object
	glBindVertexArray(0);


	/////WINE BOTTLE PARTS/////

	// Cork
	glBindVertexArray(meshes.gCylinderMesh.vao);

	// Set the mesh transfomation values
	scale = glm::scale(glm::vec3(0.09f, 0.25f, 0.09f));
	rotation = glm::rotate(0.00f, glm::vec3(1.0f, 0.0f, 0.0f));
	translation = glm::translate(glm::vec3(2.0f, 1.9f, -1.0f));
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	//set the camera view location
	glUniform3f(viewPosLoc, gCamera.Position.x, gCamera.Position.y, gCamera.Position.z);
	//set ambient lighting strength
	glUniform1f(ambStrLoc, 1.0f);
	//set ambient color
	glUniform3f(ambColLoc, 0.1f, 0.1f, 0.1f);
	glUniform3f(light1ColLoc, 1.0f, 0.8f, 0.8f);
	glUniform3f(light1PosLoc, -0.5f, 1.0f, -1.0f);
	glUniform3f(light2ColLoc, 0.8f, 1.0f, 0.8f);
	glUniform3f(light2PosLoc, 0.5f, 1.0f, -1.0f);
	//set specular intensity
	glUniform1f(specInt1Loc, .4f);
	glUniform1f(specInt2Loc, .4f);
	//set specular highlight size
	glUniform1f(highlghtSz1Loc, 2.0f);
	glUniform1f(highlghtSz2Loc, 32.0f);


	ubHasTextureVal = true;
	glUniform1i(uHasTextureLoc, ubHasTextureVal);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gTextureIdBrown);

	glUniform2f(glGetUniformLocation(gProgramId1, "uvScale"), 1.0f, 1.0f);

	glDrawArrays(GL_TRIANGLE_FAN, 0, 36);		//bottom
	glDrawArrays(GL_TRIANGLE_FAN, 36, 72);		//top
	glDrawArrays(GL_TRIANGLE_STRIP, 72, 146);	//sides

	// Deactivate the Vertex Array Object
	glBindVertexArray(0);

	// Bottle neck top
	glBindVertexArray(meshes.gCylinderMesh.vao);

	// Set the mesh transfomation values
	scale = glm::scale(glm::vec3(0.12f, 0.5f, 0.12f));
	rotation = glm::rotate(0.00f, glm::vec3(1.0f, 0.0f, 0.0f));
	translation = glm::translate(glm::vec3(2.0f, 1.5f, -1.0f));
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	//set the camera view location
	glUniform3f(viewPosLoc, gCamera.Position.x, gCamera.Position.y, gCamera.Position.z);
	//set ambient lighting strength
	glUniform1f(ambStrLoc, 0.5f);
	//set ambient color
	glUniform3f(ambColLoc, 0.1f, 0.1f, 0.1f);
	glUniform3f(light1ColLoc, 1.0f, 0.8f, 0.8f);
	glUniform3f(light1PosLoc, -0.5f, 1.0f, -1.0f);
	glUniform3f(light2ColLoc, 0.8f, 1.0f, 0.8f);
	glUniform3f(light2PosLoc, 0.5f, 1.0f, -1.0f);
	//set specular intensity
	glUniform1f(specInt1Loc, .4f);
	glUniform1f(specInt2Loc, .4f);
	//set specular highlight size
	glUniform1f(highlghtSz1Loc, 2.0f);
	glUniform1f(highlghtSz2Loc, 32.0f);

	
	ubHasTextureVal = true;
	glUniform1i(uHasTextureLoc, ubHasTextureVal);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gTextureIdYellow);

	glUniform2f(glGetUniformLocation(gProgramId1, "uvScale"), 1.0f, 1.0f);

	glDrawArrays(GL_TRIANGLE_FAN, 0, 36);		//bottom
	glDrawArrays(GL_TRIANGLE_FAN, 36, 72);		//top
	glDrawArrays(GL_TRIANGLE_STRIP, 72, 146);	//sides

	// Deactivate the Vertex Array Object
	glBindVertexArray(0);

	// Bottle neck bottom
	glBindVertexArray(meshes.gConeMesh.vao);

	// Set the mesh transfomation values
	scale = glm::scale(glm::vec3(0.4f, 0.5f, 0.4f));
	rotation = glm::rotate(0.00f, glm::vec3(1.0f, -1.0f, 0.0f));
	translation = glm::translate(glm::vec3(2.0f, 1.25f, -1.0f));
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	//set the camera view location
	glUniform3f(viewPosLoc, gCamera.Position.x, gCamera.Position.y, gCamera.Position.z);
	//set ambient lighting strength
	glUniform1f(ambStrLoc, 0.5f);
	//set ambient color
	glUniform3f(ambColLoc, 0.1f, 0.1f, 0.1f);
	glUniform3f(light1ColLoc, 1.0f, 0.8f, 0.8f);
	glUniform3f(light1PosLoc, -0.5f, 1.0f, -1.0f);
	glUniform3f(light2ColLoc, 0.8f, 1.0f, 0.8f);
	glUniform3f(light2PosLoc, 0.5f, 1.0f, -1.0f);
	//set specular intensity
	glUniform1f(specInt1Loc, .4f);
	glUniform1f(specInt2Loc, .4f);
	//set specular highlight size
	glUniform1f(highlghtSz1Loc, 2.0f);
	glUniform1f(highlghtSz2Loc, 32.0f);

	
	ubHasTextureVal = true;
	glUniform1i(uHasTextureLoc, ubHasTextureVal);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gTextureIdYellow);

	glUniform2f(glGetUniformLocation(gProgramId1, "uvScale"), 1.0f, 1.0f);

	glDrawArrays(GL_TRIANGLE_FAN, 0, 36);		//bottom
	glDrawArrays(GL_TRIANGLE_STRIP, 36, 108);	//sides

	// Deactivate the Vertex Array Object
	glBindVertexArray(0);

	// Bottle
	glBindVertexArray(meshes.gCylinderMesh.vao);

	// Set the mesh transfomation values
	scale = glm::scale(glm::vec3(0.40f, 1.25f, 0.40f));
	rotation = glm::rotate(3.142f, glm::vec3(1.0f, 0.0f, 0.0f));
	translation = glm::translate(glm::vec3(2.0f, 1.25f, -1.0f));
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	//set the camera view location
	glUniform3f(viewPosLoc, gCamera.Position.x, gCamera.Position.y, gCamera.Position.z);
	//set ambient lighting strength
	glUniform1f(ambStrLoc, 0.0f);
	//set ambient color
	glUniform3f(ambColLoc, 0.1f, 0.1f, 0.1f);
	glUniform3f(light1ColLoc, 1.0f, 0.8f, 0.8f);
	glUniform3f(light1PosLoc, -0.5f, 1.0f, -1.0f);
	//glUniform3f(light2ColLoc, 0.8f, 1.0f, 0.8f);
	//glUniform3f(light2PosLoc, 0.5f, 1.0f, -1.0f);
	//set specular intensity
	glUniform1f(specInt1Loc, .4f);
	glUniform1f(specInt2Loc, .4f);
	//set specular highlight size
	glUniform1f(highlghtSz1Loc, 2.0f);
	glUniform1f(highlghtSz2Loc, 32.0f);


	ubHasTextureVal = true;
	glUniform1i(uHasTextureLoc, ubHasTextureVal);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gTextureIdGreen);

	glUniform2f(glGetUniformLocation(gProgramId1, "uvScale"), 1.0f, 1.0f);

	glDrawArrays(GL_TRIANGLE_FAN, 0, 36);		//bottom
    glDrawArrays(GL_TRIANGLE_FAN, 36, 72);		//top
	glDrawArrays(GL_TRIANGLE_STRIP, 72, 146);	//sides

	// Deactivate the Vertex Array Object
	glBindVertexArray(0);

	//////TABLE//////

	// Table
	glBindVertexArray(meshes.gPlaneMesh.vao);

	// Set the mesh transfomation values
	scale = glm::scale(glm::vec3(3.0f, 3.0f, 3.0f));
	rotation = glm::rotate(0.0f, glm::vec3(1.0f, 1.0f, 1.0f));
	translation = glm::translate(glm::vec3(0.25f, -0.01f, -1.0f));
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	//set the camera view location
	glUniform3f(viewPosLoc, gCamera.Position.x, gCamera.Position.y, gCamera.Position.z);
	//set ambient lighting strength
	glUniform1f(ambStrLoc, 0.0f);
	//set ambient color
	glUniform3f(ambColLoc, 0.1f, 0.1f, 0.1f);
	glUniform3f(light1ColLoc, 1.0f, 0.8f, 0.8f);
	glUniform3f(light1PosLoc, -0.5f, 1.0f, -1.0f);
	glUniform3f(light2ColLoc, 1.0f, 1.0f, 1.0f);
	glUniform3f(light2PosLoc, 0.5f, 1.0f, -1.0f);
	//set specular intensity
	glUniform1f(specInt1Loc, 1.0f);
	glUniform1f(specInt2Loc, .4f);
	//set specular highlight size
	glUniform1f(highlghtSz1Loc, 2.0f);
	glUniform1f(highlghtSz2Loc, 32.0f);

	ubHasTextureVal = true;
	glUniform1i(uHasTextureLoc, ubHasTextureVal);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gTextureIdRed);

	glUniform2f(glGetUniformLocation(gProgramId1, "uvScale"), 1.0f, 1.0f);

	glDrawElements(GL_TRIANGLES, meshes.gPlaneMesh.nIndices, GL_UNSIGNED_INT, (void*)0);

	// Deactivate the Vertex Array Object
	glBindVertexArray(0);


	//////ICE CREAM//////

	// Ice Cream scoop#1
	glBindVertexArray(meshes.gSphereMesh.vao);

	// Set the mesh transfomation values
	scale = glm::scale(glm::vec3(-0.45f, -0.25f, -0.45f));
	rotation = glm::rotate(0.0f, glm::vec3(1.0f, 1.0f, 1.0f));
	translation = glm::translate(glm::vec3(-0.35f, 0.35f, 0.55f));
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	//set the camera view location
	glUniform3f(viewPosLoc, gCamera.Position.x, gCamera.Position.y, gCamera.Position.z);
	//set ambient lighting strength
	glUniform1f(ambStrLoc, 1.0f);
	//set ambient color
	glUniform3f(ambColLoc, 0.1f, 0.1f, 0.1f);
	glUniform3f(light1ColLoc, 1.0f, 0.8f, 0.8f);
	glUniform3f(light1PosLoc, -0.5f, 1.0f, -1.0f);
	//glUniform3f(light2ColLoc, 0.8f, 1.0f, 0.8f);
	//glUniform3f(light2PosLoc, 0.5f, 1.0f, -1.0f);
	//set specular intensity
	glUniform1f(specInt1Loc, .4f);
	glUniform1f(specInt2Loc, .4f);
	//set specular highlight size
	glUniform1f(highlghtSz1Loc, 2.0f);
	glUniform1f(highlghtSz2Loc, 32.0f);

	ubHasTextureVal = true;
	glUniform1i(uHasTextureLoc, ubHasTextureVal);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gTextureIdBlue);

	glUniform2f(glGetUniformLocation(gProgramId1, "uvScale"), 1.0f, 1.0f);

	glDrawElements(GL_TRIANGLES, meshes.gSphereMesh.nIndices, GL_UNSIGNED_INT, (void*)0);

	// Deactivate the Vertex Array Object
	glBindVertexArray(0);

	// Ice Cream scoop#2
	glBindVertexArray(meshes.gSphereMesh.vao);

	// Set the mesh transfomation values
	scale = glm::scale(glm::vec3(-0.45f, -0.25f, -0.45f));
	rotation = glm::rotate(0.0f, glm::vec3(1.0f, 1.0f, 1.0f));
	translation = glm::translate(glm::vec3(0.25f, 0.35f, 0.75f));
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	//set the camera view location
	glUniform3f(viewPosLoc, gCamera.Position.x, gCamera.Position.y, gCamera.Position.z);
	//set ambient lighting strength
	glUniform1f(ambStrLoc, 1.0f);
	//set ambient color
	glUniform3f(ambColLoc, 0.1f, 0.1f, 0.1f);
	glUniform3f(light1ColLoc, 1.0f, 0.8f, 0.8f);
	glUniform3f(light1PosLoc, -0.5f, 1.0f, -1.0f);
	//glUniform3f(light2ColLoc, 0.8f, 1.0f, 0.8f);
	//glUniform3f(light2PosLoc, 0.5f, 1.0f, -1.0f);
	//set specular intensity
	glUniform1f(specInt1Loc, .4f);
	glUniform1f(specInt2Loc, .4f);
	//set specular highlight size
	glUniform1f(highlghtSz1Loc, 2.0f);
	glUniform1f(highlghtSz2Loc, 32.0f);

	
	ubHasTextureVal = true;
	glUniform1i(uHasTextureLoc, ubHasTextureVal);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gTextureIdBlue);

	glUniform2f(glGetUniformLocation(gProgramId1, "uvScale"), 1.0f, 1.0f);

	glDrawElements(GL_TRIANGLES, meshes.gSphereMesh.nIndices, GL_UNSIGNED_INT, (void*)0);

	// Deactivate the Vertex Array Object
	glBindVertexArray(0);

	// Ice Cream scoop#3
	glBindVertexArray(meshes.gSphereMesh.vao);

	// Set the mesh transfomation values
	scale = glm::scale(glm::vec3(-0.45f, -0.25f, -0.45f));
	rotation = glm::rotate(0.0f, glm::vec3(1.0f, 1.0f, 1.0f));
	translation = glm::translate(glm::vec3(0.13f, 0.35f, 0.2f));
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	//set the camera view location
	glUniform3f(viewPosLoc, gCamera.Position.x, gCamera.Position.y, gCamera.Position.z);
	//set ambient lighting strength
	glUniform1f(ambStrLoc, 1.0f);
	//set ambient color
	glUniform3f(ambColLoc, 0.1f, 0.1f, 0.1f);
	glUniform3f(light1ColLoc, 1.0f, 0.8f, 0.8f);
	glUniform3f(light1PosLoc, -0.5f, 1.0f, -1.0f);
	//glUniform3f(light2ColLoc, 0.8f, 1.0f, 0.8f);
	//glUniform3f(light2PosLoc, 0.5f, 1.0f, -1.0f);
	//set specular intensity
	glUniform1f(specInt1Loc, .4f);
	glUniform1f(specInt2Loc, .4f);
	//set specular highlight size
	glUniform1f(highlghtSz1Loc, 2.0f);
	glUniform1f(highlghtSz2Loc, 32.0f);


	ubHasTextureVal = true;
	glUniform1i(uHasTextureLoc, ubHasTextureVal);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gTextureIdBlue);

	glUniform2f(glGetUniformLocation(gProgramId1, "uvScale"), 1.0f, 1.0f);

	glDrawElements(GL_TRIANGLES, meshes.gSphereMesh.nIndices, GL_UNSIGNED_INT, (void*)0);

	// Deactivate the Vertex Array Object
	glBindVertexArray(0);

	// Ice Cream scoop#4
	glBindVertexArray(meshes.gSphereMesh.vao);

	// Set the mesh transfomation values
	scale = glm::scale(glm::vec3(-0.38f, -0.25f, -0.38f));
	rotation = glm::rotate(0.0f, glm::vec3(1.0f, 1.0f, 1.0f));
	translation = glm::translate(glm::vec3(0.0f, 0.62f, 0.45f));
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	//set the camera view location
	glUniform3f(viewPosLoc, gCamera.Position.x, gCamera.Position.y, gCamera.Position.z);
	//set ambient lighting strength
	glUniform1f(ambStrLoc, 1.0f);
	//set ambient color
	glUniform3f(ambColLoc, 0.1f, 0.1f, 0.1f);
	glUniform3f(light1ColLoc, 1.0f, 0.8f, 0.8f);
	glUniform3f(light1PosLoc, -0.5f, 1.0f, -1.0f);
	//glUniform3f(light2ColLoc, 0.8f, 1.0f, 0.8f);
	//glUniform3f(light2PosLoc, 0.5f, 1.0f, -1.0f);
	//set specular intensity
	glUniform1f(specInt1Loc, .4f);
	glUniform1f(specInt2Loc, .4f);
	//set specular highlight size
	glUniform1f(highlghtSz1Loc, 2.0f);
	glUniform1f(highlghtSz2Loc, 32.0f);


	ubHasTextureVal = true;
	glUniform1i(uHasTextureLoc, ubHasTextureVal);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gTextureIdBlue);

	glUniform2f(glGetUniformLocation(gProgramId1, "uvScale"), 1.0f, 1.0f);

	glDrawElements(GL_TRIANGLES, meshes.gSphereMesh.nIndices, GL_UNSIGNED_INT, (void*)0);

	//////SPOON/////

//Spoon
	glBindVertexArray(meshes.gSphereMesh.vao);

	// Set the mesh transfomation values
	scale = glm::scale(glm::vec3(.18f, 0.1f, 0.25f));
	rotation = glm::rotate(3.142f, glm::vec3(1.0f, 0.0f, 0.0f));
	translation = glm::translate(glm::vec3(-1.5f, 0.090f, -0.5f));
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	//set the camera view location
	glUniform3f(viewPosLoc, gCamera.Position.x, gCamera.Position.y, gCamera.Position.z);
	//set ambient lighting strength
	glUniform1f(ambStrLoc, 2.0f);
	//set ambient color
	glUniform3f(ambColLoc, 0.1f, 0.1f, 0.1f);
	glUniform3f(light1ColLoc, 1.0f, 0.8f, 0.8f);
	glUniform3f(light1PosLoc, -0.5f, 1.0f, -1.0f);
	//glUniform3f(light2ColLoc, 0.8f, 1.0f, 0.8f);
	//glUniform3f(light2PosLoc, 0.5f, 1.0f, -1.0f);
	//set specular intensity
	glUniform1f(specInt1Loc, .4f);
	glUniform1f(specInt2Loc, .4f);
	//set specular highlight size
	glUniform1f(highlghtSz1Loc, 2.0f);
	glUniform1f(highlghtSz2Loc, 32.0f);

	ubHasTextureVal = true;
	glUniform1i(uHasTextureLoc, ubHasTextureVal);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gTextureIdSilver);

	glUniform2f(glGetUniformLocation(gProgramId1, "uvScale"), 1.0f, 1.0f);

	glDrawElements(GL_TRIANGLES, 720, GL_UNSIGNED_INT, (void*)0);

	// Deactivate the Vertex Array Object
	glBindVertexArray(0);

	// Spoon Handle
	glBindVertexArray(meshes.gCylinderMesh.vao);

	// Set the mesh transfomation values
	scale = glm::scale(glm::vec3(0.040f, 0.88f, 0.015f));
	rotation = glm::rotate(1.60f, glm::vec3(10.0f, -0.0f, 0.20f));
	translation = glm::translate(glm::vec3(-1.5f, 0.05f, -0.27f));
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	//set the camera view location
	glUniform3f(viewPosLoc, gCamera.Position.x, gCamera.Position.y, gCamera.Position.z);
	//set ambient lighting strength
	glUniform1f(ambStrLoc, 0.5f);
	//set ambient color
	glUniform3f(ambColLoc, 0.1f, 0.1f, 0.1f);
	glUniform3f(light1ColLoc, 1.0f, 0.8f, 0.8f);
	glUniform3f(light1PosLoc, -0.5f, 1.0f, -1.0f);
	//glUniform3f(light2ColLoc, 0.8f, 1.0f, 0.8f);
	//glUniform3f(light2PosLoc, 0.5f, 1.0f, -1.0f);
	//set specular intensity
	glUniform1f(specInt1Loc, .4f);
	glUniform1f(specInt2Loc, .4f);
	//set specular highlight size
	glUniform1f(highlghtSz1Loc, 2.0f);
	glUniform1f(highlghtSz2Loc, 32.0f);


	ubHasTextureVal = true;
	glUniform1i(uHasTextureLoc, ubHasTextureVal);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gTextureIdSilver);

	glUniform2f(glGetUniformLocation(gProgramId1, "uvScale"), 1.0f, 1.0f);

	//glDrawArrays(GL_TRIANGLE_FAN, 0, 36);		//bottom
	glDrawArrays(GL_TRIANGLE_FAN, 36, 72);		//top
	glDrawArrays(GL_TRIANGLE_STRIP, 72, 146);	//sides

	// Deactivate the Vertex Array Object
	glBindVertexArray(0);


	// Deactivate the Vertex Array Object
	glBindVertexArray(0);


	// Flips the the back buffer with the front buffer every frame (refresh)
	glfwSwapBuffers(gWindow);
}

//****************************************************
//  const char* vtxShaderSource: vertex shader source code
//  const char* fragShaderSource: fragment shader source code
//  GLuint &programId: unique ID of program associated with shaders
//****************************************************
bool CreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId)
{
	// Compilation and linkage error reporting
	int success = 0;
	char infoLog[512];

	// Create a Shader program object.
	programId = glCreateProgram();

	// Create the vertex and fragment shader objects
	GLuint vertexShaderId = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER);

	// Retrieve the shader source
	glShaderSource(vertexShaderId, 1, &vtxShaderSource, NULL);
	glShaderSource(fragmentShaderId, 1, &fragShaderSource, NULL);

	// Compile the vertex shader, and print compilation errors (if any)
	glCompileShader(vertexShaderId);
	// Check for vertex shader compile errors
	glGetShaderiv(vertexShaderId, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(vertexShaderId, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
		return false;
	}

	// Compile the fragment shader, and print compilation errors (if any)
	glCompileShader(fragmentShaderId);
	// Check for fragment shader compile errors
	glGetShaderiv(fragmentShaderId, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(fragmentShaderId, sizeof(infoLog), NULL, infoLog);
		std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
		return false;
	}

	// Attached compiled shaders to the shader program
	glAttachShader(programId, vertexShaderId);
	glAttachShader(programId, fragmentShaderId);

	// Links the shader program
	glLinkProgram(programId);
	// Check for linking errors
	glGetProgramiv(programId, GL_LINK_STATUS, &success);
	if (!success)
	{
		glGetProgramInfoLog(programId, sizeof(infoLog), NULL, infoLog);
		std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
		return false;
	}

	// Uses the shader program
	glUseProgram(programId);

	return true;
}

// Destroy the linked shader program //
void DestroyShaderProgram(GLuint programId)
{
	glDeleteProgram(programId);
}

// Images are loaded with Y axis going down, but OpenGL's Y axis goes up, so let's flip it //
void flipImageVertically(unsigned char* image, int width, int height, int channels)
{
	for (int j = 0; j < height / 2; ++j)
	{
		int index1 = j * width * channels;
		int index2 = (height - 1 - j) * width * channels;

		for (int i = width * channels; i > 0; --i)
		{
			unsigned char tmp = image[index1];
			image[index1] = image[index2];
			image[index2] = tmp;
			++index1;
			++index2;
		}
	}
}

// Generate and load the texture //
bool CreateTexture(const char* filename, GLuint& textureId)
{
	int width, height, channels;
	unsigned char *image = stbi_load(filename, &width, &height, &channels, 0);
	if (image)
	{
		flipImageVertically(image, width, height, channels);

		glGenTextures(1, &textureId);
		glBindTexture(GL_TEXTURE_2D, textureId);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		if (channels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		else if (channels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			cout << "Not implemented to handle image with " << channels << " channels" << endl;
			return false;
		}

		glGenerateMipmap(GL_TEXTURE_2D);

		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		return true;
	}

	// Error loading the image
	return false;
}

// Release the texture attached to textureId //
void DestroyTexture(GLuint textureId)
{
	glGenTextures(1, &textureId);
}