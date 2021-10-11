#include "3d_viewer.hh"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

float angleToRads(float angleInDeg){
	float pi = 3.14159;
	return (pi / 180) * angleInDeg;
}

// https://stackoverflow.com/questions/3498581/in-opengl-what-is-the-simplest-way-to-create-a-perspective-view-using-only-open
glm::mat4 createPerspectiveMatrix(float fovAngle, float aspect, float near, float far){
	glm::mat4 m(0.0);
	
	float fovRadians = angleToRads(fovAngle);
	float f = 1.0f / tan(fovRadians/2.0f);
	
	m[0][0] = f / aspect;
	m[1][1] = f;
	m[2][2] = (far + near)/(near - far);
	m[2][3] = -1.0f;
	m[3][2] = (2.0f * far * near) / (near - far);

	return m;
}

// https://github.com/tinyobjloader/tinyobjloader
void getObjModelInfo(
	auto& shapes, 
	auto& attrib, 
	std::vector<glm::vec3>& vertices,
	std::vector<glm::vec2>& uvs,
	std::vector<glm::vec3>& normals
	){
	for (size_t s = 0; s < shapes.size(); s++) {
	  // Loop over faces(polygon)
	  size_t index_offset = 0;
	  for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
		size_t fv = size_t(shapes[s].mesh.num_face_vertices[f]);

		// Loop over vertices in the face.
		for (size_t v = 0; v < fv; v++) {
		  // access to vertex
		  tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
		  float vx = (float)attrib.vertices[3*size_t(idx.vertex_index)+0];
		  float vy = (float)attrib.vertices[3*size_t(idx.vertex_index)+1];
		  float vz = (float)attrib.vertices[3*size_t(idx.vertex_index)+2];
		  vertices.push_back(glm::vec3(vx, vy, vz));

		  // Check if `normal_index` is zero or positive. negative = no normal data
		  if (idx.normal_index >= 0) {
			float nx = (float)attrib.normals[3*size_t(idx.normal_index)+0];
			float ny = (float)attrib.normals[3*size_t(idx.normal_index)+1];
			float nz = (float)attrib.normals[3*size_t(idx.normal_index)+2];
			normals.push_back(glm::vec3(nx, ny, nz));
		  }

		  // Check if `texcoord_index` is zero or positive. negative = no texcoord data
		  if (idx.texcoord_index >= 0) {
			float tx = (float)attrib.texcoords[2*size_t(idx.texcoord_index)+0];
			float ty = (float)attrib.texcoords[2*size_t(idx.texcoord_index)+1];
			uvs.push_back(glm::vec2(tx, ty));
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
	#version 330 core

	layout(location = 0) in vec3 position;
	layout(location = 1) in vec2 vertexUV;
	
	out vec2 uv;
	out vec3 pos;
	
	uniform mat4 mvp;

	void main()
	{
		pos = position;
		uv = vertexUV;
		gl_Position = mvp * vec4(position, 1.0);
	}
)glsl";

const char* fragShaderSource = R"glsl(
	#version 330 core

	uniform float time;

	in vec3 pos;
	out vec4 outColor;

	void main()
	{
		float newR = cos(time)/2; //cos(time*pos.x);
		float newG = sin(time)/2; //sin(time*pos.y);
		float newB = cos(time)/2; //cos(time*pos.y);
		outColor = vec4(newR, newG, newB, 1.0);
	}
)glsl";

//GLuint shaderProgram;
void setupShaders(GLuint& shaderProgram){
	// compile vertex shader
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
	glCompileShader(vertexShader);
	
	GLint status;
	
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &status);
	if(status != GL_TRUE){
		std::cout << "error compiling vertex shader!\n";
	}
	
	// compile frag shader
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragShaderSource, NULL);
	glCompileShader(fragmentShader);
	
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &status);
	if(status != GL_TRUE){
		std::cout << "error compiling fragment shader!\n";
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
	glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(posAttrib);
}

// return the GLuint of the texture used for offscreen rendering
// TODO: pass in dimensions later?
void setupOffscreenFramebuffer(GLuint* frameBuffer, GLuint* texture){
	// render scene to frame buffer -> texture -> use texture as image to render in the GUI
	// create and bind frame buffer
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
	
	// unbind framebuffer now since we don't need it yet (we just needed to bind it to set it up)
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	
	//GLenum drawBuffers[1] = {GL_COLOR_ATTACHMENT0};
	//glDrawBuffers(1, drawBuffers);
}

void show3dModelViewer(
	GLuint offscreenFrameBuf, 
	GLuint offscreenTexture, 
	GLuint shaderProgram,
	GLuint vbo,
	GLuint vao,
	GLuint uvBuffer,
	GLuint matrixId,
	std::chrono::time_point<std::chrono::high_resolution_clock> startTime
	){
		
	ImGui::BeginChild("3d model viewer", ImVec2(0, 760), true);
	
	static bool toggleWireframe = false;
	
	std::string filepath("assets/battleship.obj");
	tinyobj::ObjReaderConfig config;
	config.mtl_search_path = "./";
	
	tinyobj::ObjReader reader;
	
	if(!reader.ParseFromFile(filepath, config)){
		if(!reader.Error().empty()){
			ImGui::Text("TinyObjReader: %s", (reader.Error()).c_str());
			ImGui::EndChild();
		}
	}else{
		if(!reader.Warning().empty()){
			ImGui::Text("TinyObjReader: %s", (reader.Warning()).c_str());
		}
		
		// set up some static variables to let user control camera
		static float cameraCurrX = 0.0f;
		static float cameraCurrY = 0.0f;
		static float cameraCurrZ = -15.0f;
		
		auto& attrib = reader.GetAttrib();
		auto& shapes = reader.GetShapes();
		//auto& materials = reader.GetMaterials();
		std::vector<glm::vec3> verts;
		std::vector<glm::vec2> uvs;
		std::vector<glm::vec3> normals;
		
		ImGui::Text("model has %d shapes", shapes.size());
		ImGui::Text("model has %d vertices", attrib.vertices.size());
		
		getObjModelInfo(shapes, attrib, verts, uvs, normals);
		
		// uvs
		//glBindBuffer(GL_ARRAY_BUFFER, uvBuffer);
		//glBufferData(GL_ARRAY_BUFFER, sizeof(uvs)*sizeof(glm::vec2), &uvs[0], GL_STATIC_DRAW);
		
		if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE){
			std::cout << "framebuffer error: " << glCheckFramebufferStatus(GL_FRAMEBUFFER) << '\n';
		}else{
			
			int viewportHeight = 500;
			int viewportWidth = 500;
			
			// draw to the offscreen frame buffer
			// adjust the glviewport to be drawn to so the image comes out correctly
			glBindFramebuffer(GL_FRAMEBUFFER, offscreenFrameBuf);
			glViewport(0, 0, viewportWidth, viewportHeight);
			glClearColor(0.62f, 0.73f, 0.78f, 0.0f); // the color of the background
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			
			if(toggleWireframe){
				glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			}else{
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			}
			
			glEnable(GL_DEPTH_TEST);
			glDepthFunc(GL_LESS);
			glEnable(GL_CULL_FACE);
			
			// load the vertices for the model
			glBindBuffer(GL_ARRAY_BUFFER, vbo);
			glBufferData(GL_ARRAY_BUFFER, verts.size()*sizeof(glm::vec3), &verts[0], GL_STATIC_DRAW);
			
			// for shaders
			auto now = std::chrono::high_resolution_clock::now();
			float time = std::chrono::duration_cast<std::chrono::duration<float>>(now - startTime).count();
			GLuint t = glGetUniformLocation(shaderProgram, "time");
			glUniform1f(t, time);
			
			// set up project matrix
			float fovAngle = 60.0f;
			float aspect = 1.0f; // 500 x 500 screen
			float near = 0.01f;
			float far = 100.0f;
			glm::mat4 perspectiveMat = createPerspectiveMatrix(fovAngle, aspect, near, far);
			
			// set up modelview matrix (and do all the necessary transformations)
			// store y rotation and rotate based on last y rotation so the model will rotate 360
			static glm::mat4 lastYRot(1.0);
			lastYRot = glm::rotate(lastYRot, angleToRads(0.5f), glm::vec3(0,1,0));
			
			// the following should do: move model to origin, scale it, rotate about x, rotate about y, then move it to final position (i.e. further away from the camera along z)
			// these operations here are ones we want to keep constant through each render, so we do them on the identity matrix (not using the previous object's transformation matrix)
			glm::mat4 mvp(1.0);
			mvp = glm::translate(mvp, glm::vec3(cameraCurrX, cameraCurrY, cameraCurrZ));
			mvp = mvp * lastYRot; // rotate about Y based on last rotation
			mvp = glm::rotate(mvp, angleToRads(180.0f), glm::vec3(1, 0, 0)); // rotate about x 180 deg to make model right side up
			mvp = glm::scale(mvp, glm::vec3(0.5f, 0.5f, 0.5f)); // scale down by half
			mvp = glm::translate(mvp, glm::vec3(0, 0, 0)); // place at center (0,0,0)
			
			// multiply perspectiveMat and modelview to get the final view
			perspectiveMat = perspectiveMat * mvp;
			
			glUniformMatrix4fv(matrixId, 1, GL_FALSE, &perspectiveMat[0][0]);
			
			// set up attributes to vertex buffer
			glEnableVertexAttribArray(0);
			glBindBuffer(GL_ARRAY_BUFFER, vbo);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
			
 			//glEnableVertexAttribArray(1);
			//glBindBuffer(GL_ARRAY_BUFFER, uvBuffer);
			//glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
			
			glDrawArrays(GL_TRIANGLES, 0, verts.size());
			
			glDisableVertexAttribArray(0);
			//glDisableVertexAttribArray(1);
			
			// grab the offscreen texture and draw it to an imgui image
			glActiveTexture(GL_TEXTURE1);
			int pixelDataLen = viewportWidth*viewportHeight*3;
			unsigned char* pixelData = new unsigned char[pixelDataLen];
			glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, pixelData);
			
			// center the image
			ImGui::SetCursorPos(ImVec2(ImGui::GetWindowSize().x/3, 90));
			ImGui::Image((void *)(intptr_t)offscreenTexture, ImVec2(viewportWidth, viewportHeight));
			
			delete pixelData;
			
			// unbind offscreenFrameBuf and use default framebuffer again (show the gui window) - I think that's how this works?
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}
		
		//ImGui::Indent(ImGui::GetWindowSize().x/2); TODO: can't seem to use 2 indents? :/
		// I'd like to indent the button a different amount from the amount for the sliders
		
		ImGui::Indent(ImGui::GetWindowSize().x/3); // center the sliders
		if(ImGui::Button("toggle wireframe")){
			toggleWireframe = !toggleWireframe; 
		}
		
		ImGui::PushItemWidth(ImGui::GetWindowSize().x/4); // make the sliders a bit smaller in length
		
		ImGui::Dummy(ImVec2(0.0f, 3.0f)); // add some vertical spacing
		
		// control camera x movement
		ImGui::SliderFloat("move along x", &cameraCurrX, -5.0f, 5.0f);
		ImGui::Dummy(ImVec2(0.0f, 3.0f));
		
		// control camera y movement
		ImGui::SliderFloat("move along y", &cameraCurrY, -5.0f, 5.0f);
		ImGui::Dummy(ImVec2(0.0f, 3.0f));
		
		// control camera z movement
		ImGui::SliderFloat("move along z", &cameraCurrZ, -25.0f, 0.0f);
		ImGui::Dummy(ImVec2(0.0f, 1.0f));
	}
	
	ImGui::EndChild();
}