#include "Camera.hh"

void Camera::rotate(float deltaTheta, float deltaPhi){
    theta += deltaTheta;
    phi += deltaPhi;
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
