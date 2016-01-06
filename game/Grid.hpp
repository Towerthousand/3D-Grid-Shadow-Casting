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
			bool visible = false;
			Angle* angle = nullptr;
		};

		enum Dir {
			RIGHT = 0,
			UP,
			LEFT,
			DOWN
		};

		AngleDef getAngle(int x, int y, Dir d) const;

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

		vec2i origin = vec2i(10, 10);
		Angle* test1 = nullptr;
		Angle* test2 = nullptr;
		Angle* test3 = nullptr;
};

#endif //GRID_HPP
