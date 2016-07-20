#include "Grid.hpp"
#include "Scene.hpp"
#include "Manager.hpp"

#define GRIDSIZE 33
#define BOARD_SCALE 10.0f
#define EPSILON 0.000001f
#define BOARD_POSITION_X 21.0f

vec3i diff[4] = {
    { 1,  0, 0},
    { 0,  1, 0},
    {-1,  0, 0},
    { 0, -1, 0}
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

int manhattanDist(vec2i a, vec2i b) {
    return glm::abs(a.x-b.x) + glm::abs(a.y-b.y);
}

// This is a standard Fisher-Yates random in-place shuffle
template<typename T>
void fy_shuffle(std::vector<T>& v) {
    for(int i = v.size()-1; i > 0; --i) {
        int j = rand()%i;
        std::swap(v[i], v[j]);
    }
}

bool equals(const vec3f& a, const vec3f& b) {
    return glm::epsilonEqual(a, b, EPSILON) == vec3b(true);
}

AngleDef getCone(const vec3f& p1, const vec3f& p2) {
    // p1 and p2 are assumed to be unit vectors
    vec3f dir = glm::normalize(p1+p2);
    float dist = glm::dot(p1, dir);
    float tan = glm::distance(p1, dir*dist);
    return {dir, tan/dist, false};
}

AngleDef getCone(const vec3f& p1, const vec3f& p2, const vec3f& p3) {
    // The idea is to compute the two planes that run in between p1,p2 and p2,p3.
    // The direction of the cone will be the intersection of those planes (which
    // happens to be a line) and the radius can be then computed using
    // any of the three original vectors.
    vec3f pn1 = glm::normalize(
            glm::cross(
                    glm::normalize(p1+p2),
                    glm::cross(p1, p2)
                )
            );
    vec3f pn2 = glm::normalize(
            glm::cross(
                    glm::normalize(p2+p3),
                    glm::cross(p2, p3)
                )
            );
    // Cross product of the two plane's normals will give us the new direction
    vec3f dir = glm::normalize(glm::cross(pn1, pn2));
    // Flip the direction in case we got it the wrong way.
    if(glm::dot(dir, p1) <= 0.0f)
        dir = -dir;
    // Compute the new cone angle
    float dist = glm::dot(p1, dir);
    float tan = glm::distance(p1, dir*dist);
    return {dir, tan/dist, false};
}

bool insideCone(const AngleDef& c, const vec3f& v) {
    float dist = glm::dot(v, c.dir);
    float tan = glm::distance(v, c.dir*dist);
    return (dist > 0.0f && tan/dist <= (c.halfAngle+EPSILON));
}

// This is the cheap approximation for the bounding cone problem.
// Has a bad relative error rate.
// All vectors in p assumed to be unit vectors
AngleDef getSmallestConeApprox(const std::vector<vec3f>& p) {
    vec3f dir;
    for(const vec3f& v : p)
        dir += v;
    dir = glm::normalize(dir);
    float tan = 0.0f;
    for(const vec3f& v : p) {
        float dist = glm::dot(v, dir);
        tan = glm::max(tan, glm::distance(v, dir*dist)/dist);
    }
    return {dir, tan, false};
}

// This is the general implementation for the algorithm found at
// http://www.cs.technion.ac.il/~cggc/files/gallery-pdfs/Barequet-1.pdf
// which is an application of the minimum enclosing circle problem to
// the bounding cone problem. This implementation is just for reference,
// the actual function to be used is minConeUnroll, the unrolled
// version of this. All vectors in "points" are assumed to be unit vectors
AngleDef minConeTwoPoint(const std::vector<vec3f>& points, unsigned int last, const vec3f& q1, const vec3f& q2) {
    AngleDef c = getCone(q1, q2);
    for(unsigned int i = 0; i < last; ++i)
        if(!insideCone(c, points[i]))
            c = getCone(q1, q2, points[i]);
    return c;
}
AngleDef minConeOnePoint(const std::vector<vec3f>& points, unsigned int last, const vec3f& q1) {
    AngleDef c = getCone(q1, points[0]);
    for(unsigned int i = 1; i < last; ++i)
        if(!insideCone(c, points[i]))
            c = minConeTwoPoint(points, i, q1, points[i]);
    return c;
}
AngleDef minCone(const std::vector<vec3f>& points) {
    AngleDef c = getCone(points[0], points[1]);
    for(unsigned int i = 2; i < points.size(); ++i)
        if(!insideCone(c, points[i]))
            c = minConeOnePoint(points, i, points[i]);
    return c;
}

// Unrolled version of the aforementioned algorithm.
// I left the recursive calls commented wherever they would
// be called for the sake of clarity/readability.
// This only works with four points, not for the generic case.
AngleDef minConeUnroll(const vec3f& v0, const vec3f& v1, const vec3f& v2, const vec3f& v3) {
    // c = minCone(p);
    AngleDef c = getCone(v0, v1);
    if(!insideCone(c, v2)) {
        //c = minConeOnePointUnroll(p, 2, v2);
        c = getCone(v2, v0);
        if(!insideCone(c, v1)) {
            //c = minConeTwoPoint(p, 1, v2, v1);
            c = getCone(v2, v1);
            if(!insideCone(c, v0))
                c = getCone(v2, v1, v0);
        }
    }
    if(!insideCone(c, v3)) {
        //c = minConeOnePointUnroll(p, 3, v3);
        c = getCone(v3, v0);
        if(!insideCone(c, v1)) {
            //c = minConeTwoPoint(p, 1, v3, v1);
            c = getCone(v3, v1);
            if(!insideCone(c, v0))
                c = getCone(v3, v1, v0);
        }
        if(!insideCone(c, v2)) {
            //c = minConeTwoPoint(p, 2, v3, v2);
            c = getCone(v3, v2);
            if(!insideCone(c, v0))
                c = getCone(v3, v2, v0);
            if(!insideCone(c, v1))
                c = getCone(v3, v2, v1);
        }
    }
    return c;
}

AngleDef getSmallestCone(const std::vector<vec3f>& p, bool approxMode) {
    VBE_ASSERT(p.size() == 4, "getSmallestCone expects 4 points");
    for(auto v : p) {
        VBE_ASSERT(equals(glm::normalize(v), v), "getSmallestCone expects unit vectors");
    }
    if(approxMode)
        return getSmallestConeApprox(p);
    return minConeUnroll(p[0], p[1], p[2], p[3]);
}

// If genMode2D is true, this will calculate the new cones using only
// two points per face instead of 4, hence simulating a 2D grid case
AngleDef Grid::getAngle(int x, int y, Dir d) const {
    if(vec2i(x, y) == origin)
        return {{0.0f, 1.0f, 0.0f}, 0.0f, true};
    vec3f center = vec3f(x, y, 0.0f)+vec3f(0.5f, 0.5f, 0.0f)+vec3f(diff[d])*0.5f;
    vec3f orig = vec3f(vec3i(origin, 0))+vec3f(0.5f, 0.5f, 0.0f);
    if(!genMode2D) {
        std::vector<vec3f> p(4);
        switch(d) {
            case UP:
            case DOWN:
                p[0] = center+vec3f( 0.5f, 0.0f, 0.5f)-orig;
                p[1] = center+vec3f(-0.5f, 0.0f, 0.5f)-orig;
                p[2] = center+vec3f( 0.5f, 0.0f,-0.5f)-orig;
                p[3] = center+vec3f(-0.5f, 0.0f,-0.5f)-orig;
                break;
            case LEFT:
            case RIGHT:
                p[0] = center+vec3f( 0.0f, 0.5f, 0.5f)-orig;
                p[1] = center+vec3f( 0.0f,-0.5f, 0.5f)-orig;
                p[2] = center+vec3f( 0.0f, 0.5f,-0.5f)-orig;
                p[3] = center+vec3f( 0.0f,-0.5f,-0.5f)-orig;
                break;
        }
        for(vec3f& v : p) v = glm::normalize(v);
        fy_shuffle(p);
        return getSmallestCone(p, approxMode);
    }
    vec2f p1, p2;
    switch(d) {
        case UP:
        case DOWN:
            p1 = vec2f(center)+vec2f( 0.5f, 0.0f)-vec2f(orig);
            p2 = vec2f(center)+vec2f(-0.5f, 0.0f)-vec2f(orig);
            break;
        case LEFT:
        case RIGHT:
            p1 = vec2f(center)+vec2f( 0.0f,  0.5f)-vec2f(orig);
            p2 = vec2f(center)+vec2f( 0.0f, -0.5f)-vec2f(orig);
            break;
    }
    return getCone(glm::normalize(vec3f(p1, 0.0f)), glm::normalize(vec3f(p2, 0.0f)));
}

void Grid::resetCells() {
    for(int x = 0; x < GRIDSIZE; ++x)
        for(int y = 0; y < GRIDSIZE; ++y) {
            if(cells[x][y].angle != nullptr)
                cells[x][y].angle->removeAndDelete();
            cells[x][y].angle = new Angle();
            cells[x][y].angle->addTo(this);
            vec2f o = (vec2f(origin) + 0.5f)/float(GRIDSIZE);
            o = o*2.0f - 1.0f;
            cells[x][y].angle->center = vec3f(o, 0.0f);
            cells[x][y].angle->set({{0.0f, 1.0f, 0.0f}, 0.0f, false});
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
    for(float x = -1; x <= 1; x += 2.0f/GRIDSIZE) {
        lineData.push_back(vec3f(x,-1, 0));
        lineData.push_back(vec3f(x, 1, 0));
        lineData.push_back(vec3f(-1, x, 0));
        lineData.push_back(vec3f( 1, x, 0));
    }
    lines = Mesh(Vertex::Format(elems));
    lines.setVertexData(&lineData[0], lineData.size());
    lines.setPrimitiveType(Mesh::LINES);
}

// Get  grid coords for the mouse: (0, 0) is lower left, (1,1) is upper right
vec2f Grid::getRelPos() const {
    Scene* scene = (Scene*)getGame()->getObjectByName("SCENE");
    // mouse coords: (0,0) as the upper left corner, (sWidth, sHeight) as lower right
    vec2i pos = Mouse::position();
    // wSize in pixels
    vec2ui windowSize = Window::getInstance()->getSize();
    //zoom scaled to match the board scale
    float zoom = scene->getZoom()/BOARD_SCALE;
    // (set 0,0 to the center of the screen (board center initially)
    pos -= vec2i(windowSize/(uint)2);
    // top is .5, bottom is .5, y's need to be inverted since mouse coords are in -y
    vec2f relPos = vec2f(float(pos.x)/float(windowSize.y), -float(pos.y)/float(windowSize.y));
    // scale
    relPos *= zoom;
    // translate to camera pos, 2.0f*BOARD_SCALE is the board world size, since original mesh is 2x2
    relPos += (vec2f(scene->getCamera()->getWorldPos())-vec2f(BOARD_POSITION_X, 0.0f))/(2.0f*BOARD_SCALE);
    // set (0,0) to the lower left corner
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

// Main algorithm!
void Grid::calcAngles() {
    resetCells();
    std::queue<vec2i> q;
    std::vector<std::vector<bool>> vis(GRIDSIZE, std::vector<bool>(GRIDSIZE, false));
    q.push(origin);
    cells[origin.x][origin.y].angle->set({{1.0f, 1.0f, 0.0f}, 0.0f, true});
    Dir dirs[4] = {RIGHT, UP, LEFT, DOWN};
    while(!q.empty()) {
        vec2i front = q.front();
        q.pop();
        // visited
        if(vis[front.x][front.y]) continue;
        vis[front.x][front.y] = true;
        for(Dir d : dirs) {
            vec2i n = front + vec2i(diff[d]);
            // Out of bountaries
            if(n.x < 0 || n.y < 0 || n.x >= GRIDSIZE || n.y >= GRIDSIZE) continue;
            // Straight line will have the smallest possible manhattan distance to the origin
            if(manhattanDist(origin, n) < manhattanDist(origin, front)) continue;
            // This is a blocker
            if(cells[n.x][n.y].block) continue;
            // Push it
            q.push(n);
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
                // Origin painted Yellow
                pixels[x*4+y*GRIDSIZE*4  ] = 100;
                pixels[x*4+y*GRIDSIZE*4+1] = 100;
                pixels[x*4+y*GRIDSIZE*4+2] = 10;
            }
            else if(c.block) {
                // Blockers painted gray
                pixels[x*4+y*GRIDSIZE*4  ] = 15;
                pixels[x*4+y*GRIDSIZE*4+1] = 15;
                pixels[x*4+y*GRIDSIZE*4+2] = 15;
            }
            else if(c.angle->getHalfAngle() > 0 || c.angle->isFull()) {
                // Visible painted green
                pixels[x*4+y*GRIDSIZE*4  ] = 5;
                pixels[x*4+y*GRIDSIZE*4+1] = 20;
                pixels[x*4+y*GRIDSIZE*4+2] = 5;
            }
            else {
                // Non-visible painted red
                pixels[x*4+y*GRIDSIZE*4  ] = 20;
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
    transform = glm::translate(mat4f(1.0f), vec3f(BOARD_POSITION_X, 0.0f, 0.0f));
    transform = glm::scale(transform, vec3f(BOARD_SCALE));
    Mouse::setRelativeMode(false);
    if (Mouse::justPressed(Mouse::Left))
        toggleBlock();
    for (int x = 0; x < GRIDSIZE; ++x)
        for (int y = 0; y < GRIDSIZE; ++y)
            cells[x][y].angle->doDraw = false;

    vec2i c = getMouseCellCoords();
    if(c.x >= 0 &&
       c.y >= 0 &&
       c.x < GRIDSIZE &&
       c.y < GRIDSIZE) {
        cells[c.x][c.y].angle->doDraw = true;
    }
    if(Keyboard::justPressed(Keyboard::Space)) {
        genMode2D = !genMode2D;
        Log::message() << "Setting mode to " << (genMode2D? "2D" : "3D") << Log::Flush;
        calcAngles();
    }
    if(Keyboard::justPressed(Keyboard::Z)) {
        approxMode = !approxMode;
        Log::message() << "Setting mode to " << (approxMode? "approximated" : "exact") << Log::Flush;
        calcAngles();
    }
}

void Grid::draw() const {
    const Camera* cam = (Camera*) getGame()->getObjectByName("mainCamera");
    ProgramManager.get("textured").uniform("MVP")->set(cam->projection*cam->getView()*fullTransform);
    ProgramManager.get("textured").uniform("tex")->set(gridTex);
    quad.draw(ProgramManager.get("textured"));
    ProgramManager.get("colored").uniform("u_color")->set(vec4f(0.2f, 0.2f, 0.2f, 1.0f));
    ProgramManager.get("colored").uniform("MVP")->set(cam->projection*cam->getView()*fullTransform);
    lines.draw(ProgramManager.get("colored"));
}
