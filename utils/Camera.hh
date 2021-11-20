#ifndef CAMERA_H
#define CAMERA_H

#include <math.h>
#include <GLM/glm.hpp>
#include <GLM/vec3.hpp>

#define M_PI 3.14159265358979323846

// helpful! https://computergraphics.stackexchange.com/questions/151/how-to-implement-a-trackball-in-opengl

class Camera {
    public:
        float theta;
        float phi;
        float radius;
        float up;
        glm::vec3 targetPos;
    
        Camera();
        
        void rotate(float deltaTheta, float deltaPhi);
        void pan(float deltaX, float deltaY);
        void zoom(float distance);
        
        glm::vec3 getCameraPos();
        
        ~Camera();
        
    private:        
        glm::vec3 toCartesian();
};


#endif