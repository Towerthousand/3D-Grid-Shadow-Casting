#ifndef GRIDORTHO_HPP
#define GRIDORTHO_HPP

#include "commons.hpp"

struct Square {
    static Square squareUnion(const Square& s1, const Square& s2);
    static Square squareIntersection(const Square& s1, const Square& s2);
    vec2f p;
    vec2f d;
};

const Log&operator << (const Log& log, const Square& s);

class GridOrtho : public GameObject {
    public:
        GridOrtho();
        ~GridOrtho();

    private:
        struct Cell {
            bool block = false;
            Square sq;
        };

        enum Dir {
            RIGHT = 0,
            UP,
            LEFT,
            DOWN
        };

        Square getSquare(int x, int y, Dir d) const;

        void resetCells();
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

        std::vector<std::vector<Cell>> cells;
        Texture2D gridTex;
        mutable MeshIndexed quad;
        mutable Mesh lines;
        vec3f sunDir = glm::normalize(vec3f(-1.0f, 1.4f, 0.0f));
};

#endif //GRIDORTHO_HPP
