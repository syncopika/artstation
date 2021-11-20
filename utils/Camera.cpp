#include "Camera.hh"

// random note: prefer initialization lists
// https://stackoverflow.com/questions/6822422/c-where-to-initialize-variables-in-constructor
// theta and phi are 1.0 because that allows the object to appear initially
// if they're set to 0.0, you have to click and drag first to get the object to show. not sure why atm.
Camera::Camera() : theta(1.0f), phi(1.0f), radius(3.0f), up(1.0f), targetPos{0, 0, -8} {}

void Camera::rotate(float deltaTheta, float deltaPhi){
    if(up > 0.0f){
        theta += deltaTheta;
    }else{
        theta -= deltaTheta;
    }
    
    phi += deltaPhi;
    
    // keep phi within -2PI to 2PI
    if(phi > 2*M_PI){
        phi -= 2*M_PI;
    }else if(phi < -2*M_PI){
        phi += 2*M_PI;
    }
    
    // if (0 < phi < PI), 'up' should be positive Y,
    // otherwise, negative Y
    if(phi > 0.0f && phi < M_PI){
        up = 1.0f;
    }else{
        up = -1.0f;
    }
}

void Camera::pan(float deltaX, float deltaY){
    glm::vec3 look = glm::normalize(toCartesian());
    glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 right = glm::cross(look, worldUp);
    glm::vec3 up = glm::cross(look, right);
    targetPos += targetPos + (right *  deltaX) + (up * deltaY);
}

void Camera::zoom(float distance){
    radius -= distance;
}

glm::vec3 Camera::getCameraPos(){
    return targetPos + toCartesian();
}

glm::vec3 Camera::toCartesian(){
    float x = radius * sinf(phi) * sinf(theta);
    float y = radius * cosf(phi);
    float z = radius * sinf(phi) * cos(theta);
    return glm::vec3{x, y, z};
}

Camera::~Camera(){
}
