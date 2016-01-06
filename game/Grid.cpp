#include "Grid.hpp"
#include "Scene.hpp"
#include "Manager.hpp"

#define GRIDSIZE 21

vec2i diff[4] = {
	{ 1,  0},
	{ 0,  1},
	{-1,  0},
	{ 0, -1}
};

Grid::Grid() {
	cells = std::vector<std::vector<Cell>>(GRIDSIZE, std::vector<Grid::Cell>(GRIDSIZE));
	resetCells();
	initGridTex();
	initQuadMesh();
	initLinesMesh();
	updateGridTex();
}

Grid::~Grid() {
}

AngleDef Grid::getAngle(int x, int y, Dir d) const {
	if(vec2i(x, y) == origin)
		return {vec2f(0.0f, 1.0f), 0.0f, true};
	vec2f center = vec2f(x, y)+0.5f+vec2f(diff[d])*0.5f;
	vec2f orig = vec2f(origin)+0.5f;
	float ballRadius = 0.5f;
	float tangent = glm::sqrt(glm::distance2(center,orig) - glm::pow(ballRadius, 2));
	float angle = ballRadius/tangent;
	return {glm::normalize(center-orig), angle, false};
}

void Grid::resetCells() {
	for(int x = 0; x < GRIDSIZE; ++x)
		for(int y = 0; y < GRIDSIZE; ++y) {
			if(cells[x][y].angle == nullptr) {
				cells[x][y].angle = new Angle();
				cells[x][y].angle->addTo(this);
			}
			cells[x][y].angle->set({{0.0f, 1.0f}, 0.0f, false});
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
	calcAngles();
}

void Grid::calcAngles() {
	resetCells();
	std::queue<vec2i> q;
	std::unordered_set<vec2i> inQ;
	std::vector<std::vector<bool>> vis(GRIDSIZE, std::vector<bool>(GRIDSIZE, false));
	q.push(origin);
	inQ.insert(origin);
	cells[origin.x][origin.y].angle->set({{0.0f, 1.0f}, 0.0f, true});
	Dir dirs[4] = {RIGHT, UP, LEFT, DOWN};
	while(!q.empty()) {
		vec2i front = q.front();
		vis[front.x][front.y] = true;
		q.pop();
		for(Dir d : dirs) {
			vec2i n = front + diff[d];
			if(n.x < 0 || n.y < 0 || n.x >= GRIDSIZE || n.y >= GRIDSIZE)
				continue;
			if(cells[n.x][n.y].block)
				continue;
			if(vis[n.x][n.y])
				continue;
			if(inQ.count(n) == 0) {
				inQ.insert(n);
				q.push(n);
			}
			cells[n.x][n.y].angle->set(
					Angle::angleUnion(
						cells[n.x][n.y].angle->getDef(),
						Angle::angleIntersection(
							getAngle(front.x, front.y, d),
							cells[front.x][front.y].angle->getDef()
							)
						)
					);
		}
	}
	updateGridTex();
}

void Grid::updateGridTex() {
	std::vector<char> pixels(GRIDSIZE*GRIDSIZE*4, 0);
	for(int x = 0; x < GRIDSIZE; ++x) {
		for(int y = 0; y < GRIDSIZE; ++y) {
			Cell& c = cells[x][y];
			if(vec2i(x, y) == origin) {
				pixels[x*4+y*GRIDSIZE*4  ] = 100;
				pixels[x*4+y*GRIDSIZE*4+1] = 100;
				pixels[x*4+y*GRIDSIZE*4+2] = 10;
			}
			else if(c.block) {
				pixels[x*4+y*GRIDSIZE*4  ] = 15;
				pixels[x*4+y*GRIDSIZE*4+1] = 15;
				pixels[x*4+y*GRIDSIZE*4+2] = 15;
			}
			else {
				if(c.angle->getHalfAngle() > 0 || c.angle->isFull()) {
					pixels[x*4+y*GRIDSIZE*4  ] = 5;
					pixels[x*4+y*GRIDSIZE*4+1] = 20;
					pixels[x*4+y*GRIDSIZE*4+2] = 5;
				}
				else {
					pixels[x*4+y*GRIDSIZE*4  ] = 20;
					pixels[x*4+y*GRIDSIZE*4+1] = 5;
					pixels[x*4+y*GRIDSIZE*4+2] = 5;
				}
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
