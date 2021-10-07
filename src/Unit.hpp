#pragma once

#include <glm/glm.hpp>

class Unit
{
	public: 
		Unit(glm::vec2 pos, glm::vec2 size, float angle)
			: m_pos(pos), m_size(size), m_angle(angle)
		{};

		inline void move(glm::vec2 pos) { m_pos += pos; };
		inline void turn(float angle) { m_angle += angle; };

		inline float getAngle() { return m_angle; };
		inline glm::vec2 getPos() { return m_pos; };

	private:
		glm::vec2 m_pos;
		glm::vec2 m_size;
		float m_angle;
};
