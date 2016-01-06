#ifndef ANGLE_HPP
#define ANGLE_HPP

#include "commons.hpp"

struct AngleDef{
	vec2f dir;
	float halfAngle;
	bool full;
};

class Angle : public GameObject {
	private:
		enum AngleOverlap {
			INVALID = -1,
			NONE = 0,
			PARTIAL,
			CONTAINS
		};

	public:
		Angle();
		~Angle();

		static AngleDef angleUnion(const Angle* a, const Angle* b);
		static AngleDef angleIntersection(const Angle* a, const Angle* b);

		void set(const AngleDef& a) {
			set(a.dir, a.halfAngle, a.full);
		}
		void set(vec2f dir, float halfAngle, bool full);
		vec2f getDir() const { return dir; }
		float getHalfAngle() const { return half; }
		bool isFull() const {
			return full;
		}
		AngleOverlap overlapTest(const Angle* other) const {
			return overlapTest(other->getDir(), other->getHalfAngle(), other->full);
		}
		AngleOverlap overlapTest(const AngleDef& other) const {
			return overlapTest(other.dir, other.halfAngle, other.full);
		}
		AngleOverlap overlapTest(const vec2f& dir, float half, bool full) const;

		vec3f color = vec3f(0.0f, 1.0f, 0.0f);
		vec3f center = vec3f(0.0f);
		bool doDraw = false;
		float magnitude = 2.0f;

	private:
		vec2f dir = vec2f(0.0f, 1.0f);
		float half = M_PI/6.0f;
		bool full = false;

		void updateVerts();

		void update(float deltaTime) override;
		void draw() const override;

		mutable Mesh triangles;
		mutable Mesh lines;
};

#endif //ANGLE_HPP
