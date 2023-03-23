#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include <filesystem>

#include <stdio.h>
#include <stdlib.h>

#define STB_IMAGE_IMPLEMENTATION
#include<stb/stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>

// Let's add STB Image for this recipe
// Removed wireframe option
// Map uv on a triamgle isntead of cude for simplicity
// Book removed depth, kept it (and diagonal rotation)
// Check Shaders + Line 169 for Specific Image Code 

using glm::mat4;
using glm::vec3;

// Will use the wireframe int for color
struct PerFrameData {
	mat4 mvp;
};

static const char* shaderCodeVertex = R"(
#version 460 core
layout(std140, binding = 0) uniform PerFrameData
{
	uniform mat4 MVP;
};
layout (location=0) out vec2 uv;
const vec2 pos[3] = vec2[3](
	vec2(-0.6f, -0.4f),
	vec2( 0.6f, -0.4f),
	vec2( 0.0f,  0.6f)
);
const vec2 tc[3] = vec2[3](
	vec2( 0.0, 0.0 ),
	vec2( 1.0, 0.0 ),
	vec2( 0.5, 1.0 )
);
void main()
{
	gl_Position = MVP * vec4(pos[gl_VertexID], 0.0, 1.0);
	uv = tc[gl_VertexID];
}
)";

static const char* shaderCodeFragment = R"(
#version 460 core
layout (location=0) in vec2 uv;
layout (location=0) out vec4 out_FragColor;
uniform sampler2D texture0;
void main()
{
	out_FragColor = texture(texture0, uv);
}
)";

int main()
{
	// No Debugging very simple window + rotating cube 
	glfwSetErrorCallback(
		  []( int error, const char* description ) 
	{
		  fprintf( stderr, "Error: %s\n", description );
	}
	);
	if (!glfwInit())
		exit( EXIT_FAILURE );

	// Using 4.6
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(1024, 768, "Simple example", nullptr, nullptr);

	if (!window)
	{
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	glfwSetKeyCallback(window,
		[](GLFWwindow* window, int key, int scancode, int action, int mods)
	{
		if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GLFW_TRUE);
	
		// Save screenshot with F9
		if (key == GLFW_KEY_F9 && action == GLFW_PRESS)
		{
			int width, height;
	
			// Implement resizable window, can also go via glfwSetWindowSizeCallback()
			glfwGetFramebufferSize(window, &width, &height);
	
			// Lets save amges as png (many other supported formats)
			// 1 Initiate pointer
			uint8_t* ptr = (uint8_t*)malloc(width * height * 4);
			glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, ptr);
			// stb magic happens here
			stbi_write_png("screenshot.png", width, height, 4, ptr, 0);
			free(ptr);
		}
		});

	glfwMakeContextCurrent(window);
	gladLoadGL(glfwGetProcAddress);
	glfwSwapInterval(1);

	// Source and Bind shaders
	const GLuint shaderVertex = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(shaderVertex, 1, &shaderCodeVertex, nullptr);
	glCompileShader( shaderVertex );	

	const GLuint shaderFragment = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(shaderFragment, 1, &shaderCodeFragment, nullptr);
	glCompileShader( shaderFragment );	

	const GLuint program = glCreateProgram();

	glAttachShader(program, shaderVertex);
	glAttachShader(program, shaderFragment);

	glLinkProgram(program);

	// Empty VAO
	GLuint VAO;
	glCreateVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	// Modern OpenGl passes buffers a bit similar to Vulkan
	const GLsizeiptr kBufferSize = sizeof(PerFrameData);
	GLuint perFrameDataBuffer;
	glCreateBuffers(1, &perFrameDataBuffer);
	// Dynamic buffer storage means it can be updated after creation
	glNamedBufferStorage(perFrameDataBuffer, kBufferSize, nullptr, GL_DYNAMIC_STORAGE_BIT);
	// Binds buffer at target index 0, to be used in shader code
	glBindBufferRange(GL_UNIFORM_BUFFER, 0, perFrameDataBuffer, 0, kBufferSize);


	glClearColor(1.0f, 0.8f, 0.6f, 1.0f);		// Nothing new here
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_POLYGON_OFFSET_LINE);
	glPolygonOffset(-1.0f, -1.0f);

	// We want to load our data as a vec3 RGB
	int w, h, comp;
	const uint8_t* img = stbi_load("data/ch2_sample3_STB.jpg", &w, &h, &comp, 3);
	
	// Let's use our loaded image as an OpenGl texture
	GLuint texture;
	glCreateTextures(GL_TEXTURE_2D, 1, &texture);
	glTextureParameteri(texture, GL_TEXTURE_MAX_LEVEL, 0);
	glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTextureStorage2D(texture, 1, GL_RGB8, w, h);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTextureSubImage2D(texture, 0, 0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, img);
	glBindTextures(0, 1, &texture);

	stbi_image_free((void*)img);


	// The main Loop
	while (!glfwWindowShouldClose(window))
	{
		
		int width, height;
		glfwGetFramebufferSize( window, &width, &height );
		// Build simple MVP with glm
		const float ratio = width / (float)height;

		glViewport(0, 0, width, height);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Cube Model Matrix rotating 1,1,1 diag
		const mat4 m = glm::rotate(glm::translate(mat4(1.0f), vec3(0.0f, 0.0f, -3.5f)),
			(float)glfwGetTime(), vec3(1.0f, 1.0f, 1.0f));
		// Perspective matrix
		const mat4 p = glm::perspective(45.0f, ratio, 0.1f, 1000.0f);

		// Update uniform buffer and submit draw calls
		// Designated Initializer 
		PerFrameData perFrameData = { .mvp = p * m };

		glUseProgram(program);
		glNamedBufferSubData(perFrameDataBuffer, 0, kBufferSize, &perFrameData);
		glDrawArrays(GL_TRIANGLES, 0, 36);

		// Fragment output was rendered into the back buffer, swap
		glfwSwapBuffers(window);
		glfwPollEvents();
	}
	
	// Delete objects created
	glDeleteTextures(1, &texture);
	glDeleteBuffers(1, &perFrameDataBuffer);
	glDeleteProgram(program);
	glDeleteShader(shaderFragment);
	glDeleteShader(shaderVertex);
	glDeleteVertexArrays(1, &VAO);

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}
