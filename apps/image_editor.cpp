// image editor app
#include "image_editor.hh"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

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

int correctRGB(int channel){
	if(channel > 255){
		return 255;
	}
	if(channel < 0){
		return 0;
	}
	return channel;
}

// https://github.com/ocornut/imgui/wiki/Image-Loading-and-Displaying-Examples
bool importImage(const char* filename, GLuint* tex, GLuint* originalImage, int* width, int* height){
	int imageWidth = 0;
	int imageHeight = 0;
	unsigned char* imageData = stbi_load(filename, &imageWidth, &imageHeight, NULL, 4);
	if(imageData == NULL){
		return false;
	}
	
	glActiveTexture(GL_TEXTURE0);
	GLuint imageTexture;
	glGenTextures(1, &imageTexture);
	glBindTexture(GL_TEXTURE_2D, imageTexture);
	
	// TODO: understand this stuff
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	
	#if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
		glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	#endif
	
	// create the texture with the image data
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imageWidth, imageHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageData);
	
	// store the image in another texture that we won't touch (but just read from)
	glActiveTexture(GL_TEXTURE2);
	GLuint imageTexture2;
	glGenTextures(1, &imageTexture2);
	glBindTexture(GL_TEXTURE_2D, imageTexture2);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imageWidth, imageHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageData);
	
	stbi_image_free(imageData);
	
	*tex = imageTexture;
	*originalImage = imageTexture2;
	*width = imageWidth;
	*height = imageHeight;
	
	return true;
}

void showImageEditor(){
	ImGui::BeginChild("image editor", ImVec2(0, 800), true);
	
	static bool showImage = false;
	static GLuint texture = 0;
	static GLuint originalImage = 1;
	static int imageHeight = 0;
	static int imageWidth = 0;
	static char importImageFilepath[64] = "assets/test_image.png";
	
	// for filters that have customizable parameters,
	// have a bool flag so we can toggle the params
	static bool showSaturateParams = false;
	
	bool importImageClicked = ImGui::Button("import image");
	ImGui::SameLine();
	ImGui::InputText("filepath", importImageFilepath, 64);
	
	if(importImageClicked){
		// set the texture (the imported image) to be used for the filters
		// until a new image is imported, the current one will be used
		
		std::string filepath(importImageFilepath);
		
		if(trimString(filepath) != ""){
			bool loaded = importImage(filepath.c_str(), &texture, &originalImage, &imageWidth, &imageHeight);
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
		
		// make sure we use the right texture (since we also have one for rendering an offscreen frame buffer scene on)
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
		
		if (ImGui::Button("saturate")){
			showSaturateParams = !showSaturateParams;
		}
		
		if(showSaturateParams){
			static float lumG = 0.5f;
			static float lumR = 0.5f;
			static float lumB = 0.5f;
			static float saturationVal = 0.5f;
			
			unsigned char* pixelData = new unsigned char[pixelDataLen];
			
			// use the original texture to get pixel data from
			glActiveTexture(GL_TEXTURE2);
			glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelData);
			
			float r1 = ((1 - saturationVal) * lumR) + saturationVal;
			float g1 = ((1 - saturationVal) * lumG) + saturationVal;
			float b1 = ((1 - saturationVal) * lumB) + saturationVal;
			
			float r2 = (1 - saturationVal) * lumR;
			float g2 = (1 - saturationVal) * lumG;
			float b2 = (1 - saturationVal) * lumB;
			
			for(int i = 0; i <= pixelDataLen-4; i += 4){
				int r = (int)pixelData[i];
				int g = (int)pixelData[i+1];
				int b = (int)pixelData[i+2];
				
				int newR = (int)(r*r1 + g*g2 + b*b2);
				int newG = (int)(r*r2 + g*g1 + b*b2);
				int newB = (int)(r*r2 + g*g2 + b*b1);
				
				// ensure value is within range of 0 and 255
				newR = correctRGB(newR);
				newG = correctRGB(newG);
				newB = correctRGB(newB);
				
				pixelData[i] = (unsigned char)newR;
				pixelData[i+1] = (unsigned char)newG;
				pixelData[i+2] = (unsigned char)newB;
			}
			
			// after editing pixel data, write it to the other texture
			glActiveTexture(GL_TEXTURE0);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imageWidth, imageHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelData);
			delete pixelData;
			
			ImGui::SliderFloat("saturation val", &saturationVal, 0.0f, 5.0f);
			ImGui::SliderFloat("lumR", &lumR, 0.0f, 5.0f);
			ImGui::SliderFloat("lumG", &lumG, 0.0f, 5.0f);
			ImGui::SliderFloat("lumB", &lumB, 0.0f, 5.0f);
		}
	}
	
	ImGui::EndChild();
}