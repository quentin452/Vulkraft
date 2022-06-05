#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>



class Camera {
    private:
        const float rotSpeed =  glm::radians(60.f);
        const float mouseRes = 500.0f;
        glm::mat4 CamMat;
        glm::mat3 CamDir;
        glm::vec3 CamPos;
        glm::vec3 CamAng;
        

    private:
        void updateDirection();
    
    public:

        Camera();
        ~Camera();


        glm::mat4 getCamera();
        glm::vec3 getPosition();
        void setPOsition(glm::vec3 position);
        void updatePosition(glm::vec3 direction);
        glm::mat3 getDirection();
        glm::vec3 getAngle(){return CamAng;};
        void updateAngle(double x , double y);
        glm::mat4 getMatrix();


};