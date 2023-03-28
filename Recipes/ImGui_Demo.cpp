#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include <stdio.h>
#include <stdlib.h>

#include <imgui/imgui.h>

using glm::mat4;
using glm::vec3;

// ImGui geometry data is used as vertex + Texture + Color Data
// Uniform PerframeData similar to its c++ counterpart
// Frag shader simply modulates vert color with a texture

const GLchar* shaderCodeVertex = R"(
	#version 460 core
	
	layout (location = 0) in vec2 Position;
	layout (location = 1) in vec2 UV;
	layout (location = 2) in vec4 Color;
	
	layout (std140, binding = 0) uniform PerFrameData
	{
		uniform mat4 MVP;
	};
	
	out vec2 Frag_UV;
	out vec4 Frag_Color;

	void main()
	{
		Frag_UV = UV;
		Frag_Color = Color;
		gl_Position = MVP * vec4(Position.xy, 0, 1);
	}
)";

const GLchar* shaderCodeFragment = R"(
	#version 460 core
	
	in vec2 Frag_UV;
	in vec4 Frag_Color;
	
	layout (binding = 0) uniform sampler2D Texture;
	layout (location = 0) out vec4 out_Color;

	void main()
	{
		out_Color = Frag_Color * texture(Texture, Frag_UV.st);
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
	});

	glfwSetCursorPosCallback(window,
		[](auto* window, double x, double y)
		{
			ImGui::GetIO().MousePos = ImVec2((float)x, (float)y);
		}
	);

	glfwSetMouseButtonCallback(window,
		[](auto* window, int button, int action, int mods)
		{
			auto& io = ImGui::GetIO();
			const int idx = button == GLFW_MOUSE_BUTTON_LEFT ? 0 : button == GLFW_MOUSE_BUTTON_RIGHT ? 2 : 1;
			io.MouseDown[idx] = action == GLFW_PRESS;
		}
	);

	glfwMakeContextCurrent(window);
	gladLoadGL(glfwGetProcAddress);
	glfwSwapInterval(1);

	// Create VAO to render ImGui Geometry
	GLuint VAO;
	glCreateVertexArrays(1, &VAO);

	GLuint handleVBO;
	glCreateBuffers(1, &handleVBO);
	glNamedBufferStorage(handleVBO, 128 * 1024, nullptr,
		GL_DYNAMIC_STORAGE_BIT);

	GLuint handleElements;
	glCreateBuffers(1, &handleElements);
	glNamedBufferStorage(handleElements, 256 * 1024, nullptr,
		GL_DYNAMIC_STORAGE_BIT);

	// Geometry consists of 2D vertex positions, textures and colors
	glVertexArrayElementBuffer(VAO, handleElements);
	// Structure ImDrawVert vec2 pos, vec2 uv, u32 color
	glVertexArrayVertexBuffer(VAO, 0, handleVBO, 0, sizeof(ImDrawVert));

	// Vertex attributes are stored interleaved format, this is how you enable + set them up
	glEnableVertexArrayAttrib(VAO, 0);
	glEnableVertexArrayAttrib(VAO, 1);
	glEnableVertexArrayAttrib(VAO, 2);

	// Use IM_OFFSETOF macro for offsets in struct
	glVertexArrayAttribFormat(VAO, 0, 2, GL_FLOAT, GL_FALSE,
		IM_OFFSETOF(ImDrawVert, pos));
	glVertexArrayAttribFormat(VAO, 1, 2, GL_FLOAT, GL_FALSE,
		IM_OFFSETOF(ImDrawVert, uv));
	glVertexArrayAttribFormat(VAO, 2, 4, GL_UNSIGNED_BYTE, GL_TRUE,
		IM_OFFSETOF(ImDrawVert, col));

	// Complete the VAO by telling OpenGL every vertex should be read from same Buffer
	glVertexArrayAttribBinding(VAO, 0, 0);
	glVertexArrayAttribBinding(VAO, 1, 0);
	glVertexArrayAttribBinding(VAO, 2, 0);

	glBindVertexArray(VAO);

	// Nothing unusual here, just binding/compiling the shaders
	const GLuint handleVertex = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(handleVertex, 1, &shaderCodeVertex, nullptr);
	glCompileShader(handleVertex);
	
	const GLuint handleFragment = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(handleFragment, 1, &shaderCodeFragment, nullptr);
	glCompileShader(handleFragment);

	const GLuint program = glCreateProgram();
	glAttachShader(program, handleVertex);
	glAttachShader(program, handleFragment);
	glLinkProgram(program);
	glUseProgram(program);

	//Not creating the struct for MVP, simply passing a mat4 
	GLuint perFrameDataBuffer;
	glCreateBuffers(1, &perFrameDataBuffer);
	glNamedBufferStorage(perFrameDataBuffer, sizeof(mat4), nullptr, GL_DYNAMIC_STORAGE_BIT);
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, perFrameDataBuffer);

	ImGui::CreateContext();
	
	// Little performance tip for the output mesh
	ImGuiIO& io = ImGui::GetIO();
	io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;

	// Build Texture Atlas
	ImFontConfig cfg = ImFontConfig();
	// Tell ImGui we are handling memory ourselves
	cfg.FontDataOwnedByAtlas = false;
	// Brighten up font from default 1.0
	cfg.RasterizerMultiply = 1.5f;
	// Calculate Pixel height of font
	cfg.SizePixels = 768.0f / 32.0f;
	// Allign glyph
	cfg.PixelSnapH = true;
	cfg.OversampleH = 4;
	cfg.OversampleV = 4;

	// Finally, load our font
	ImFont* Font = io.Fonts->AddFontFromFileTTF("data/OpenSans-Light.ttf", cfg.SizePixels, &cfg);

	// Now we have context + Atlas Bitmap, let's use it for OpenGl Texture
	unsigned char* pixels = nullptr;
	int width, height;
	io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

	// Texture Creation
	GLuint texture;
	glCreateTextures(GL_TEXTURE_2D, 1, &texture);
	glTextureParameteri(texture, GL_TEXTURE_MAX_LEVEL, 0);
	glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTextureStorage2D(texture, 1, GL_RGBA8, width, height);

	// Scanlines in ImGui Bitmap are not padded, remove unpack aligment in OpenGl
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTextureSubImage2D(texture, 0, 0, 0, width, height, 
		GL_RGBA, GL_UNSIGNED_BYTE, pixels);
	glBindTextures(0, 1, &texture);
	
	io.Fonts->TexID = (ImTextureID)(intptr_t)texture;
	io.FontDefault = Font;
	io.DisplayFramebufferScale = ImVec2(1, 1);
	
	// Process OpenGL state setup for rendering
	glEnable(GL_BLEND);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_SCISSOR_TEST);

	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	
	while (!glfwWindowShouldClose(window))
	{
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);

		glViewport(0, 0, width, height);
		glClear(GL_COLOR_BUFFER_BIT);

		// render ImGUi Demo, geometry data is generated in Render function
		ImGuiIO& io = ImGui::GetIO();
		io.DisplaySize = ImVec2((float)width, (float)height);
		ImGui::NewFrame();
		ImGui::ShowDemoWindow();
		ImGui::Render();
		const ImDrawData* draw_data = ImGui::GetDrawData();

		// Need to construct proper projection matrix based on clipping
		const float L = draw_data->DisplayPos.x;
		const float R = draw_data->DisplayPos.x + draw_data->DisplaySize.x;
		const float T = draw_data->DisplayPos.y;
		const float B = draw_data->DisplayPos.y + draw_data->DisplaySize.y;
		const mat4 orthoProj = glm::ortho(L, R, B, T);
		glNamedBufferSubData(perFrameDataBuffer, 0, sizeof(mat4), glm::value_ptr(orthoProj));

		// Now go through all the ImGUi Command List -> update content of the vertex buffer and
		// index buffer -> invoke rendering commands

		for (int i = 0; i < draw_data->CmdListsCount; i++)
		{
			const ImDrawList* cmd_list = draw_data->CmdLists[i];
			glNamedBufferSubData(handleVBO, 0, (GLsizeiptr)cmd_list->VtxBuffer.Size * sizeof(ImDrawVert),
				cmd_list->VtxBuffer.Data);
			glNamedBufferSubData(handleElements, 0, (GLsizeiptr)cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx),
				cmd_list->IdxBuffer.Data);
			// Rendering commands are stored in the command buffer
			for (int j = 0; j < cmd_list->CmdBuffer.Size; j++)
			{
				const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[j];
				const ImVec4 cr = pcmd->ClipRect;
				glScissor((int)cr.x, (int)(height - cr.w), (int)(cr.z - cr.x), (int)(cr.w - cr.y));
				glBindTextureUnit(0, (GLuint)(intptr_t)pcmd->TextureId);
				glDrawElementsBaseVertex(GL_TRIANGLES, (GLsizei)pcmd->ElemCount, GL_UNSIGNED_SHORT,
					(void*)(intptr_t)(pcmd->IdxOffset * sizeof(ImDrawIdx)), (GLint)pcmd->VtxOffset);
			}
		}
		

		glScissor(0, 0, width, height);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	ImGui::DestroyContext();

	glfwDestroyWindow(window);

	glfwTerminate();
	exit(EXIT_SUCCESS);

	return 0;
}
