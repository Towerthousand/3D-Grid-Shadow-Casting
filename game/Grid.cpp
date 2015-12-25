#include "Grid.hpp"

Grid::Grid() {
	// Setup texture
	gridTex = Texture2D(vec2ui(100), TextureFormat::RGBA8);
	std::vector<char> pixels(100*100*4, 255);
	gridTex.setData(&pixels[0], TextureFormat::RGBA, TextureFormat::UNSIGNED_BYTE);

	// Setup model
	std::vector<Vertex::Attribute> elems = {
		Vertex::Attribute("a_position", Vertex::Attribute::Float, 3)
	};
	std::vector<vec3f> data = {
		vec3f(1, -1, 0), vec3f(1, 1, 0), vec3f(-1, 1, 0),
		vec3f(-1, -1, 0)
	};
	std::vector<unsigned int> indexes = {
		0, 1, 2, 3, 0, 2
	};
	quad = MeshIndexed(Vertex::Format(elems));
	quad.setVertexData(&data[0], 6);
	quad.setIndexData(&indexes[0], 6);
	quad.setPrimitiveType(Mesh::TRIANGLES);

	// Setup program
	program = ShaderProgram(
		Storage::openAsset("shaders/quad.vert"),
		Storage::openAsset("shaders/quad.frag")
	);
}

Grid::~Grid() {
}

void Grid::update(float deltaTime) {
	(void) deltaTime;
}

void Grid::draw() const {
	const Camera* cam = (Camera*) getGame()->getObjectByName("mainCamera");
	program.uniform("MVP")->set(cam->projection*cam->getView());
	quad.draw(program);
}
