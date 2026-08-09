#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace wgfx {

enum class CameraMovement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT,
    UP,
    DOWN
};

const float YAW         = -90.0f;
const float PITCH       =  0.0f;
const float SPEED       =  2.5f;
const float SENSITIVITY =  0.1f;
const float ZOOM        =  45.0f;

class Camera {
public:
    Camera();

    void setPosition(float x, float y, float z);
    void setRotation(float pitch, float yaw, float roll);
    void processInput(float deltaTime, CameraMovement movement);
    void processMouseMovement(float xOffset, float yOffset);
    void setAspectRatio(float aspectRatio);

    [[nodiscard]] glm::mat4 getViewMatrix() const;
    [[nodiscard]] glm::mat4 getProjectionMatrix() const;
    [[nodiscard]] const glm::vec3& getPosition() const;
    [[nodiscard]] float getNearPlane() const;
    [[nodiscard]] float getFarPlane() const;

private:
    glm::vec3 position;
    glm::vec3 front;
    glm::vec3 up;
    glm::vec3 right;
    glm::vec3 worldUp;
    float yaw;
    float pitch;
    float movementSpeed;
    float fov;
    float aspectRatio;
    float nearPlane;
    float farPlane;

    void updateCameraVectors();
};

} // namespace wgfx
