#ifndef SCENE_HPP
#define SCENE_HPP

#include "commons.hpp"

class Scene : public GameObject {
    public:
        Scene();
        ~Scene();

        const Camera* getCamera() const {
            return camera;
        }
        float getZoom() const {
            return zoom;
        }

    private:
        void update(float deltaTime) override;
        void draw() const override;

        void updateView(float deltaTime);

        Camera* camera = nullptr;
        float zoom = 10.0f;
        vec3f pos = vec3f(0.0f);
};

#endif //SCENE_HPP
