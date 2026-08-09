#include "camera.h"

Camera::Camera()
    : position(0.0f, 0.0f, 3.0f),
      front(0.0f, 0.0f, -1.0f),
      up(0.0f, 1.0f, 0.0f),
      right(1.0f, 0.0f, 0.0f),
      worldUp(0.0f, 1.0f, 0.0f),
      yaw(YAW),
      pitch(PITCH),
      movementSpeed(SPEED),
      fov(ZOOM),
      aspectRatio(4.0f / 3.0f),
      nearPlane(0.1f),
      farPlane(100.0f) {}




void Camera::setPosition(float x, float y, float z) {
    position = glm::vec3(x, y, z);
}

void Camera::setRotation(float pitch, float yaw, float roll) {
    this->pitch = pitch;
    this->yaw = yaw;
    // Note: roll is not used in a first-person camera setup
    updateCameraVectors();
}

void Camera::setAspectRatio(float aspectRatio) {
    this->aspectRatio = aspectRatio;
}

glm::mat4 Camera::getViewMatrix() const {
    return glm::lookAt(position, position + front, up);
}

glm::mat4 Camera::getProjectionMatrix() const {
    return glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane);
}

const glm::vec3& Camera::getPosition() const {
    return position;
}

float Camera::getNearPlane() const {
    return nearPlane;
}

float Camera::getFarPlane() const {
    return farPlane;
}

void Camera::processInput(float deltaTime, CameraMovement movement) {
    float speed = movementSpeed * deltaTime;

    switch (movement) {
        case CameraMovement::FORWARD:
            position += speed * front;
            break;
        case CameraMovement::BACKWARD:
            position -= speed * front;
            break;
        case CameraMovement::LEFT:
            position -= speed * right;
            break;
        case CameraMovement::RIGHT:
            position += speed * right;
            break;
        case CameraMovement::UP:
            position += speed * up;
            break;
        case CameraMovement::DOWN:
            position -= speed * up;
            break;
    }
}

void Camera::processMouseMovement(float xOffset, float yOffset) {
    yaw += xOffset * SENSITIVITY;
    pitch = glm::clamp(pitch + yOffset * SENSITIVITY, -89.0f, 89.0f);
    updateCameraVectors();
}

void Camera::updateCameraVectors() {
    // calculate the new Front vector
    glm::vec3 _front;
    _front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    _front.y = sin(glm::radians(pitch));
    _front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    front = glm::normalize(_front);
    // also re-calculate the Right and Up vector
    right = glm::normalize(glm::cross(front, worldUp));  // normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
    up    = glm::normalize(glm::cross(right, front));
}
