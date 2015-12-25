#ifndef GRID_HPP
#define GRID_HPP

#include "commons.hpp"

class Grid : public GameObject {
	public:
		Grid();
		~Grid();

	private:
		void update(float deltaTime) override;
		void draw() const override;

		Texture2D gridTex;
		mutable MeshIndexed quad;
		ShaderProgram program;
};

#endif //GRID_HPP
