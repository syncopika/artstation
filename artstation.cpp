#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl3.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include <iostream>
#include <chrono>
#include <vector>
#include <set>
#include <stdio.h>
#include <SDL.h>
#include <GL/glew.h>

#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <SDL_opengles2.h>
#else
#include <SDL_opengl.h>
#endif

// https://github.com/ocornut/imgui/wiki/Image-Loading-and-Displaying-Examples
bool importImage(const char* filename, GLuint* tex, int* width, int* height){
	int image_width = 0;
	int image_height = 0;
	unsigned char* image_data = stbi_load(filename, &image_width, &image_height, NULL, 4);
	if(image_data == NULL){
		return false;
	}
	
	glActiveTexture(GL_TEXTURE0);
	GLuint image_texture;
	glGenTextures(1, &image_texture);
	glBindTexture(GL_TEXTURE_2D, image_texture);
	
	// TODO: understand this stuff
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	
	#if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
		glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	#endif
	
	// create the texture with the image data
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_width, image_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
	
	stbi_image_free(image_data);
	
	*tex = image_texture;
	*width = image_width;
	*height = image_height;
	
	return true;
}

std::string trimString(std::string str){
	std::string trimmed("");
	std::string::iterator it;
	for(it = str.begin(); it < str.end(); it++){
		if(*it != ' '){
			trimmed += *it;
		}
	}
	return trimmed;
}

void showDrawingCanvas(bool* p_open){
	// set up canvas for drawing on
	// TODO: be able to clear canvas (need to empty canvasPoints and lastSegmentIndexes)
	ImGui::Begin("drawing canvas");
	
	static std::vector<ImVec2> canvasPoints;
	
	// use this to keep track of where each separately drawn segement ends because everything gets redrawn each re-render from the beginning
	static std::set<int> lastSegmentIndexes;
	
	//static ImVec2 scrolling(0.0f, 0.0f);
	
	ImVec4 brushColor = ImVec4(1.0f, 1.0f, 0.4f, 1.0f);
	ImU32 yellow = ImColor(brushColor);
	
	ImVec2 canvas_pos0 = ImGui::GetCursorScreenPos();      // ImDrawList API uses screen coordinates!
	ImVec2 canvas_size = ImGui::GetContentRegionAvail();   // Resize canvas to what's available
	if (canvas_size.x < 150.0f) canvas_size.x = 150.0f;
	if (canvas_size.y < 150.0f) canvas_size.y = 150.0f;
	ImVec2 canvas_pos1 = ImVec2(canvas_pos0.x + canvas_size.x, canvas_pos0.y + canvas_size.y);

	// Draw border and background color
	ImGuiIO& io = ImGui::GetIO();
	ImDrawList* draw_list = ImGui::GetWindowDrawList();
	draw_list->AddRectFilled(canvas_pos0, canvas_pos1, IM_COL32(50, 50, 50, 255));
	draw_list->AddRect(canvas_pos0, canvas_pos1, IM_COL32(255, 255, 255, 255));

	// This will catch our interactions
	ImGui::InvisibleButton("drawingCanvas", canvas_size, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
	
	// Hovered - this is so we ensure that we take into account only mouse interactions that occur
	// on this particular canvas. otherwise it could pick up mouse clicks that occur on other windows as well.
	const bool is_hovered = ImGui::IsItemHovered();
	const bool is_active = ImGui::IsItemActive();   // Held
	
	const ImVec2 origin(canvas_pos0.x, canvas_pos0.y); // Lock scrolled origin
	const ImVec2 mouse_pos_in_canvas(io.MousePos.x - origin.x, io.MousePos.y - origin.y);

	if (is_active && is_hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
	{
		canvasPoints.push_back(mouse_pos_in_canvas);
	}
	
	ImVec2 drag_delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left);
	if (is_hovered && is_active && (drag_delta.x > 0 || drag_delta.y > 0))
	{
		canvasPoints.push_back(mouse_pos_in_canvas);
	}
	
	int lastIdx = -1;
	if (is_hovered && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
	{
		// I think we might be setting the last element in the canvasPoints vector to
		// mouse_pos_in_canvas? and since it's a reference, it works? don't think we need this though
		//canvasPoints.back() = mouse_pos_in_canvas;
		lastIdx = canvasPoints.size() - 1;
	}
	
	// need to redraw each event update
	int numPoints = (int)canvasPoints.size();
	for(int i = 0; i < numPoints; i++){
		ImVec2 p1 = ImVec2(origin.x + canvasPoints[i].x, origin.y + canvasPoints[i].y);
		ImVec2 p2 = ImVec2(origin.x + canvasPoints[i+1].x, origin.y + canvasPoints[i+1].y);
		
		if(lastSegmentIndexes.find(i) != lastSegmentIndexes.end()){
			//std::cout << "last point index: " << lastPointIndex << std::endl;				
			// don't draw a line from this point to the next
			draw_list->AddCircleFilled(p1, 4.0f, yellow, 10.0f);
			continue;
		}
		if(i < numPoints - 1){
			// connect the points
			draw_list->AddLine(p1, p2, yellow, 8.0f);
		}else{
			// just draw point
			draw_list->AddCircle(p1, 4.0f, yellow, 10.0f);
		}
	}
	
	if(lastIdx > -1){
		// keep track of the last point of the last segment drawn so we don't connect two separately drawn segments
		lastSegmentIndexes.insert(lastIdx); //lastPointIndex = lastIdx;
	}
	
	if(ImGui::Button("clear")){
		canvasPoints.clear();
		lastSegmentIndexes.clear();
	}
	
	ImGui::End();
}

void showImageEditor(bool* p_open){
	ImGui::Begin("image editor");
	
	static bool showImage = false;
	static GLuint texture = 0;
	static int imageHeight = 0;
	static int imageWidth = 0;
	static char importImageFilepath[64] = "";
	
	bool importImageClicked = ImGui::Button("import image");
	ImGui::SameLine();
	ImGui::InputText("filepath", importImageFilepath, 64);
	
	if(importImageClicked){
		// set the texture (the imported image) to be used for the filters
		// until a new image is imported, the current one will be used
		
		std::string filepath(importImageFilepath);
		
		if(trimString(filepath) != ""){
			bool loaded = importImage(filepath.c_str(), &texture, &imageWidth, &imageHeight);
			if(loaded){
				showImage = true;
			}else{
				ImGui::Text("import image failed");
			}
		}
	}
	
	if(showImage){
		// TODO: get an open file dialog working?
		ImGui::Text("size = %d x %d", imageWidth, imageHeight);
		ImGui::Image((void *)(intptr_t)texture, ImVec2(imageWidth, imageHeight));
		//ImGui::Text("image imported");
		
		int pixelDataLen = imageWidth*imageHeight*4; // 4 because rgba
		
		glActiveTexture(GL_TEXTURE0);
		
		if (ImGui::Button("grayscale")){	
			// grayscale the image data
			unsigned char* pixelData = new unsigned char[pixelDataLen];
			glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelData); // uses currently bound texture from importImage()
			
			for(int i = 0; i < pixelDataLen - 4; i+=4){
				unsigned char r = pixelData[i];
				unsigned char g = pixelData[i+1];
				unsigned char b = pixelData[i+2];
				
				unsigned char grey = (r+g+b)/3;
				pixelData[i] = grey;
				pixelData[i+1] = grey;
				pixelData[i+2] = grey;
			}
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imageWidth, imageHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelData);
			delete pixelData;
		}
		ImGui::SameLine();
		
		if (ImGui::Button("invert")){		
			unsigned char* pixelData = new unsigned char[pixelDataLen];
			glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelData); // uses currently bound texture from importImage()
			for(int i = 0; i < pixelDataLen - 4; i+=4){
				pixelData[i] = 255 - pixelData[i];
				pixelData[i+1] = 255 - pixelData[i+1];
				pixelData[i+2] = 255 - pixelData[i+2];
			}
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imageWidth, imageHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelData);
			delete pixelData;
		}
		ImGui::SameLine();
	}
	
	ImGui::End();
}

// https://github.com/tinyobjloader/tinyobjloader
void getObjModelInfo(auto& shapes, auto& attrib){
	for (size_t s = 0; s < shapes.size(); s++) {
	  // Loop over faces(polygon)
	  size_t index_offset = 0;
	  for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
		size_t fv = size_t(shapes[s].mesh.num_face_vertices[f]);

		// Loop over vertices in the face.
		for (size_t v = 0; v < fv; v++) {
		  // access to vertex
		  tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
		  tinyobj::real_t vx = attrib.vertices[3*size_t(idx.vertex_index)+0];
		  tinyobj::real_t vy = attrib.vertices[3*size_t(idx.vertex_index)+1];
		  tinyobj::real_t vz = attrib.vertices[3*size_t(idx.vertex_index)+2];

		  // Check if `normal_index` is zero or positive. negative = no normal data
		  if (idx.normal_index >= 0) {
			tinyobj::real_t nx = attrib.normals[3*size_t(idx.normal_index)+0];
			tinyobj::real_t ny = attrib.normals[3*size_t(idx.normal_index)+1];
			tinyobj::real_t nz = attrib.normals[3*size_t(idx.normal_index)+2];
		  }

		  // Check if `texcoord_index` is zero or positive. negative = no texcoord data
		  if (idx.texcoord_index >= 0) {
			tinyobj::real_t tx = attrib.texcoords[2*size_t(idx.texcoord_index)+0];
			tinyobj::real_t ty = attrib.texcoords[2*size_t(idx.texcoord_index)+1];
		  }

		  // Optional: vertex colors
		  // tinyobj::real_t red   = attrib.colors[3*size_t(idx.vertex_index)+0];
		  // tinyobj::real_t green = attrib.colors[3*size_t(idx.vertex_index)+1];
		  // tinyobj::real_t blue  = attrib.colors[3*size_t(idx.vertex_index)+2];
		}
		index_offset += fv;

		// per-face material
		//shapes[s].mesh.material_ids[f];
	  }
	}
}

// 3d stuff - TODO: move elsewhere
// shaders
const char* vertexShaderSource = R"glsl(
	#version 150 core

	in vec2 position;
	out vec2 pos;

	void main()
	{
		pos = position;
		gl_Position = vec4(position, 0.0, 1.0);
	}
)glsl";

const char* fragShaderSource = R"glsl(
	#version 150 core

	uniform float time;

	in vec2 pos;
	out vec4 outColor;

	void main()
	{
		float newR = cos(time*pos.x);
		float newG = sin(time*pos.y);
		float newB = cos(time*(pos.x+pos.y));
		outColor = vec4(newR, newG, newB, 1.0);
	}
)glsl";

GLuint shaderProgram;
void setupShaders(){
	// compile vertex shader
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
	glCompileShader(vertexShader);
	
	GLint status;
	
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &status);
	if(status != GL_TRUE){
		std::cout << "error compiling vertex shader!" << std::endl;
	}
	
	// compile frag shader
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragShaderSource, NULL);
	glCompileShader(fragmentShader);
	
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &status);
	if(status != GL_TRUE){
		std::cout << "error compiling fragment shader!" << std::endl;
	}
	
	// combine shaders into a program
	shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	
	glBindFragDataLocation(shaderProgram, 0, "outColor");
	glLinkProgram(shaderProgram);
	glUseProgram(shaderProgram);
	
	// set up position attribute in vertex shader
	GLint posAttrib = glGetAttribLocation(shaderProgram, "position");
	glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(posAttrib);
}

void setupTriangle(){
	float vertices[] = {
		0.0f, 0.5f,
		0.5f, 0.0f,
		-0.5f, 0.0f
	};
	
	// vertex buffer object
	GLuint vbo;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	
	// vertex array object
	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
}

// return the GLuint of the texture used for offscreen rendering
// TODO: pass in dimensions later?
void setupOffscreenFramebuffer(GLuint* frameBuffer, GLuint* texture){
	// render scene to frame buffer -> texture -> use texture as image to render in the GUI
	// create frame buffer
	glGenFramebuffers(1, frameBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, *frameBuffer);
	
	// create the texture that the scene will be rendered to (it will be empty initially)
	glActiveTexture(GL_TEXTURE1);
	glGenTextures(1, texture);
	glBindTexture(GL_TEXTURE_2D, *texture); // any gl texture operations now will use this texture
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 500, 500, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);  
	glBindTexture(GL_TEXTURE_2D, 0); // unbind the texture

	// configure frame buffer to use texture
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, *texture, 0);
	
	// depth buffer
	GLuint depth;
	glGenRenderbuffers(1, &depth);
	glBindRenderbuffer(GL_RENDERBUFFER, depth);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, 500, 500);
	glBindRenderbuffer(GL_RENDERBUFFER, 0); // unbind after allocating storage
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth);
	
	//GLenum drawBuffers[1] = {GL_COLOR_ATTACHMENT0};
	//glDrawBuffers(1, drawBuffers);
}

void show3dModelViewer(bool* p_open, GLuint offscreenFrameBuf, GLuint offscreenTexture, auto startTime){
	ImGui::Begin("3d model viewer");
	
	std::string filepath = "battleship.obj";
	tinyobj::ObjReaderConfig config;
	config.mtl_search_path = "./";
	
	tinyobj::ObjReader reader;
	
	if(!reader.ParseFromFile(filepath, config)){
		if(!reader.Error().empty()){
			ImGui::Text("TinyObjReader: %s", (reader.Error()).c_str());
			ImGui::End();
		}
	}else{
		if(!reader.Warning().empty()){
			ImGui::Text("TinyObjReader: %s", (reader.Warning()).c_str());
		}
		
		auto& attrib = reader.GetAttrib();
		auto& shapes = reader.GetShapes();
		//auto& materials = reader.GetMaterials();
		
		ImGui::Text("model has %d shapes", shapes.size());
		ImGui::Text("model has %d vertices", attrib.vertices.size());
		
		if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE){
			std::cout << "framebuffer error: " << glCheckFramebufferStatus(GL_FRAMEBUFFER) << std::endl;
		}else{
			// draw the triangle to the offscreen frame buffer
			// adjust the glviewport to be drawn to so the image comes out correctly
			glBindFramebuffer(GL_FRAMEBUFFER, offscreenFrameBuf);
			glViewport(0, 0, 500, 500);
			
			// for shaders
			auto now = std::chrono::high_resolution_clock::now();
			float time = std::chrono::duration_cast<std::chrono::duration<float>>(now - startTime).count();
			GLuint t = glGetUniformLocation(shaderProgram, "time");
			glUniform1f(t, time);
			
			glDrawArrays(GL_TRIANGLES, 0, 3);
			
			// grab the offscreen texture and draw it to an imgui image
			glActiveTexture(GL_TEXTURE1);
			int w = 500;
			int h = 500;
			int pixelDataLen = w*h*3;
			unsigned char* pixelData = new unsigned char[pixelDataLen];
			glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, pixelData);
			ImGui::Image((void *)(intptr_t)offscreenTexture, ImVec2(w, h));
			delete pixelData;
			
			// use default framebuffer again (show the gui window)
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}
	}
	
	ImGui::End();
}

// Main code
int main(int, char**)
{
    // Setup SDL
    // (Some versions of SDL before <2.0.10 appears to have performance/stalling issues on a minority of Windows systems,
    // depending on whether SDL_INIT_GAMECONTROLLER is enabled or disabled.. updating to latest version of SDL is recommended!)
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
    {
        printf("Error: %s\n", SDL_GetError());
        return -1;
    }

    // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100
    const char* glsl_version = "#version 100";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#elif defined(__APPLE__)
    // GL 3.2 Core + GLSL 150
    const char* glsl_version = "#version 150";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Always required on Mac
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif

    // Create window with graphics context
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Window* window = SDL_CreateWindow("ArtStation", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, window_flags);
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1); // Enable vsync

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != NULL);

    // Our state
    bool show_demo_window = true;
	static bool showCanvasForDrawingFlag = true;
	static bool showImageEditorFlag = true;
	static bool show3dModelViewerFlag = true;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
	
	// set up GLEW
	GLenum err = glewInit();
	if(GLEW_OK != err){
		std::cout << "problem with glew: " << glewGetErrorString(err) << std::endl;
	}
	setupTriangle();
	setupShaders();
	
	// set up for rendering scene to frame buffer -> texture -> use texture as image to render in the GUI
	GLuint offscreenFrameBuf;
	GLuint offscreenTexture;
	setupOffscreenFramebuffer(&offscreenFrameBuf, &offscreenTexture);
	
	// for fun shaders
	auto startTime = std::chrono::high_resolution_clock::now();
	
    // Main loop
    bool done = false;
    while (!done)
    {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                done = true;
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
                done = true;
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);
		
		// game presentation window
		// ideas: 
		//	- have a subwindow for image editing + export
		//  - have a subwindow for drawing? brush examples?
		// 	- docked windows?
		// 	- start working on learning how to import an fbx or obj model?
		//  - audio stuff too? :D
		
		if(showCanvasForDrawingFlag) 
			showDrawingCanvas(&showCanvasForDrawingFlag);
		
		if(showImageEditorFlag)
			showImageEditor(&showImageEditorFlag);
		
		if(show3dModelViewerFlag){
			show3dModelViewer(&show3dModelViewerFlag, offscreenFrameBuf, offscreenTexture, startTime);
		}
		
		/*
 		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("apps"))
			{
				ImGui::MenuItem("drawing canvas", NULL, &showCanvasForDrawing);
				ImGui::MenuItem("image editor", NULL, &showImageEditor);
				ImGui::EndMenu();
			}
			ImGui::EndMenuBar();
		} 
		*/

        // Rendering
        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}