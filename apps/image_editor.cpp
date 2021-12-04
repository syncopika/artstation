// image editor app
#include "image_editor.hh"

#include "stb_image.h"
#include "stb_image_write.h"

#define IMAGE_DISPLAY GL_TEXTURE2
#define ORIGINAL_IMAGE GL_TEXTURE3

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
bool importImage(const char* filename, GLuint* tex, GLuint* originalImage, int* width, int* height, int* channels){
    int imageWidth = 0;
    int imageHeight = 0;
    int imageChannels = 4; //rgba
    stbi_set_flip_vertically_on_load(false);
    unsigned char* imageData = stbi_load(filename, &imageWidth, &imageHeight, &imageChannels, 4);
    if(imageData == NULL){
        return false;
    }
    
    glActiveTexture(IMAGE_DISPLAY);
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
    glActiveTexture(ORIGINAL_IMAGE);
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
    *channels = imageChannels;
    
    return true;
}

void showImageEditor(){
    //ImGui::BeginChild("image editor", ImVec2(0, 800), true);
    
    static bool showImage = false;
    static GLuint texture;
    static GLuint originalImage;
    static int imageHeight = 0;
    static int imageWidth = 0;
    static int imageChannels = 4; //rgba
    static char importImageFilepath[64] = "assets/test_image.png";
    
    // for filters that have customizable parameters,
    // have a bool flag so we can toggle the params
    static bool showSaturateParams = false;
    static bool showOutlineParams = false;
    
    bool importImageClicked = ImGui::Button("import image");
    ImGui::SameLine();
    ImGui::InputText("filepath", importImageFilepath, 64);
    
    if(importImageClicked){
        // set the texture (the imported image) to be used for the filters
        // until a new image is imported, the current one will be used
        std::string filepath(importImageFilepath);
        
        if(trimString(filepath) != ""){
            bool loaded = importImage(filepath.c_str(), &texture, &originalImage, &imageWidth, &imageHeight, &imageChannels);
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
        unsigned char* pixelData = new unsigned char[pixelDataLen];
        
        // use GL_TEXTURE0 to display our edited image
        glActiveTexture(IMAGE_DISPLAY);
        
        // GRAYSCALE
        if(ImGui::Button("grayscale")){
            // get current image
            glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelData); // uses currently bound texture from importImage()
            
            // modify it
            for(int i = 0; i < pixelDataLen - 4; i+=4){
                unsigned char r = pixelData[i];
                unsigned char g = pixelData[i+1];
                unsigned char b = pixelData[i+2];
                unsigned char grey = (r+g+b)/3;
                pixelData[i] = grey;
                pixelData[i+1] = grey;
                pixelData[i+2] = grey;
            }
            
            // write it back
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imageWidth, imageHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelData);
        }
        ImGui::SameLine();
        
        // INVERT
        if(ImGui::Button("invert")){
            glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelData); // uses currently bound texture from importImage()
            for(int i = 0; i < pixelDataLen - 4; i+=4){
                pixelData[i] = 255 - pixelData[i];
                pixelData[i+1] = 255 - pixelData[i+1];
                pixelData[i+2] = 255 - pixelData[i+2];
            }
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imageWidth, imageHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelData);
        }
        ImGui::SameLine();
        
        // SATURATION
        if(ImGui::Button("saturate")){
            showSaturateParams = !showSaturateParams;
            showOutlineParams = false; // TODO: figure out better way to do this
        }
        ImGui::SameLine();
        
        // OUTLINE
        if(ImGui::Button("outline")){
            showOutlineParams = !showOutlineParams;
            showSaturateParams = false;
        }
        ImGui::SameLine();
        
        // EXPORT IMAGE
        if(ImGui::Button("export image (.bmp)")){
            glActiveTexture(IMAGE_DISPLAY);
            glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelData);
            
            // TODO: allow image naming
            // TODO: hardcoding 4 channels b/c not sure we were getting the right channel value back for the imageChannels variable?
            stbi_write_bmp("artstation_image_export.bmp", imageWidth, imageHeight, 4, (void *)pixelData);
        }
        
        if(showSaturateParams){
            static float lumG = 0.5f;
            static float lumR = 0.5f;
            static float lumB = 0.5f;
            static float saturationVal = 0.5f;
            
            // use another texture to get pixel data (notice GL_TEXTURE2 instead of GL_TEXTURE0)
            // so that we don't clobber the same image texture with repeated operations (we always want to work on a fresh copy)
            glActiveTexture(ORIGINAL_IMAGE);
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
            glActiveTexture(IMAGE_DISPLAY);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imageWidth, imageHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelData);
            
            ImGui::SliderFloat("saturation val", &saturationVal, 0.0f, 5.0f);
            ImGui::SliderFloat("lumR", &lumR, 0.0f, 5.0f);
            ImGui::SliderFloat("lumG", &lumG, 0.0f, 5.0f);
            ImGui::SliderFloat("lumB", &lumB, 0.0f, 5.0f);
        }
        
        if(showOutlineParams){
            unsigned char* sourceImageCopy = new unsigned char[pixelDataLen];
            
            // use the original texture to get pixel data from
            glActiveTexture(ORIGINAL_IMAGE);
            glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelData);
            glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, sourceImageCopy);
            
            // for each pixel, check the above pixel (if it exists)
            // if the above pixel is 'significantly' different (i.e. more than +/- 5 of rgb),
            // color the above pixel black and the current pixel white. otherwise, both become white. 
            static int limit = 10;
            
            bool setSameColor = false;
            
            for(int i = 0; i < imageHeight; i++){
                for(int j = 0; j < imageWidth; j++){
                    // the current pixel is i*width + j
                    // the above pixel is (i-1)*width + j
                    if(i > 0){
                        int aboveIndexR = (i-1)*imageWidth*4 + j*4;
                        int aboveIndexG = (i-1)*imageWidth*4 + j*4 + 1;
                        int aboveIndexB = (i-1)*imageWidth*4 + j*4 + 2;
                        
                        int currIndexR = i*imageWidth*4 + j*4;
                        int currIndexG = i*imageWidth*4 + j*4 + 1;
                        int currIndexB = i*imageWidth*4 + j*4 + 2;
                        
                        uint8_t aboveR = sourceImageCopy[aboveIndexR];
                        uint8_t aboveG = sourceImageCopy[aboveIndexG];
                        uint8_t aboveB = sourceImageCopy[aboveIndexB];
                    
                        uint8_t currR = sourceImageCopy[currIndexR]; 
                        uint8_t currG = sourceImageCopy[currIndexG];
                        uint8_t currB = sourceImageCopy[currIndexB];
                        
                        if(aboveR - currR < limit && aboveR - currR > -limit){
                            if(aboveG - currG < limit && aboveG - currG > -limit){
                                if(aboveB - currB < limit && aboveB - currB > -limit){
                                    setSameColor = true;
                                }else{
                                    setSameColor = false;
                                }
                            }else{
                                setSameColor = false;
                            }
                        }else{
                            setSameColor = false;
                        }
                        
                        if(!setSameColor){
                            pixelData[aboveIndexR] = 0;
                            pixelData[aboveIndexG] = 0;
                            pixelData[aboveIndexB] = 0;
                            
                            pixelData[currIndexR] = 255; 
                            pixelData[currIndexG] = 255; 
                            pixelData[currIndexB] = 255; 
                        }else{
                            pixelData[aboveIndexR] = 255;
                            pixelData[aboveIndexG] = 255;
                            pixelData[aboveIndexB] = 255;
                            
                            pixelData[currIndexR] = 255; 
                            pixelData[currIndexG] = 255; 
                            pixelData[currIndexB] = 255; 
                        }
                    }
                }
            }
            
            delete[] sourceImageCopy;
            
            // after editing pixel data, write it to the other texture
            glActiveTexture(IMAGE_DISPLAY);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imageWidth, imageHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelData);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imageWidth, imageHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelData);
            
            ImGui::SliderInt("color difference limit", &limit, 1, 20);
        }
        
        delete[] pixelData;
    }
    
    //ImGui::EndChild();
}