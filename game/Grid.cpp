#include "Grid.hpp"
#include "Scene.hpp"
#include "Manager.hpp"
#define GRIDSIZE 33
#define BOARD_SCALE 10.0f
#define BOARD_POSITION_X 21.0f

int manhattanDist(vec3i a, vec3i b) {
    return glm::abs(a.x-b.x) + glm::abs(a.y-b.y) + glm::abs(a.z-b.z);
}

#define EPSILON 0.000001f

enum AngleOverlap {
    INVALID = -1,
    NONE = 0,
    PARTIAL,
    CONTAINS
};

struct Angle {
    vec3f dir;
    float halfAngle; //stored as tan(half angle)
    bool full;
};

vec3i offsets[6] = {
    {-1, 0, 0},
    { 1, 0, 0},
    { 0,-1, 0},
    { 0, 1, 0},
    { 0, 0,-1},
    { 0, 0, 1}
};

AngleOverlap overlapTest(const Angle& a, const Angle& b) {
    if(a.full) return CONTAINS;
    if(b.full) return NONE;
    vec3f dir1 = b.dir;
    vec3f dir2 = a.dir;
    // if dir1 == dir2 then just check the halfangles
    if(glm::epsilonEqual(dir1, dir2, EPSILON) == vec3b(true)) {
        if(a.halfAngle >= b.halfAngle)
            return CONTAINS;
        return NONE;
    }
    // side is the vector that orthogonally points away from dir1. With a
    // magnitude of halfangle tangent
    vec3f side = glm::cross(glm::normalize(glm::cross(dir1, dir2)), dir1)*b.halfAngle;
    // p1 and p2 are the "extremes" of b's angle
    vec3f p1 = dir1-side;
    vec3f p2 = dir1+side;
    // l1 and l2 are the length of the vector that results from
    // projecting p1 and p2 onto dir2
    float l1 = glm::dot(p1, dir2);
    float l2 = glm::dot(p2, dir2);
    // r1 and r2 are the rejection vectors of said projection
    float r1 = glm::length(p1-dir2*l1);
    float r2 = glm::length(p2-dir2*l2);
    // Return value explanation
    // NONE: this angle doesn't contain any half of the other,
    // so it can both mean it's fully inside it or that they don't
    // overlap at all
    // PARTIAL: this angle contains one of the ends of the other
    // or is right next to the other by a negligible distance
    // CONTAINS: this angle fully contains the other
    int result = 0;
    if(l1 > 0.0f && r1/l1 < a.halfAngle+EPSILON) result++;
    if(l2 > 0.0f && r2/l2 < a.halfAngle+EPSILON) result++;
    return static_cast<AngleOverlap>(result);
};

Angle angleUnion(const Angle& a, const Angle& b) {
    // if any of both are full, union will be full
    if(a.full || b.full)
        return {{0.0f, 0.0f, 0.0f}, 0.0f, true};
    // if one of them is null, return the other
    if(a.halfAngle == 0.0f)
        return b;
    if(b.halfAngle == 0.0f)
        return a;
    // if a contains b, result is a
    AngleOverlap acb = INVALID;
    if(a.halfAngle >= b.halfAngle) {
        acb = overlapTest(a, b);
        if(acb == CONTAINS)
            return a;
    }
    // and viceversa
    AngleOverlap bca = INVALID;
    if(b.halfAngle >= a.halfAngle) {
        bca = overlapTest(b, a);
        if(bca == CONTAINS)
            return b;
    }
    // General case. We compute the ends of the intersection
    // by rotating each cone direction away from the other direction
    // by their own half angle. This is done avoiding trigonometry,
    vec3f dir1 = a.dir;
    vec3f dir2 = b.dir;
    // if dir1 == dir2 then just check the halfangles
    if(glm::epsilonEqual(dir1, dir2, EPSILON) == vec3b(true)) {
        if(a.halfAngle > b.halfAngle)
            return a;
        return b;
    }
    // cross will point opposite directions depending on whether
    // dir1 is on the clockwise side of dir2 or the other way around,
    // in the context of the plane given by (0,0,0), dir1 and dir2,
    // where dir1 and dir2 are coplanar. cross will point orthogonally away
    // from that plane one side or the other
    vec3f cross = glm::normalize(glm::cross(dir1, dir2));
    // dir3 is obtained by rotating dir1 away from dir2 by a's halfangle.
    // the cross product of (cross, dir1) tells us which direction is
    // towards dir2.
    vec3f dir3 = glm::normalize(-glm::cross(cross, dir1)*a.halfAngle+dir1);
    // dir4 is obtained by rotating dir2 away from dir1 by b's halfangle.
    // the cross product of (-cross, dir2) tells us which direction is
    // towards dir1. We negate cross because -cross(dir1, dir2) is the same
    // as cross(dir2, dir1)
    vec3f dir4 = glm::normalize(-glm::cross(-cross, dir2)*b.halfAngle+dir2);
    // d is the direction of the union angle
    vec3f d = glm::normalize(dir3 + dir4);
    // if dot(dir1+dir2, d) < 0.0f, the union angle is > 180 deg
    if(glm::dot(dir1+dir2, d) <= 0.0f) return {{0.0f, 0.0f, 0.0f}, 0.0f, true};
    // we compute the tangent of the new halfangle by scaling one of the
    // outer vectors by the inverse of it's projection onto the new
    // angle's direction, and computing the length of the vector that
    // results from going from the central direction to this scaled outer direction
    return {d, glm::length(d-(dir3/glm::dot(dir3,d))), false};
}

Angle angleIntersection(const Angle& a, const Angle& b) {
    // if b is full, intersection will be a
    if(b.full)
        return {a.dir, a.halfAngle, a.full};
    // if a is full, intersection will be b
    if(a.full)
        return {b.dir, b.halfAngle, b.full};
    // if any of both are null, intersection will be empty
    if(a.halfAngle == 0.0f || b.halfAngle == 0.0f)
        return {{0.0f, 0.0f, 0.0f}, 0.0f, false};
    AngleOverlap acb = INVALID;
    // if a contains b, intersection will equal b
    if(a.halfAngle >= b.halfAngle) {
        acb = overlapTest(a, b);
        if(acb == CONTAINS)
            return {b.dir, b.halfAngle, b.full};
    }
    // if a is not bigger than b...
    if(acb == INVALID) {
        AngleOverlap bca = overlapTest(b, a);
        // b contains a, return a
        if(bca == CONTAINS)
            return {a.dir, a.halfAngle, a.full};
        // they don't overlap, return empty
        if(bca == NONE)
            return {{0.0f, 0.0f, 0.0f}, 0.0f, false};
    }
    else if(acb == NONE)
        // if a is bigger than B but does not contain it, return empty
        return {{0.0f, 0.0f, 0.0f}, 0.0f, false};
    // General case. We compute the ends of the intersection
    // by rotating each cone direction towards the other direction
    // by their own half angle. This is done avoiding trigonometry,
    // if a.dir == b.dir then just check the halfangles
    if(glm::epsilonEqual(a.dir, b.dir, EPSILON) == vec3b(true)) {
        if(a.halfAngle > b.halfAngle)
            return b;
        return a;
    }
    // cross will point opposite directions depending on whether
    // a.dir is on the clockwise side of b.dir or the other way around,
    // in the context of the plane given by (0,0,0), a.dir and b.dir,
    // where a.dir and b.dir are coplanar. cross will point orthogonally away
    // from that plane one side or the other
    vec3f cross = glm::normalize(glm::cross(a.dir, b.dir));
    // dir3 is obtained by rotating a.dir towards b.dir by a's halfangle.
    // the cross product of (cross, a.dir) tells us which direction is
    // towards b.dir.
    vec3f dir3 = glm::normalize(glm::cross(cross, a.dir)*a.halfAngle+a.dir);
    // dir4 is obtained by rotating b.dir towards a.dir by b's halfangle.
    // the cross product of (-cross, b.dir) tells us which direction is
    // towards a.dir. We negate cross because -cross(a.dir, b.dir) is the same
    // as cross(b.dir, a.dir)
    vec3f dir4 = glm::normalize(glm::cross(-cross, b.dir)*b.halfAngle+b.dir);
    // d is the direction of the intersection angle
    vec3f d = glm::normalize(dir3 + dir4);
    // we compute the tangent of the new halfangle by scaling one of the
    // outer vectors by the inverse of it's projection onto the new
    // angle's direction, and computing the length of the vector that
    // results from going from the central direction to this scaled outer direction
    float tangent = glm::length(d-(dir3/glm::dot(dir3,d)));
    // this handles the imprecision-caused edge case where an angle is created with
    // a very small tangent
    if(glm::epsilonEqual(tangent, 0.0f, EPSILON))
        return {{0.0f, 0.0f, 0.0f}, 0.0f, false};
    return {d, tangent, false};
}

// This is a standard Fisher-Yates random in-place shuffle
template<typename T>
void fy_shuffle(T* v, int count) {
    for(int i = count-1; i > 0; --i) {
        int j = rand()%i;
        std::swap(v[i], v[j]);
    }
}

bool equals(const vec3f& a, const vec3f& b) {
    return glm::epsilonEqual(a, b, EPSILON) == vec3b(true);
}

Angle getCone(const vec3f& p1, const vec3f& p2) {
    // p1 and p2 are assumed to be unit vectors
    vec3f dir = glm::normalize(p1+p2);
    float dist = glm::dot(p1, dir);
    float tan = glm::distance(p1, dir*dist);
    return {dir, tan/dist, false};
}

Angle getCone(const vec3f& p1, const vec3f& p2, const vec3f& p3) {
    // The idea is to compute the two planes that run in between p1,p2 and p2,p3.
    // The direction of the cone will be the intersection of those planes (which
    // happens to be a line) and the radius can be then computed using
    // any of the three original vectors.
    vec3f pn1 =
            glm::cross(
                    p1+p2,
                    glm::cross(p1, p2)
                );
    vec3f pn2 =
            glm::cross(
                    p2+p3,
                    glm::cross(p2, p3)
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

bool insideCone(const Angle& c, const vec3f& v) {
    float dist = glm::dot(v, c.dir);
    float tan = glm::distance(v, c.dir*dist);
    return (dist > 0.0f && tan/dist <= (c.halfAngle+EPSILON));
}

// This is the unroll implementation for the algorithm found at
// http://www.cs.technion.ac.il/~cggc/files/gallery-pdfs/Barequet-1.pdf
// which is an application of the minimum enclosing circle problem to
// the bounding cone problem.  All vectors in "points" are assumed to
// be unit vectors
//
// I left the recursive calls commented wherever they would
// be called for the sake of clarity/readability.
// This only works with four points, not for the generic case.
Angle minConeUnroll(const vec3f& v0, const vec3f& v1, const vec3f& v2, const vec3f& v3) {
    // c = minCone(p);
    Angle c = getCone(v0, v1);
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

// If genMode2D is true, this will calculate the new cones using only
// two points per face instead of 4, hence simulating a 2D grid case
Angle getAngle(const vec3i& pos, Grid::Face f,const vec3i& origin) {
    if(pos == origin)
        return {{0.0f, 0.0f, 0.0f}, 0.0f, true};
    vec3f center = vec3f(pos)+0.5f+vec3f(offsets[f])*0.5f;
    vec3f orig = vec3f(origin)+0.5f;
    static vec3f p[4];
    vec3f rel = center-orig;
    switch(f) {
        case Grid::MINX:
        case Grid::MAXX:
            p[0] = rel+vec3f( 0.0f, 0.5f, 0.5f);
            p[1] = rel+vec3f( 0.0f,-0.5f, 0.5f);
            p[2] = rel+vec3f( 0.0f, 0.5f,-0.5f);
            p[3] = rel+vec3f( 0.0f,-0.5f,-0.5f);
            break;
        case Grid::MINY:
        case Grid::MAXY:
            p[0] = rel+vec3f( 0.5f, 0.0f, 0.5f);
            p[1] = rel+vec3f(-0.5f, 0.0f, 0.5f);
            p[2] = rel+vec3f( 0.5f, 0.0f,-0.5f);
            p[3] = rel+vec3f(-0.5f, 0.0f,-0.5f);
            break;
        case Grid::MINZ:
        case Grid::MAXZ:
            p[0] = rel+vec3f( 0.5f, 0.5f, 0.0f);
            p[1] = rel+vec3f(-0.5f, 0.5f, 0.0f);
            p[2] = rel+vec3f( 0.5f,-0.5f, 0.0f);
            p[3] = rel+vec3f(-0.5f,-0.5f, 0.0f);
            break;
    }
    for(vec3f& v : p) v = glm::normalize(v);
    fy_shuffle(p, 4);
    return minConeUnroll(p[0], p[1], p[2], p[3]);
}

Angle angles[GRIDSIZE][GRIDSIZE];

// Main algorithm!
void Grid::calcAngles() {
    std::queue<vec3i> q;
    static bool vis[GRIDSIZE][GRIDSIZE];
    q.push(origin);
    memset(angles, 0, sizeof(angles));
    memset(vis, 0, sizeof(vis));
    angles[origin.x][origin.y].full = true;
    while(!q.empty()) {
        vec3i front = q.front();
        q.pop();
        // visited
        if(vis[front.x][front.y]) continue;
        vis[front.x][front.y] = true;
        const Angle& frontAngle = angles[front.x][front.y];
        if(frontAngle.halfAngle == 0.0f && !frontAngle.full) continue;
        for(int i = 0; i < 6; ++i) {
            vec3i n = front + offsets[i];
            // Out of bountaries
            if(n.x < 0 || n.y < 0 || n.x >= GRIDSIZE || n.y >= GRIDSIZE || n.z != origin.z) continue;
            // Straight line will have the smallest possible manhattan distance to the origin
            if(manhattanDist(origin, n) < manhattanDist(origin, front)) continue;
            // This is a blocker
            if(blockers[n.x][n.y]) continue;
            // Push it
            q.push(n);
            angles[n.x][n.y] = angleUnion(
                angles[n.x][n.y],
                angleIntersection(getAngle(front, (Grid::Face) i, origin), frontAngle)
            );
        }
    }
    updateGridTex();
}

Grid::Grid() {
    blockers = std::vector<std::vector<bool>>(GRIDSIZE, std::vector<bool>(GRIDSIZE, false));
    initGridTex();
    initQuadMesh();
    initLinesMesh();
    updateGridTex();
}

Grid::~Grid() {
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
    blockers[c.x][c.y] = !blockers[c.x][c.y];
    calcAngles();
}

void Grid::updateGridTex() {
    std::vector<char> pixels(GRIDSIZE*GRIDSIZE*4, 0);
    for(int x = 0; x < GRIDSIZE; ++x) {
        for(int y = 0; y < GRIDSIZE; ++y) {
            if(vec2i(x, y) == vec2i(origin)) {
                // Origin painted Yellow
                pixels[x*4+y*GRIDSIZE*4  ] = 100;
                pixels[x*4+y*GRIDSIZE*4+1] = 100;
                pixels[x*4+y*GRIDSIZE*4+2] = 10;
            }
            else if(blockers[x][y]) {
                // Blockers painted gray
                pixels[x*4+y*GRIDSIZE*4  ] = 15;
                pixels[x*4+y*GRIDSIZE*4+1] = 15;
                pixels[x*4+y*GRIDSIZE*4+2] = 15;
            }
            else if(angles[x][y].halfAngle > 0 || angles[x][y].full) {
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
