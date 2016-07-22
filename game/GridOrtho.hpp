#ifndef GRIDORTHO_HPP
#define GRIDORTHO_HPP

#include "commons.hpp"

class GridOrtho : public GameObject {
    public:
        GridOrtho();
        ~GridOrtho();

        enum Face {
            MINX = 0,
            MAXX,
            MINY,
            MAXY,
            MINZ,
            MAXZ,
        };

    private:
        void initGridTex();
        void initQuadMesh();
        void initLinesMesh();

        vec2f getRelPos() const;
        vec2i getMouseCellCoords() const;
        void toggleBlock();

        void calcSquares();
        void updateGridTex();

        void update(float deltaTime) override;
        void draw() const override;

        std::vector<std::vector<bool>> blockers;
        Texture2D gridTex;
        mutable MeshIndexed quad;
        mutable Mesh lines;
        vec3f sunDir = glm::normalize(vec3f(-1.0f, 1.4f, 0.0f));
};

#endif //GRIDORTHO_HPP
