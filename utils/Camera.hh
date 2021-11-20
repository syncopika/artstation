#ifndef CAMERA_H
#define CAMERA_H

#include <math.h>
#include <GLM/glm.hpp>
#include <GLM/vec3.hpp>

// helpful! https://computergraphics.stackexchange.com/questions/151/how-to-implement-a-trackball-in-opengl

class Camera {
    public:
        // random note: prefer initialization lists
        // https://stackoverflow.com/questions/6822422/c-where-to-initialize-variables-in-constructor
        Camera() : theta(0.0f), phi(0.0f), radius(10.0f), up(1.0f), targetPos{0, 0, 0} {}
        
        void rotate(float deltaTheta, float deltaPhi);
        void pan(float deltaX, float deltaY);
        void zoom(float distance);
        
        glm::vec3 getCameraPos();
        
        ~Camera();
        
    private:
        float theta;
        float phi;
        float radius;
        float up;
        glm::vec3 targetPos;
        
        glm::vec3 toCartesian();
};


#endif