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

		std::pair<vec2f, float>  getAngle(int x, int y) const;

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

		vec2i origin = vec2i(10,10);
};

#endif //GRID_HPP
