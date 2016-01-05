#include "Angle.hpp"
#include "Manager.hpp"

Angle::Angle() {
	std::vector<Vertex::Attribute> elems = {
		Vertex::Attribute("a_position", Vertex::Attribute::Float, 3)
	};
	lines = Mesh(Vertex::Format(elems));
	lines.setPrimitiveType(Mesh::LINES);
	triangles = Mesh(Vertex::Format(elems));
	triangles.setPrimitiveType(Mesh::TRIANGLE_FAN);
	updateVerts();
}

Angle::~Angle() {
}

float modAngle(float angle) {
	if(angle > M_PI) return angle - 2*M_PI;
	if(angle < -M_PI) return angle + 2*M_PI;
	return angle;
}

Angle::AngleOverlap Angle::contains(const vec2f& dir, float half, bool full) const {
	if(full) return this->full ? CONTAINS : PARTIAL;
	if(this->full) return CONTAINS;
	vec3f dir1 = vec3f(dir, 0.0f);
	vec3f dir2 = vec3f(this->dir, 0.0f);
	vec3f side = glm::cross(glm::normalize(glm::cross(dir1, dir2)), dir1)*half;
	vec3f p1 = dir1-side;
	vec3f p2 = dir1+side;
	float l1 = glm::dot(p1, dir2);
	float l2 = glm::dot(p2, dir2);
	float r1 = glm::length(p1-dir2*l1);
	float r2 = glm::length(p2-dir2*l2);
	int result = 0;
	if(l1 > 0.0f && r1/l1 < this->half) result++;
	if(l2 > 0.0f && r2/l2 < this->half) result++;
	return static_cast<AngleOverlap>(result);
};

AngleDef Angle::angleUnion(const Angle* a, const Angle* b) {
	if(a->isFull() || b->isFull()) return {{0.0f, 1.0f}, 0.0f, true};
	AngleOverlap acb = a->contains(b);
	if(acb  == CONTAINS) return {a->getDir(), a->getHalfAngle(), false};
	if(acb == PARTIAL && b->contains(a) == CONTAINS) return {b->getDir(), b->getHalfAngle(), false};
	vec3f dir1 = vec3f(a->getDir(), 0.0f);
	vec3f dir2 = vec3f(b->getDir(), 0.0f);
	vec3f cross = glm::normalize(glm::cross(dir1, dir2));
	vec3f dir3 = glm::normalize(-glm::cross(cross, dir1)*a->getHalfAngle()+dir1);
	vec3f dir4 = glm::normalize(-glm::cross(-cross, dir2)*b->getHalfAngle()+dir2);
	vec3f d = glm::normalize(dir3 + dir4);
	if(glm::dot(dir1+dir2, d) <= 0.0f) return {{0.0f, 1.0f}, 0.0f, true};
	else return {vec2f(d), glm::length(d-dir3*(1.0f/glm::dot(dir3,d))), false};
}

AngleDef Angle::angleIntersection(const Angle* a, const Angle* b) {
	AngleOverlap acb = a->contains(b);
	if(a->isFull() || acb == CONTAINS) return {b->getDir(), b->getHalfAngle(), b->isFull()};
	else if(acb == PARTIAL) {
		AngleOverlap bca = b->contains(a);
		if(b->isFull() || bca == CONTAINS) return {a->getDir(), a->getHalfAngle(), a->isFull()};
	}
	else 
		return {{0.0f, 1.0f}, 0.0f, false};
	vec3f dir1 = vec3f(a->getDir(), 0.0f);
	vec3f dir2 = vec3f(b->getDir(), 0.0f);
	vec3f cross = glm::normalize(glm::cross(dir1, dir2));
	vec3f dir3 = glm::normalize(glm::cross(cross, dir1)*a->getHalfAngle()+dir1);
	vec3f dir4 = glm::normalize(glm::cross(-cross, dir2)*b->getHalfAngle()+dir2);
	vec3f d = glm::normalize(dir3 + dir4);
	if(glm::dot(dir1+dir2, d) <= 0.0f) return {{0.0f, 1.0f}, 0.0f, true};
	else return {vec2f(d), glm::length(d-dir3*(1.0f/glm::dot(dir3,d))), false};
}

void Angle::set(vec2f dir, float halfAngle, bool full) {
	dir = glm::normalize(dir);
	VBE_ASSERT(!glm::isnan(dir).x && !glm::isnan(dir).y, "setAngle needs a non-zero dir");
	this->dir = dir;
	if(full) {
		half = 0.0f;
		this->full = true;
	}
	else {
		VBE_ASSERT(halfAngle >= 0.0f, "Angle must be positive");
		VBE_ASSERT(glm::atan(halfAngle) <= M_PI, "Angle too big!");
		half = halfAngle;
		this->full = false;
	}
	updateVerts();
}

void Angle::updateVerts() {
	float radHalf = 0.0f;
	if(full) radHalf = M_PI;
	else radHalf = glm::atan(half);
	vec3f p1 = vec3f(glm::rotate(dir, radHalf), 0.0f); // small angle
	vec3f p2 = vec3f(glm::rotate(dir, -radHalf), 0.0f); // big angle
	std::vector<vec3f> data;
	data.push_back(vec3f(0.0f));
	data.push_back(p1);
	data.push_back(vec3f(0.0f));
	data.push_back(p2);
	data.push_back(vec3f(0.0f));
	data.push_back(vec3f(dir, 0.0f));
	lines.setVertexData(&data[0], data.size());

	data.clear();
	data.push_back(vec3f(0.0f));
	for(float current = -radHalf; current < radHalf; current = current+0.1f) {
		data.push_back(vec3f(glm::rotate(dir, current), 0.0f));
	}
	data.push_back(p1);
	triangles.setVertexData(&data[0], data.size());
}

void Angle::update(float deltaTime) {
	(void) deltaTime;
	transform = glm::translate(mat4f(1.0f), center);
	transform = glm::scale(transform, vec3f(magnitude));
}

void Angle::draw() const {
	if(!doDraw) return;
	const Camera* cam = (Camera*) getGame()->getObjectByName("mainCamera");
	ProgramManager.get("colored").uniform("u_color")->set(vec4f(color, 1.0f));
	ProgramManager.get("colored").uniform("MVP")->set(cam->projection*cam->getView()*fullTransform);
	lines.draw(ProgramManager.get("colored"));
	ProgramManager.get("colored").uniform("u_color")->set(vec4f(color, 0.1f));
	triangles.draw(ProgramManager.get("colored"));
}
