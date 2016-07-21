#ifndef GRID_HPP
#define GRID_HPP

#include "commons.hpp"

class Grid : public GameObject {
    public:
        Grid();
        ~Grid();

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

        void calcAngles();
        void updateGridTex();

        void update(float deltaTime) override;
        void draw() const override;

        std::vector<std::vector<bool>> blockers;
        Texture2D gridTex;
        mutable MeshIndexed quad;
        mutable Mesh lines;

        vec3i origin = {16, 16, 16};
};

#endif //GRID_HPP
