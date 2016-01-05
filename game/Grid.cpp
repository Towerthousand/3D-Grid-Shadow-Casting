#include "Grid.hpp"
#include "Scene.hpp"
#include "Manager.hpp"

#define GRIDSIZE 21

Grid::Grid() {
	cells = std::vector<std::vector<Cell>>(GRIDSIZE, std::vector<Grid::Cell>(GRIDSIZE));
	resetCells();
	initGridTex();
	initQuadMesh();
	initLinesMesh();
	updateGridTex();

	test1 = new Angle();
	test1->addTo(this);
	test1->doDraw = true;
	test1->magnitude = 1.0f;
	test1->color = vec3f(0.0f, 0.0f, 1.0f);

	test2 = new Angle();
	test2->addTo(this);
	test2->doDraw = true;
	test2->magnitude = 1.0f;
	test2->color = vec3f(0.0f, 0.0f, 1.0f);

	test3 = new Angle();
	test3->addTo(this);
	test3->doDraw = true;
	test3->color = vec3f(0.0f, 1.0f, 1.0f);

	test1->set(glm::normalize(vec2f(-1.0f, 1.0f)), glm::tan(M_PI/6.0f), false);
	test2->set(glm::normalize(vec2f(1.0f)), glm::tan(M_PI/6.0f), false);
	test3->set(vec2f(0.0f, 1.0f), glm::tan(M_PI/6.0f), false);
}

Grid::~Grid() {
}

AngleDef Grid::getAngle(int x, int y) const {
	if(vec2i(x, y) == origin)
		return {vec2f(0.0f, 1.0f), 0.0f, true};
	vec2f center = vec2f(x, y)+0.5f;
	vec2f orig = vec2f(origin)+0.5f;
	float ballRadius = sqrt(2.0f)*0.5f;
	float angle = ballRadius/(glm::length(center-orig)-ballRadius);
	return {glm::normalize(center-orig), angle, true};
}

void Grid::resetCells() {
	for(int x = 0; x < GRIDSIZE; ++x)
		for(int y = 0; y < GRIDSIZE; ++y) {
			if(cells[x][y].angle != nullptr)
				continue;
			cells[x][y].angle = new Angle();
			cells[x][y].angle->addTo(this);
			AngleDef a = getAngle(x, y);
			cells[x][y].angle->set(a);
		}
}

void Grid::initGridTex() {
	gridTex = Texture2D(vec2ui(GRIDSIZE), TextureFormat::RGBA8);
	gridTex.setFilter(GL_NEAREST, GL_NEAREST);
}

void Grid::initQuadMesh() {
	std::vector<Vertex::Attribute> elems = {
		Vertex::Attribute("a_position", Vertex::Attribute::Float, 3),
		Vertex::Attribute("a_texCoord", Vertex::Attribute::Float, 2)
	};
	struct Vert { vec3f c; vec2f t; };
	std::vector<Vert> data = {
		{vec3f( 1, -1, 0), vec2f(1.0f, 0.0f)},
		{vec3f( 1,  1, 0), vec2f(1.0f, 1.0f)},
		{vec3f(-1,  1, 0), vec2f(0.0f, 1.0f)},
		{vec3f(-1, -1, 0), vec2f(0.0f, 0.0f)}
	};
	std::vector<unsigned int> indexes = {
		0, 1, 2, 3, 0, 2
	};
	quad = MeshIndexed(Vertex::Format(elems));
	quad.setVertexData(&data[0], 4);
	quad.setIndexData(&indexes[0], 6);
	quad.setPrimitiveType(MeshIndexed::TRIANGLES);
}

void Grid::initLinesMesh() {
	std::vector<Vertex::Attribute> elems = {
		Vertex::Attribute("a_position", Vertex::Attribute::Float, 3)
	};
	std::vector<vec3f> lineData;
	for(float x = -1; x <= 1; x += 2.0f/21.0f) {
		lineData.push_back(vec3f(x,-1, 0));
		lineData.push_back(vec3f(x, 1, 0));
		lineData.push_back(vec3f(-1, x, 0));
		lineData.push_back(vec3f( 1, x, 0));
	}
	lines = Mesh(Vertex::Format(elems));
	lines.setVertexData(&lineData[0], lineData.size());
	lines.setPrimitiveType(Mesh::LINES);
}

vec2f Grid::getRelPos() const {
	Scene* scene = (Scene*)getGame()->getObjectByName("SCENE");
	vec2i pos = Mouse::position();
	vec2ui windowSize = Window::getInstance()->getSize();
	//zoom scaled to match the board scale (10.0f)
	float zoom = scene->getZoom()/10.0f;
	// (set 0,0 to the center of the screen (board center initially)
	pos -= vec2i(windowSize/(uint)2);
	// top is .5, bottom is .5, y's need to be inverted since mouse coords are in -y
	vec2f relPos = vec2f(float(pos.x)/float(windowSize.y), -float(pos.y)/float(windowSize.y));
	// scale
	relPos *= zoom;
	// translate to camera pos, 20.0f is the board size
	relPos += vec2f(scene->getCamera()->getWorldPos()/20.0f);
	//finally, set (0,0) to the lower left corner
	relPos += vec2f(0.5f);
	return relPos;
}

vec2i Grid::getMouseCellCoords() const {
	vec2f pos = getRelPos();
	float cellsize = 1.0f/GRIDSIZE;
	return  vec2i(glm::floor(pos/cellsize));
}

void Grid::toggleBlock() {
	vec2i c = getMouseCellCoords();
	if(c.x < 0 || c.y < 0 || c.x >= GRIDSIZE || c.y >= GRIDSIZE) return;
	cells[c.x][c.y].block = !cells[c.x][c.y].block;
	updateGridTex();
}

void Grid::calcAngles() {
	resetCells();
	std::queue<vec2i> q;
	std::vector<std::vector<bool>> vis(GRIDSIZE, std::vector<bool>(GRIDSIZE, false));
	q.push(origin);
	vis[origin.x][origin.y] = true;
	updateGridTex();
}

void Grid::updateGridTex() {
	std::vector<char> pixels(GRIDSIZE*GRIDSIZE*4, 0);
	for(int x = 0; x < GRIDSIZE; ++x) {
		for(int y = 0; y < GRIDSIZE; ++y) {
			Cell& c = cells[x][y];
			if(vec2i(x, y) == origin) {
				pixels[x*4+y*GRIDSIZE*4  ] = 100;
				pixels[x*4+y*GRIDSIZE*4+1] = 10;
				pixels[x*4+y*GRIDSIZE*4+2] = 10;
			}
			else if(c.block) {
				pixels[x*4+y*GRIDSIZE*4  ] = 15;
				pixels[x*4+y*GRIDSIZE*4+1] = 15;
				pixels[x*4+y*GRIDSIZE*4+2] = 15;
			}
			else {
				pixels[x*4+y*GRIDSIZE*4  ] = 5;
				pixels[x*4+y*GRIDSIZE*4+1] = 5;
				pixels[x*4+y*GRIDSIZE*4+2] = 5;
			}
			pixels[x*4+y*GRIDSIZE*4+3] = 255;
		}
	}
	gridTex.setData(&pixels[0], TextureFormat::RGBA, TextureFormat::UNSIGNED_BYTE);
}

void Grid::update(float deltaTime) {
	(void) deltaTime;
	transform = glm::scale(mat4f(1.0f), vec3f(10.0f));
	Mouse::setRelativeMode(false);
	if (Mouse::justPressed(Mouse::Left)) {
		toggleBlock();
	}
	for (int x = 0; x < GRIDSIZE; ++x) {
		for (int y = 0; y < GRIDSIZE; ++y) {
			cells[x][y].angle->doDraw = false;
		}
	}

	vec2i c = getMouseCellCoords();
	if(c.x >= 0 &&
			c.y >= 0 &&
			c.x < GRIDSIZE &&
			c.y < GRIDSIZE) {
		cells[c.x][c.y].angle->doDraw = true;
	}
	static bool spin = false;
	if(Keyboard::justPressed(Keyboard::Space)) spin = !spin;
	if(spin) {
		test1->set(glm::rotate(test1->getDir(), .5f*deltaTime), test1->getHalfAngle(), false);
		test2->set(glm::rotate(test2->getDir(), .5f*deltaTime), test2->getHalfAngle(), false);
	}
	if(Keyboard::pressed(Keyboard::R))
		test2->set(test2->getDir(), glm::min(test2->getHalfAngle()+0.001f, float(M_PI*0.999999f)), false);
	if(Keyboard::pressed(Keyboard::F))
		test2->set(test2->getDir(), glm::max(test2->getHalfAngle()-0.001f, 0.01f), false);
	if(Keyboard::pressed(Keyboard::V))
		test2->set(glm::rotate(test2->getDir(), 1.0f*deltaTime), test2->getHalfAngle(), false);
	if(Keyboard::pressed(Keyboard::B))
		test2->set(glm::rotate(test2->getDir(), -1.0f*deltaTime), test2->getHalfAngle(), false);
	if(Keyboard::justPressed(Keyboard::Z))
		test1->doDraw = !test1->doDraw;
	if(Keyboard::justPressed(Keyboard::X))
		test2->doDraw = !test2->doDraw;
	if(Keyboard::justPressed(Keyboard::C))
		test3->doDraw = !test3->doDraw;
	static int state = 0;
	if(Keyboard::justPressed(Keyboard::N)) state = (state+1)%2;
	switch(state) {
		case 0:
			test3->set(Angle::angleUnion(test1, test2));
			break;
		case 1:
			test3->set(Angle::angleIntersection(test1, test2));
			break;
	}
}

void Grid::draw() const {
	const Camera* cam = (Camera*) getGame()->getObjectByName("mainCamera");
	ProgramManager.get("textured").uniform("MVP")->set(cam->projection*cam->getView()*fullTransform);
	ProgramManager.get("textured").uniform("tex")->set(gridTex);
	quad.draw(ProgramManager.get("textured"));
	ProgramManager.get("colored").uniform("u_color")->set(vec4f(0.5f, 0.5f, 0.5f, 1.0f));
	ProgramManager.get("colored").uniform("MVP")->set(cam->projection*cam->getView()*fullTransform);
	lines.draw(ProgramManager.get("colored"));
}
