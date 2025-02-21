#include "3d_viewer.hh"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include "stb_image.h"

#define WIDTH 500
#define HEIGHT 500

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

// https://www.scratchapixel.com/lessons/mathematics-physics-for-computer-graphics/lookat-function
glm::mat4 lookAt(const glm::vec3& from, const glm::vec3& to, const glm::vec3& tmp = glm::vec3(0, 1, 0)){
    glm::vec3 forward = glm::normalize(from - to);
    glm::vec3 right = glm::cross(glm::normalize(tmp), forward);
    glm::vec3 up = glm::cross(forward, right);
    
    glm::mat4 camToWorld;
    
    camToWorld[0][0] = right.x;
    camToWorld[0][1] = right.y;
    camToWorld[0][2] = right.z;
    camToWorld[0][3] = 0;
    
    camToWorld[1][0] = up.x;
    camToWorld[1][1] = up.y;
    camToWorld[1][2] = up.z;
    camToWorld[1][3] = 0;
    
    camToWorld[2][0] = forward.x;
    camToWorld[2][1] = forward.y;
    camToWorld[2][2] = forward.z;
    camToWorld[2][3] = 0;
    
    camToWorld[3][0] = from.x;
    camToWorld[3][1] = from.y;
    camToWorld[3][2] = from.z;
    camToWorld[3][3] = 1;
    
    return camToWorld;
}

// https://github.com/tinyobjloader/tinyobjloader
void getObjModelInfo(
    auto& shapes, 
    auto& attrib, 
    std::vector<glm::vec3>& vertices,
    std::vector<glm::vec2>& uvs,
    std::vector<glm::vec3>& normals
){
    for(size_t s = 0; s < shapes.size(); s++){
      // Loop over faces(polygon)
      size_t index_offset = 0;
      for(size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++){
        size_t fv = size_t(shapes[s].mesh.num_face_vertices[f]);

        // Loop over vertices in the face.
        for(size_t v = 0; v < fv; v++){
          // access to vertex
          tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
          float vx = (float)attrib.vertices[3*size_t(idx.vertex_index)+0];
          float vy = (float)attrib.vertices[3*size_t(idx.vertex_index)+1];
          float vz = (float)attrib.vertices[3*size_t(idx.vertex_index)+2];
          vertices.push_back(glm::vec3(vx, vy, vz));

          // Check if `normal_index` is zero or positive. negative = no normal data
          if(idx.normal_index >= 0){
            float nx = (float)attrib.normals[3*size_t(idx.normal_index)+0];
            float ny = (float)attrib.normals[3*size_t(idx.normal_index)+1];
            float nz = (float)attrib.normals[3*size_t(idx.normal_index)+2];
            normals.push_back(glm::vec3(nx, ny, nz));
          }

          // Check if `texcoord_index` is zero or positive. negative = no texcoord data
          if(idx.texcoord_index >= 0){
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
    layout(location = 2) in vec3 normal;
    
    out vec3 pos;
    out vec2 uv;
    out vec3 norm;
    
    uniform float u_time;
    uniform mat4 mvp;

    void main()
    {
        pos = position;
        uv = vertexUV;
        norm = normal;
        gl_Position = mvp * vec4(position, 1.0);
    }
)glsl";

const char* fragShaderSource = R"glsl(
    #version 330 core

    uniform float u_time;
    uniform sampler2D theTexture;

    in vec3 pos;
    in vec2 uv;
    in vec3 norm;
    
    out vec4 FragColor;

    void main()
    {
        //float newR = cos(u_time)/2; //cos(u_time*pos.x);
        //float newG = sin(u_time)/2; //sin(u_time*pos.y);
        //float newB = cos(u_time)/2; //cos(u_time*pos.y);
        
        // add some lighting
        vec3 lightPos = vec3(0.0f, 6.0f, -6.0f);
        vec3 lightColor = vec3(1.0f, 1.0f, 1.0f);
        
        float ambientStrength = 0.1f;
        vec3 ambient = ambientStrength * lightColor;
        
        vec3 n = normalize(norm);
        vec3 lightDir = normalize(lightPos - pos);
        float diff = max(dot(n, lightDir), 0.0f);
        vec3 diffuse = diff * lightColor;
        
        vec3 color = (ambient + diffuse) * vec3(texture(theTexture, uv));
        FragColor = vec4(color, 1.0f); //vec4(newR, newG, newB, 1.0);
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
    
    // cleanup
    glDetachShader(shaderProgram, vertexShader);
    glDetachShader(shaderProgram, fragmentShader);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

void recompileShaders(GLuint& shaderProgram, std::string& vertexShaderSrc, std::string& fragShaderSrc, std::string& err){
    
    GLint status;
    
    // try compiling vertex shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    const char* vertShader = vertexShaderSrc.c_str();
    glShaderSource(vertexShader, 1, &vertShader, NULL);
    glCompileShader(vertexShader);
    
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &status);
    if(status != GL_TRUE){
        std::cout << "error compiling vertex shader!\n";
        err = std::string("error compiling vertex shader!");
        glDeleteShader(vertexShader);
        return;
    }
    
    // try compiling frag shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    const char* frgShader = fragShaderSrc.c_str();
    glShaderSource(fragmentShader, 1, &frgShader, NULL);
    glCompileShader(fragmentShader);
    
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &status);
    if(status != GL_TRUE){
        std::cout << "error compiling fragment shader!\n";
        err = std::string("error compiling fragment shader!");
        glDeleteShader(fragmentShader);
        return;
    }
    
    // delete old shader program only if vertex and frag shaders compiled
    glUseProgram(0); // important!
    glDeleteProgram(shaderProgram);
    
    // combine shaders into a new program
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    
    glBindFragDataLocation(shaderProgram, 0, "outColor");
    glLinkProgram(shaderProgram);
    glUseProgram(shaderProgram);
    
    // cleanup
    glDetachShader(shaderProgram, vertexShader);
    glDetachShader(shaderProgram, fragmentShader);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    err = "";
}

// return the GLuint of the texture used for offscreen rendering
// TODO: pass in dimensions later?
void setupOffscreenFramebuffer(GLuint* frameBuffer, GLuint* texture){
    // render scene to frame buffer -> texture -> use texture as image to render in the GUI
    // create and bind frame buffer
    glGenFramebuffers(1, frameBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, *frameBuffer);
    
    // create the texture that the scene will be rendered to (it will be empty initially)
    glActiveTexture(GL_TEXTURE0); // seems to be ok to use the default texture unit
    glGenTextures(1, texture);
    glBindTexture(GL_TEXTURE_2D, *texture); // any gl texture operations now will use this texture
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, WIDTH, HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);  
    glBindTexture(GL_TEXTURE_2D, 0); // unbind the texture

    // configure frame buffer to use texture
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, *texture, 0);
    
    // depth buffer
    GLuint depth;
    glGenRenderbuffers(1, &depth);
    glBindRenderbuffer(GL_RENDERBUFFER, depth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, WIDTH, HEIGHT);
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
    GLuint normalBuffer,
    GLuint matrixId,
    GLuint materialTexture,
    std::string& materialTextureName,
    std::chrono::time_point<std::chrono::high_resolution_clock> startTime
){
    static char importObjFilepath[64] = "assets/battleship-edit2.obj";
    static std::string filepath(importObjFilepath);
    
    static bool toggleWireframe = false;
    
    static std::string errMsg("");
    
    // keep track of time uniform var for shader
    static float time = 0;
    
    static std::string vertexShaderEditable(
      R"glsl(
        #version 330 core

        layout(location = 0) in vec3 position;
        layout(location = 1) in vec2 vertexUV;
        layout(location = 2) in vec3 normal;
        
        out vec3 pos;
        out vec2 uv;
        out vec3 norm;
        
        uniform float u_time;
        uniform mat4 mvp;

        void main()
        {
            pos = position;
            uv = vertexUV;
            norm = normal;
            gl_Position = mvp * vec4(position, 1.0);
        }
      )glsl"
    );
    
    static std::string fragmentShaderEditable(
      R"glsl(
        #version 330 core

        uniform float u_time;
        uniform sampler2D theTexture;

        in vec3 pos;
        in vec2 uv;
        in vec3 norm;
        
        out vec4 FragColor;

        void main()
        {   
            // add some lighting
            vec3 lightPos = vec3(0.0f, 6.0f, -6.0f);
            vec3 lightColor = vec3(1.0f, 1.0f, 1.0f);
            
            float ambientStrength = 0.1f;
            vec3 ambient = ambientStrength * lightColor;
            
            vec3 n = normalize(norm);
            vec3 lightDir = normalize(lightPos - pos);
            float diff = max(dot(n, lightDir), 0.0f);
            vec3 diffuse = diff * lightColor;
            
            vec3 color = (ambient + diffuse) * vec3(texture(theTexture, uv));
            FragColor = vec4(color, 1.0f);
        }
      )glsl"
    );
    
    bool importObjClicked = ImGui::Button("import .obj model");
    ImGui::SameLine();
    ImGui::InputText("filepath", importObjFilepath, 64);
    
    if(importObjClicked){
        // update filepath with new model path
        filepath = std::string(importObjFilepath);
    }
    
    tinyobj::ObjReaderConfig config;
    config.mtl_search_path = "";
    
    tinyobj::ObjReader reader;
    
    if(!reader.ParseFromFile(filepath, config)){
        if(!reader.Error().empty()){
            ImGui::Text("TinyObjReader: %s", (reader.Error()).c_str());
        }
        ImGui::Text("error reading in .obj file. is the path correct?");
        return;
    }
    
    if(!reader.Warning().empty()){
        ImGui::Text("TinyObjReader: %s", (reader.Warning()).c_str());
    }
    
    // set up some static variables to let user control camera
    static Camera cam;
    static float modelPosX = 0.0f;
    static float modelPosY = 0.0f;
    static float modelPosZ = -8.0f;
    
    auto& attrib = reader.GetAttrib();
    auto& shapes = reader.GetShapes();
    auto& materials = reader.GetMaterials();
    
    std::vector<glm::vec3> verts;
    std::vector<glm::vec2> uvs;
    std::vector<glm::vec3> normals;
    
    ImGui::Text("model has %d shapes", shapes.size());
    ImGui::Text("model has %d vertices", attrib.vertices.size());
    
    getObjModelInfo(shapes, attrib, verts, uvs, normals);
    
    ImGuiIO& io = ImGui::GetIO();
    
    ImGui::Columns(2, "objviewer");
    
    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE){
        std::cout << "framebuffer error: " << glCheckFramebufferStatus(GL_FRAMEBUFFER) << '\n';
    }else{
        // load the image data from the file specified by the materials
        std::string texName = "assets/" + materials[0].diffuse_texname;
        
        if(materialTextureName != texName){
            // this conditional should only happen once when a new obj is loaded
            materialTextureName = texName;
            
            int imgWidth, imgHeight, numChannels=4;
            
            stbi_set_flip_vertically_on_load(true);
            
            unsigned char* imgData = stbi_load(
                texName.c_str(),
                &imgWidth,
                &imgHeight,
                &numChannels,
                4
            );
            
            if(imgData == NULL){
                std::cout << "failed to load texture: " << texName << "\n";
                
                // load in placeholder
                imgData = stbi_load(
                    "assets/color.png",
                    &imgWidth,
                    &imgHeight,
                    &numChannels,
                    4
                );
            }else{
                //std::cout << "r: " << (int)imgData[0] << ", g: " << (int)imgData[1] << ", b: " << (int)imgData[2] << ", a: " << (int)imgData[3] << '\n';
                //std::cout << "uv size: " << uvs.size() << '\n';
            }
            
            // prep for using image as texture
            // important to specify the active texture to use otherwise when switching between apps that use textures, 
            // we might use the wrong active texture and get a segmentation fault
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, materialTexture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imgWidth, imgHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, imgData);
            glBindTexture(GL_TEXTURE_2D, 0); // done with this texture for now so unbind
            stbi_image_free(imgData);
        }
        
        int viewportHeight = HEIGHT;
        int viewportWidth = WIDTH;
        
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
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        
        // uvs
        glBindBuffer(GL_ARRAY_BUFFER, uvBuffer);
        glBufferData(GL_ARRAY_BUFFER, uvs.size()*sizeof(glm::vec2), &uvs[0], GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        
        // normals
        glBindBuffer(GL_ARRAY_BUFFER, normalBuffer);
        glBufferData(GL_ARRAY_BUFFER, normals.size()*sizeof(glm::vec3), &normals[0], GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        
        // variables for shaders
        //auto now = std::chrono::high_resolution_clock::now();
        //float time = std::chrono::duration_cast<std::chrono::duration<float>>(now - startTime).count();
        time += 0.005f; // https://github.com/syncopika/threejs-projects/blob/master/shaders/index.js#L105
        GLuint t = glGetUniformLocation(shaderProgram, "u_time");
        glUniform1f(t, time);
        
        // set up project matrix
        float fovAngle = 60.0f;
        float aspect = WIDTH / HEIGHT;
        float near = 0.01f;
        float far = 100.0f;
        glm::mat4 perspectiveMat = createPerspectiveMatrix(fovAngle, aspect, near, far);
        
        // store y rotation and rotate based on last y rotation so the model will rotate 360
        static glm::mat4 lastYRot(1.0);
        lastYRot = glm::rotate(lastYRot, angleToRads(0.5f), glm::vec3(0,1,0));
        
        // the following should do: move model to origin, scale it, rotate about x, rotate about y, then move it to final position (i.e. further away from the camera along z)
        // these operations here are ones we want to keep constant through each render, so we do them on the identity matrix (not using the previous object's transformation matrix)
        glm::mat4 model(1.0);
        model = glm::translate(model, glm::vec3(modelPosX, modelPosY, modelPosZ)); // final position of model
        model = model * lastYRot; // rotate about Y based on last rotation
        model = glm::rotate(model, angleToRads(180.0f), glm::vec3(1, 0, 0)); // rotate about x 180 deg to make model right side up
        model = glm::scale(model, glm::vec3(0.5f, 0.5f, 0.5f)); // scale down by half
        model = glm::translate(model, glm::vec3(0, 0, 0)); // place at center (0,0,0)
        
        // multiply perspectiveMat, view and model matrices to get the final view
        glm::mat4 viewMat = glm::lookAt(cam.getCameraPos(), glm::vec3(modelPosX, modelPosY, modelPosZ), glm::vec3(0, cam.up, 0));
        glm::mat4 mvp = perspectiveMat * viewMat * model;
        
        // give mvp info to shader
        glUniformMatrix4fv(matrixId, 1, GL_FALSE, &mvp[0][0]);
        
        // set up attributes for the shader (vertex position, uv, normals)
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        // 0 means the 0th element of the inputs to the vertex shader
        // 3 means the size of the input (b/c vec3 in this case)
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
        
        // pass the uv coords to the vertex shader
        glBindBuffer(GL_ARRAY_BUFFER, uvBuffer);
        // this buffer is separate from the vbo, so use 0 and 0 for the last args
        // if vertex and uv data were in the same buffer, then those values would be different
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
        
        // pass the normal coords to the vertex shader
        glBindBuffer(GL_ARRAY_BUFFER, normalBuffer);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
        
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, materialTexture); // make sure we use the loaded texture
        glDrawArrays(GL_TRIANGLES, 0, verts.size());
        glBindTexture(GL_TEXTURE_2D, 0);
        
        // draw texture to an imgui image
        int pixelDataLen = viewportWidth*viewportHeight*3;
        unsigned char* pixelData = new unsigned char[pixelDataLen];
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, pixelData);
        
        ImVec2 pos = ImGui::GetCursorScreenPos();
        
        // This will catch our interactions with the 3d model display
        ImGui::InvisibleButton("3dCanvas", ImVec2(viewportWidth, viewportHeight), ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
        
        // Hovered - this is so we ensure that we take into account only mouse interactions that occur
        // on this particular canvas. otherwise it could pick up mouse clicks that occur on other windows as well.
        const bool isHovered = ImGui::IsItemHovered();
        
        // handle any input for trackball
        ImVec2 dragDelta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left);
        if(isHovered && (std::abs(dragDelta.x) > 0 || std::abs(dragDelta.y) > 0)){
            //std::cout << "x: " << dragDelta.x << ", y: " << dragDelta.y << '\n';
            cam.rotate(-dragDelta.x/800, dragDelta.y/800); // moving mouse up is a negative delta y
        }
        
        if(isHovered && io.MouseWheel != 0.0f){
            if(io.MouseWheel < 0){
                // scroll backwards, zoom out
                cam.zoom(-1.0f);
            }else{
                cam.zoom(1.0f);
            }
        }
        
        //ImGui::SetCursorPos(ImVec2(ImGui::GetWindowSize().x/3, 90)); // center the image
        ImGui::SetCursorScreenPos(pos);
        ImGui::Image((void *)(intptr_t)offscreenTexture, ImVec2(viewportWidth, viewportHeight));
        
        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);
        glDisableVertexAttribArray(2);
        
        delete[] pixelData;
        
        // unbind offscreenFrameBuf and use default framebuffer again (show the gui window) - I think that's how this works?
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    
    //ImGui::Indent(ImGui::GetWindowSize().x/3); // attempt to center the button
    if(ImGui::Button("toggle wireframe")){
        toggleWireframe = !toggleWireframe; 
    }
    
    ImGui::NextColumn();
    
    // shader editing
    // https://github.com/ocornut/imgui/issues/4511
    ImVec2 shaderEditorSize(500, 300);
    ImGui::InputTextMultiline("vertex shader", &vertexShaderEditable, shaderEditorSize);
    ImGui::InputTextMultiline("fragment shader", &fragmentShaderEditable, shaderEditorSize);
    
    if(ImGui::Button("update shader")){
      // update shaders
      recompileShaders(shaderProgram, vertexShaderEditable, fragmentShaderEditable, errMsg);
    }
    
    if(errMsg != ""){
      ImGui::SameLine();
      ImGui::Text(errMsg.c_str());
    }

    ImGui::Columns();
}