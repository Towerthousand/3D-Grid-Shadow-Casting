#include "GridOrtho.hpp"
#include "Scene.hpp"
#include "Manager.hpp"

#define GRIDSIZE 33
#define BOARD_SCALE 10.0f
#define EPSILON 0.000001f

namespace {
    vec3i offsets[6] = {
        {-1, 0, 0},
        { 1, 0, 0},
        { 0,-1, 0},
        { 0, 1, 0},
        { 0, 0,-1},
        { 0, 0, 1}
    };
}

struct Square {
    vec2f p;
    vec2f d;
};


//pair(float, vector) comp for pqueue
struct fvpaircomp {
    bool operator() (const std::pair<float, vec3i>& lhs, const std::pair<float, vec3i>& rhs) const {
        return lhs.first>rhs.first;
    };
};

Square squareUnion(const Square& s1, const Square& s2) {
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

Square squareIntersection(const Square& s1, const Square& s2) {
    if(s1.d == vec2f(0.0f) ||
       s2.d == vec2f(0.0f) ||
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

mat3f getViewMatrixForDirection(const vec3f& direction) {
    vec3f dummyUp = (glm::abs(glm::normalize(direction)) == vec3f(0, 1, 0))? vec3f(0,0,1) : vec3f(0, 1, 0);
    vec3f front = glm::normalize(-direction);
    vec3f right = glm::normalize(glm::cross(dummyUp, front));
    vec3f up = glm::normalize(glm::cross(front, right));
    return glm::transpose(
        mat3f(
            right.x, right.y, right.z,
            up.x   , up.y   , up.z   ,
            front.x, front.y, front.z
        )
    );
};

// If genMode2D is true, this will calculate the new cones using only
// two points per face instead of 4, hence simulating a 2D grid case
Square getSquare(vec3i pos, GridOrtho::Face f, const mat3f& viewMatrix) {
    vec3f center = vec3f(pos)+0.5f+vec3f(offsets[f])*0.5f;
    std::vector<vec3f> p(4);
    switch(f) {
        case GridOrtho::MINX:
        case GridOrtho::MAXX:
            p[0] = center+vec3f( 0.0f, 0.5f, 0.5f);
            p[1] = center+vec3f( 0.0f,-0.5f, 0.5f);
            p[2] = center+vec3f( 0.0f, 0.5f,-0.5f);
            p[3] = center+vec3f( 0.0f,-0.5f,-0.5f);
            break;
        case GridOrtho::MINY:
        case GridOrtho::MAXY:
            p[0] = center+vec3f( 0.5f, 0.0f, 0.5f);
            p[1] = center+vec3f(-0.5f, 0.0f, 0.5f);
            p[2] = center+vec3f( 0.5f, 0.0f,-0.5f);
            p[3] = center+vec3f(-0.5f, 0.0f,-0.5f);
            break;
        case GridOrtho::MINZ:
        case GridOrtho::MAXZ:
            p[0] = center+vec3f( 0.5f, 0.5f, 0.0f);
            p[1] = center+vec3f(-0.5f, 0.5f, 0.0f);
            p[2] = center+vec3f( 0.5f,-0.5f, 0.0f);
            p[3] = center+vec3f(-0.5f,-0.5f, 0.0f);
            break;
    }
    vec2f min = vec2f(std::numeric_limits<float>::max());
    vec2f max = vec2f(std::numeric_limits<float>::lowest());
    for(vec3f& v : p) {
        v = viewMatrix*v;
        min.x = glm::min(min.x, v.x);
        min.y = glm::min(min.y, v.y);
        max.x = glm::max(max.x, v.x);
        max.y = glm::max(max.y, v.y);
    }
    return {min, max-min};
}

Square squares[GRIDSIZE][GRIDSIZE];
bool vis[GRIDSIZE][GRIDSIZE];

// Main algorithm!
void GridOrtho::calcSquares() {
    mat3f viewMatrix = getViewMatrixForDirection(sunDir);
    memset(squares, 0, sizeof(squares));
    memset(vis, 0, sizeof(vis));
    std::priority_queue<std::pair<float, vec3i>, std::vector<std::pair<float, vec3i>>, fvpaircomp> q;
    std::bitset<6> faces = 0;
    if(sunDir.x != 0.0f) faces.set(sunDir.x < 0.0f? MAXX : MINX);
    if(sunDir.y != 0.0f) faces.set(sunDir.y < 0.0f? MAXY : MINY);
    if(sunDir.z != 0.0f) faces.set(sunDir.z < 0.0f? MAXZ : MINZ);
    for(int i = 0; i < 6; ++i) {
        if(!faces.test(i)) continue;
        vec3i min = {
            (i == MAXX) ? GRIDSIZE-1 : 0,
            (i == MAXY) ? GRIDSIZE-1 : 0,
            (i == MAXZ) ? 0 : 0
        };
        vec3i max = {
            (i == MINX) ? 0 : GRIDSIZE-1,
            (i == MINY) ? 0 : GRIDSIZE-1,
            (i == MINZ) ? 0 : 0
        };
        for(int x = min.x; x <= max.x; ++x)
            for(int y = min.y; y <= max.y; ++y)
                for(int z = min.z; z <= max.z; ++z) {
                    float len = glm::dot(vec2f(x, y), vec2f(sunDir));
                    q.push(std::make_pair(len, vec3i(x, y, z)));
                    if(blockers[x][y]) continue;
                    squares[x][y] = squareUnion(squares[x][y], getSquare(vec3i(x, y, z), (Face)i, viewMatrix));
                }
    }
    while(!q.empty()) {
        std::pair<float, vec3i> frontP = q.top();
        q.pop();
        vec3i front = frontP.second;
        if(vis[front.x][front.y]) continue;
        vis[front.x][front.y] = true;
        if(squares[front.x][front.y].d == vec2f(0.0f)) continue;
        for(int i = 0; i < 6; ++i) {
            vec3i n = front + offsets[i];
            // Out of bountaries
            if(n.x < 0 || n.y < 0 || n.x >= GRIDSIZE || n.y >= GRIDSIZE) continue;
            // This is a blocker
            if(blockers[n.x][n.y]) continue;
            // Already visited
            if(vis[n.x][n.y]) continue;
            // Update square
            squares[n.x][n.y] = squareUnion(
                squares[n.x][n.y],
                squareIntersection(
                    getSquare(front, (Face)i, viewMatrix),
                    squares[front.x][front.y]
                )
            );
            // Push it
            float len = glm::dot(vec2f(n), vec2f(sunDir));
            q.push(std::make_pair(len, n));
        }
    }
    updateGridTex();
}

GridOrtho::GridOrtho() {
    blockers = std::vector<std::vector<bool>>(GRIDSIZE, std::vector<bool>(GRIDSIZE, false));
    initGridTex();
    initQuadMesh();
    initLinesMesh();
    updateGridTex();
}

GridOrtho::~GridOrtho() {
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
    blockers[c.x][c.y] = !blockers[c.x][c.y];
    calcSquares();
}

void GridOrtho::updateGridTex() {
    std::vector<char> pixels(GRIDSIZE*GRIDSIZE*4, 0);
    for(int x = 0; x < GRIDSIZE; ++x) {
        for(int y = 0; y < GRIDSIZE; ++y) {
            if(blockers[x][y]) {
                // Blockers painted gray
                pixels[x*4+y*GRIDSIZE*4  ] = 15;
                pixels[x*4+y*GRIDSIZE*4+1] = 15;
                pixels[x*4+y*GRIDSIZE*4+2] = 15;
            }
            else if(squares[x][y].d.x > 0.0f || squares[x][y].d.y > 0.0f) {
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
