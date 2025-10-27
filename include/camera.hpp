#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct Camera {
	glm::vec3 pos{0, 30, 80};
	float yaw = -90.0f, pitch = -20.0f;
	float fov = 60.0f;

	glm::mat4 view() const {
		glm::vec3 front{
			cos(glm::radians(yaw)) * cos(glm::radians(pitch)),
			sin(glm::radians(pitch)),
			sin(glm::radians(yaw)) * cos(glm::radians(pitch))
		};
		return glm::lookAt(pos, pos + glm::normalize(front), {0,1,0});
	}
};
