#include "camera.hpp"
#include <algorithm>

Camera::Camera(){
    CamMat = getMatrix();
    CamDir = glm::mat3(1.0f);
    CamPos = glm::vec3(16.0f , 50.0f , 16.0f);
    CamAng = glm::vec3(0.0f);
}


void Camera::updateDirection(){
    CamDir =    glm::mat3(glm::rotate(glm::mat4(1.0f), CamAng.y, glm::vec3(0.0f, 1.0f, 0.0f))) *
                glm::mat3(glm::rotate(glm::mat4(1.0f), CamAng.x, glm::vec3(1.0f, 0.0f, 0.0f))) *
                glm::mat3(glm::rotate(glm::mat4(1.0f), CamAng.z, glm::vec3(0.0f, 0.0f, 1.0f)));
}

void Camera::updatePosition(glm::vec3 direction){
    CamPos += glm::vec3(glm::rotate(glm::mat4(1.0f), CamAng.y, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(direction.x, direction.y, direction.z, 1));
}

glm::vec3 Camera::getPosition(){
    return CamPos;
}

void Camera::updateAngle(double x , double y){
    
    CamAng.y += x * rotSpeed / mouseRes;
    CamAng.x += y * rotSpeed / mouseRes;
    CamAng.x = std::clamp(CamAng.x, glm::radians(-90.0f) , glm::radians(90.0f));


    updateDirection();
}




glm::mat4 Camera::getMatrix(){
    CamMat = glm::translate(glm::transpose(glm::mat4(CamDir)), -CamPos) ;
    return CamMat;
}

Camera::~Camera(){

}