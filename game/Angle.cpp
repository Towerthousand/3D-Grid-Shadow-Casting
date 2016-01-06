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

Angle::AngleOverlap Angle::contains(const vec2f& dir, float half, bool full) const {
	if(full) return this->full ? CONTAINS : PARTIAL;
	if(this->full) return CONTAINS;
	vec3f dir1 = vec3f(dir, 0.0f);
	vec3f dir2 = vec3f(this->dir, 0.0f);
	// side is the vector that orthogonally points away from dir1. With a
	// magnitude of halfangle tangent
	vec3f side = glm::cross(glm::normalize(glm::cross(dir1, dir2)), dir1)*half;
	// p1 and p2 are the "extremes" of the other angle
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
	// CONTAINS: this angle fully contains the other
	int result = 0;
	if(l1 > 0.0f && r1/l1 < this->half) result++;
	if(l2 > 0.0f && r2/l2 < this->half) result++;
	return static_cast<AngleOverlap>(result);
};

AngleDef Angle::angleUnion(const Angle* a, const Angle* b) {
	// if any of both are full, union will be full
	if(a->isFull() || b->isFull()) return {{0.0f, 1.0f}, 0.0f, true};
	AngleOverlap acb = a->contains(b);
	// if a contains b, result is a
	if(acb == CONTAINS)
		return {a->getDir(), a->getHalfAngle(), false};
	// if a doesn't contain any side of b then check if b contains a
	if(acb == NONE && b->contains(a) == CONTAINS)
		return {b->getDir(), b->getHalfAngle(), false};
	// General case. We compute the ends of the intersection
	// by rotating each cone direction away from the other direction
	// by their own half angle. This is done avoiding trigonometry,
	vec3f dir1 = vec3f(a->getDir(), 0.0f);
	vec3f dir2 = vec3f(b->getDir(), 0.0f);
	// cross will point opposite directions depending on whether
	// dir1 is on the clockwise side of dir2 or the other way around,
	// in the context of the plane given by (0,0,0), dir1 and dir2,
	// where dir1 and dir2 are coplanar. cross will point orthogonally away
	// from that plane one side or the other
	vec3f cross = glm::normalize(glm::cross(dir1, dir2));
	// dir3 is obtained by rotating dir1 away from dir2 by a's halfangle.
	// the cross product of (cross, dir1) tells us which direction is
	// towards dir2.
	vec3f dir3 = glm::normalize(-glm::cross(cross, dir1)*a->getHalfAngle()+dir1);
	// dir4 is obtained by rotating dir2 away from dir1 by b's halfangle.
	// the cross product of (-cross, dir2) tells us which direction is
	// towards dir1. We negate cross because -cross(dir1, dir2) is the same
	// as cross(dir2, dir1)
	vec3f dir4 = glm::normalize(-glm::cross(-cross, dir2)*b->getHalfAngle()+dir2);
	// d is the direction of the union angle
	vec3f d = glm::normalize(dir3 + dir4);
	// if dot(dir1+dir2, d) < 0.0f, the union angle is > 180 deg
	if(glm::dot(dir1+dir2, d) <= 0.0f) return {{0.0f, 1.0f}, 0.0f, true};
	// we compute the tangent of the new halfangle by scaling one of the
	// outer vectors by the inverse of it's projection onto the new
	// angle's direction, and computing the length of the vector that
	// results from going from the central direction to this scaled outer direction
	return {vec2f(d), glm::length(d-(dir3/glm::dot(dir3,d))), false};
}

AngleDef Angle::angleIntersection(const Angle* a, const Angle* b) {
	AngleOverlap acb = a->contains(b);
	// if b is full, intersection will be a
	if(b->isFull()) return {a->getDir(), a->getHalfAngle(), a->isFull()};
	// if A is full or contains b, intersection will equal b
	if(a->isFull() || acb == CONTAINS) return {b->getDir(), b->getHalfAngle(), b->isFull()};
	// if a does not contain any side of b, then either b contains a or they don't overlap
	else if(acb == NONE) {
		AngleOverlap bca = b->contains(a);
		if(b->isFull() || bca == CONTAINS)
			return {a->getDir(), a->getHalfAngle(), a->isFull()};
		// they don't overlap, return dummy angle
		return {{0.0f, 1.0f}, 0.0f, false};
	}
	// General case. We compute the ends of the intersection
	// by rotating each cone direction towards the other direction
	// by their own half angle. This is done avoiding trigonometry,
	vec3f dir1 = vec3f(a->getDir(), 0.0f);
	vec3f dir2 = vec3f(b->getDir(), 0.0f);
	// cross will point opposite directions depending on whether
	// dir1 is on the clockwise side of dir2 or the other way around,
	// in the context of the plane given by (0,0,0), dir1 and dir2,
	// where dir1 and dir2 are coplanar. cross will point orthogonally away
	// from that plane one side or the other
	vec3f cross = glm::normalize(glm::cross(dir1, dir2));
	// dir3 is obtained by rotating dir1 towards dir2 by a's halfangle.
	// the cross product of (cross, dir1) tells us which direction is
	// towards dir2.
	vec3f dir3 = glm::normalize(glm::cross(cross, dir1)*a->getHalfAngle()+dir1);
	// dir4 is obtained by rotating dir2 towards dir1 by b's halfangle.
	// the cross product of (-cross, dir2) tells us which direction is
	// towards dir1. We negate cross because -cross(dir1, dir2) is the same
	// as cross(dir2, dir1)
	vec3f dir4 = glm::normalize(glm::cross(-cross, dir2)*b->getHalfAngle()+dir2);
	// d is the direction of the intersection angle
	vec3f d = glm::normalize(dir3 + dir4);
	// we compute the tangent of the new halfangle by scaling one of the
	// outer vectors by the inverse of it's projection onto the new
	// angle's direction, and computing the length of the vector that
	// results from going from the central direction to this scaled outer direction
	return {vec2f(d), glm::length(d-(dir3/glm::dot(dir3,d))), false};
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
