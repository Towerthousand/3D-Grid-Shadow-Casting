#ifndef GRID_HPP
#define GRID_HPP

#include "Angle.hpp"

class Grid : public GameObject {
    public:
        Grid();
        ~Grid();

    private:
        struct Cell {
            bool block = false;
            Angle* angle = nullptr;
        };

        enum Face {
            MINX = 0,
            MAXX,
            MINY,
            MAXY,
            MINZ,
            MAXZ,
        };

        AngleDef getAngle(vec3i pos, Face f, vec3i origin) const;

        void resetCells();
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

        std::vector<std::vector<Cell>> cells;
        Texture2D gridTex;
        mutable MeshIndexed quad;
        mutable Mesh lines;

        vec3i origin = {16, 16, 16};
        bool genMode2D = false;
        bool approxMode = false;
};

#endif //GRID_HPP
