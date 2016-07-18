#include "GridOrtho.hpp"
#include "Scene.hpp"
#include "Manager.hpp"

#define GRIDSIZE 33
#define BOARD_SCALE 10.0f
#define EPSILON 0.00001f

vec3i diff2[4] = {
    { 1,  0, 0},
    { 0,  1, 0},
    {-1,  0, 0},
    { 0, -1, 0}
};

//pair(float, vector) comp for pqueue
struct fvpaircomp {
    bool operator() (const std::pair<float, vec2i>& lhs, const std::pair<float, vec2i>& rhs) const {
        return lhs.first>rhs.first;
    };
};

const Log&operator <<(const Log& log, const Square& s) {
    log << "Square[Point: " << s.p << ", Dimensions: " << s.d << "]";
    return log;
}

Square Square::squareUnion(const Square& s1, const Square& s2) {
    if(s1.p == vec2f(0.0f) && s1.d == vec2f(0.0f)) return s2;
    else if(s2.p == vec2f(0.0f) && s2.d == vec2f(0.0f)) return s1;
    Square s = {
        {
            glm::min(s1.p.x, s2.p.x),
            glm::min(s1.p.y, s2.p.y),
        },
        {
            glm::max(s1.p.x + s1.d.x, s2.p.x + s2.d.x),
            glm::max(s1.p.y + s1.d.y, s2.p.y + s2.d.y),
        }
    };
    s.d -= s.p;
    if(glm::epsilonEqual(s.d.x*s.d.y, 0.0f, EPSILON)) return {{0.0f, 0.0f}, {0.0f, 0.0f}};
    return s;
};

Square Square::squareIntersection(const Square& s1, const Square& s2) {
    if((s1.p == vec2f(0.0f) && s1.d == vec2f(0.0f)) ||
       (s2.p == vec2f(0.0f) && s2.d == vec2f(0.0f)) ||
       s1.p.x+s1.d.x <= s2.p.x ||
       s1.p.x >= s2.p.x+s2.d.x ||
       s1.p.y+s1.d.y <= s2.p.y ||
       s1.p.y >= s2.p.y+s2.d.y)
        return {{0.0f, 0.0f}, {0.0f, 0.0f}};
    Square s = {
        {
            glm::max(s1.p.x, s2.p.x),
            glm::max(s1.p.y, s2.p.y),
        },
        {
            glm::min(s1.p.x + s1.d.x, s2.p.x + s2.d.x),
            glm::min(s1.p.y + s1.d.y, s2.p.y + s2.d.y),
        }
    };
    s.d -= s.p;
    if(glm::epsilonEqual(s.d.x*s.d.y, 0.0f, EPSILON)) return {{0.0f, 0.0f}, {0.0f, 0.0f}};
    VBE_ASSERT(s.d.x >= 0.0f && s.d.y >= 0.0f, "Sanity check for squareIntersection");
    return s;
};

GridOrtho::GridOrtho() {
    cells = std::vector<std::vector<Cell>>(GRIDSIZE, std::vector<GridOrtho::Cell>(GRIDSIZE));
    resetCells();
    initGridTex();
    initQuadMesh();
    initLinesMesh();
    updateGridTex();
}

GridOrtho::~GridOrtho() {
}

mat4f getViewMatrixForDirection(const vec3f& direction) {
    vec3f dummyUp = (glm::abs(glm::normalize(direction)) == vec3f(0, 1, 0))? vec3f(0,0,1) : vec3f(0, 1, 0);
    vec3f front = glm::normalize(-direction);
    vec3f right = glm::normalize(glm::cross(dummyUp, front));
    vec3f up = glm::normalize(glm::cross(front, right));
    return glm::transpose(
        mat4f(
            right.x, right.y, right.z, 0,
            up.x   , up.y   , up.z   , 0,
            front.x, front.y, front.z, 0,
            0      , 0      , 0      , 1
        )
    );
};

// If genMode2D is true, this will calculate the new cones using only
// two points per face instead of 4, hence simulating a 2D grid case
Square GridOrtho::getSquare(int x, int y, Dir d) const {
    vec3f center = vec3f(x, y, 0.0f)+vec3f(0.5f, 0.5f, 0.0f)+vec3f(diff2[d])*0.5f;
    std::vector<vec3f> p(4);
    switch(d) {
        case UP:
        case DOWN:
            p[0] = center+vec3f( 0.5f, 0.0f, 0.5f);
            p[1] = center+vec3f(-0.5f, 0.0f, 0.5f);
            p[2] = center+vec3f( 0.5f, 0.0f,-0.5f);
            p[3] = center+vec3f(-0.5f, 0.0f,-0.5f);
            break;
        case LEFT:
        case RIGHT:
            p[0] = center+vec3f( 0.0f, 0.5f, 0.5f);
            p[1] = center+vec3f( 0.0f,-0.5f, 0.5f);
            p[2] = center+vec3f( 0.0f, 0.5f,-0.5f);
            p[3] = center+vec3f( 0.0f,-0.5f,-0.5f);
            break;
    }
    mat4f viewMatrix = getViewMatrixForDirection(sunDir);
    vec2f min = vec2f(std::numeric_limits<float>::max());
    vec2f max = vec2f(std::numeric_limits<float>::lowest());
    for(vec3f& v : p) {
        v = vec3f(viewMatrix*vec4f(v, 1.0f));
        min.x = glm::min(min.x, v.x);
        min.y = glm::min(min.y, v.y);
        max.x = glm::max(max.x, v.x);
        max.y = glm::max(max.y, v.y);
    }
    return {min, max-min};
}

void GridOrtho::resetCells() {
    for(int x = 0; x < GRIDSIZE; ++x)
        for(int y = 0; y < GRIDSIZE; ++y)
            cells[x][y].sq = {{0.0f, 0.0f}, {0.0f, 0.0f}};
}

void GridOrtho::initGridTex() {
    gridTex = Texture2D(vec2ui(GRIDSIZE), TextureFormat::RGBA8);
    gridTex.setFilter(GL_NEAREST, GL_NEAREST);
}

void GridOrtho::initQuadMesh() {
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

void GridOrtho::initLinesMesh() {
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
vec2f GridOrtho::getRelPos() const {
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
    relPos += vec2f(scene->getCamera()->getWorldPos()/(2.0f*BOARD_SCALE));
    // set (0,0) to the lower left corner
    relPos += vec2f(0.5f);
    return relPos;
}

vec2i GridOrtho::getMouseCellCoords() const {
    vec2f pos = getRelPos();
    float cellsize = 1.0f/GRIDSIZE;
    return  vec2i(glm::floor(pos/cellsize));
}

void GridOrtho::toggleBlock() {
    vec2i c = getMouseCellCoords();
    if(c.x < 0 || c.y < 0 || c.x >= GRIDSIZE || c.y >= GRIDSIZE) return;
    cells[c.x][c.y].block = !cells[c.x][c.y].block;
    calcSquares();
}

// Main algorithm!
void GridOrtho::calcSquares() {
    resetCells();
    std::priority_queue<std::pair<float, vec2i>, std::vector<std::pair<float, vec2i>>, fvpaircomp> q;
    std::unordered_set<vec2i> inQ;
    std::vector<std::vector<bool>> vis(GRIDSIZE, std::vector<bool>(GRIDSIZE, false));
    for(int y = 0; y < GRIDSIZE; ++y) {
        inQ.insert(vec2i(GRIDSIZE-1, y));
        float len = glm::dot(vec2f(GRIDSIZE-1, y), vec2f(sunDir));
        q.push(std::make_pair(len, vec2i(GRIDSIZE-1, y)));
        cells[GRIDSIZE-1][y].sq = getSquare(GRIDSIZE-1, y, RIGHT);
    }
    for(int x = 0; x < GRIDSIZE; ++x) {
        inQ.insert(vec2i(x, 0));
        float len = glm::dot(vec2f(x, 0), vec2f(sunDir));
        q.push(std::make_pair(len, vec2i(x, 0)));
        cells[x][0].sq = getSquare(x, 0, RIGHT);
    }
    Dir dirs[4] = {RIGHT, UP, LEFT, DOWN};
    while(!q.empty()) {
        std::pair<float, vec2i> frontP = q.top();
        vec2i front = frontP.second;
        vis[front.x][front.y] = true;
        q.pop();
        for(Dir d : dirs) {
            vec2i n = front + vec2i(diff2[d]);
            // Out of bountaries
            if(n.x < 0 || n.y < 0 || n.x >= GRIDSIZE || n.y >= GRIDSIZE)
                continue;
            // This is a blocker
            if(cells[n.x][n.y].block)
                continue;
            // Already visited
            if(vis[n.x][n.y])
                continue;
            // Have we pushed this to the queue yet?
            if(inQ.count(n) == 0) {
                inQ.insert(n);
                float len = glm::dot(vec2f(n), vec2f(sunDir));
                q.push(std::make_pair(len, n));
            }
            Square frontSquare = getSquare(front.x, front.y, d);
            Square intersection =  Square::squareIntersection(
                frontSquare,
                cells[front.x][front.y].sq
            );
            cells[n.x][n.y].sq = Square::squareUnion(
                cells[n.x][n.y].sq,
                intersection
            );
        }
    }
    updateGridTex();
}

void GridOrtho::updateGridTex() {
    std::vector<char> pixels(GRIDSIZE*GRIDSIZE*4, 0);
    for(int x = 0; x < GRIDSIZE; ++x) {
        for(int y = 0; y < GRIDSIZE; ++y) {
            Cell& c = cells[x][y];
            if(c.block) {
                // Blockers painted gray
                pixels[x*4+y*GRIDSIZE*4  ] = 15;
                pixels[x*4+y*GRIDSIZE*4+1] = 15;
                pixels[x*4+y*GRIDSIZE*4+2] = 15;
            }
            else if(c.sq.d.x > 0.0f || c.sq.d.y > 0.0f) {
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

void GridOrtho::update(float deltaTime) {
    (void) deltaTime;
    transform = glm::scale(mat4f(1.0f), vec3f(BOARD_SCALE));
    Mouse::setRelativeMode(false);
    if (Mouse::justPressed(Mouse::Left))
        toggleBlock();
}

void GridOrtho::draw() const {
    const Camera* cam = (Camera*) getGame()->getObjectByName("mainCamera");
    ProgramManager.get("textured").uniform("MVP")->set(cam->projection*cam->getView()*fullTransform);
    ProgramManager.get("textured").uniform("tex")->set(gridTex);
    quad.draw(ProgramManager.get("textured"));
    ProgramManager.get("colored").uniform("u_color")->set(vec4f(0.2f, 0.2f, 0.2f, 1.0f));
    ProgramManager.get("colored").uniform("MVP")->set(cam->projection*cam->getView()*fullTransform);
    lines.draw(ProgramManager.get("colored"));
}
