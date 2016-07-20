#ifndef ANGLE_HPP
#define ANGLE_HPP

#include "commons.hpp"

struct AngleDef{
    vec3f dir;
    float halfAngle; //stored as tan(half angle)
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

        static AngleDef angleUnion(const AngleDef& a, const AngleDef& b);
        static AngleDef angleIntersection(const AngleDef& a, const AngleDef& b);

        void set(const AngleDef& newDef);
        vec3f getDir() const { return def.dir; }
        float getHalfAngle() const { return def.halfAngle; }
        bool isFull() const { return def.full; }
        const AngleDef& getDef() const { return def; }
        static AngleOverlap overlapTest(const AngleDef& a, const AngleDef& b);

        vec3f color = vec3f(0.0f, 1.0f, 0.0f);
        vec3f center = vec3f(0.0f);
        bool doDraw = false;
        float magnitude = 2.0f;

    private:
        AngleDef def = {{0.0f, 0.0f, 0.0f}, M_PI/6.0f, false};

        void updateVerts();
        void update(float deltaTime) override;
        void draw() const override;

        mutable Mesh triangles;
        mutable Mesh lines;
};

#endif //ANGLE_HPP
