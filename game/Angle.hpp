#ifndef ANGLE_HPP
#define ANGLE_HPP

#include "commons.hpp"

class Angle : public GameObject {
	public:
		Angle();
		~Angle();

		void set(vec2f dir, float halfAngle);
		vec2f getDir() { return dir; }
		float getHalfAngle() { return half; }

		vec3f color = vec3f(0.0f, 1.0f, 0.0f);
		vec3f center = vec3f(0.0f);
		bool doDraw = false;
		float magnitude = 2.0f;

	private:
		vec2f dir = vec2f(0.0f, 1.0f);
		float half = M_PI/6.0f;

		void updateVerts();

		void update(float deltaTime) override;
		void draw() const override;

		mutable Mesh triangles;
		mutable Mesh lines;
};

#endif //ANGLE_HPP
