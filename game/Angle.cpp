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

Angle::AngleOverlap Angle::overlapTest(const AngleDef& a, const AngleDef& b) {
	if(a.full) return CONTAINS;
	if(b.full) return NONE;
	vec3f dir1 = vec3f(b.dir, 0.0f);
	vec3f dir2 = vec3f(a.dir, 0.0f);
	if(glm::epsilonEqual(dir1, dir2, 0.000001f) == vec3b(true)) {
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
	// CONTAINS: this angle fully contains the other
	int result = 0;
	if(l1 > 0.0f && r1/l1 < a.halfAngle) result++;
	if(l2 > 0.0f && r2/l2 < a.halfAngle) result++;
	return static_cast<AngleOverlap>(result);
};

AngleDef Angle::angleUnion(const AngleDef& a, const AngleDef& b) {
	// if any of both are full, union will be full
	if(a.full || b.full)
		return {{0.0f, 1.0f}, 0.0f, true};
	if(a.halfAngle == 0.0f)
		return b;
	if(b.halfAngle == 0.0f)
		return a;
	// if a contains b, result is a
	if(a.halfAngle >= b.halfAngle && overlapTest(a, b) == CONTAINS)
		return a;
	// and viceversa
	if(b.halfAngle >= a.halfAngle && overlapTest(b, a) == CONTAINS)
		return b;
	// General case. We compute the ends of the intersection
	// by rotating each cone direction away from the other direction
	// by their own half angle. This is done avoiding trigonometry,
	vec3f dir1 = vec3f(a.dir, 0.0f);
	vec3f dir2 = vec3f(b.dir, 0.0f);
	if(glm::epsilonEqual(dir1, dir2, 0.000001f) == vec3b(true)) {
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
	if(glm::dot(dir1+dir2, d) <= 0.0f) return {{0.0f, 1.0f}, 0.0f, true};
	// we compute the tangent of the new halfangle by scaling one of the
	// outer vectors by the inverse of it's projection onto the new
	// angle's direction, and computing the length of the vector that
	// results from going from the central direction to this scaled outer direction
	return {vec2f(d), glm::length(d-(dir3/glm::dot(dir3,d))), false};
}

AngleDef Angle::angleIntersection(const AngleDef& a, const AngleDef& b) {
	// if b is full, intersection will be a
	if(b.full)
		return {a.dir, a.halfAngle, a.full};
	// if a is full, intersection will be b
	if(a.full)
		return {b.dir, b.halfAngle, b.full};
	// if any of both are null, intersection will be empty
	if(a.halfAngle == 0.0f || b.halfAngle == 0.0f)
		return {{0.0f, 1.0f}, 0.0f, false};
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
			return {{0.0f, 1.0f}, 0.0f, false};
	}
	else if(acb == NONE)
		// if a is bigger than B but does not contain it, return empty
		return {{0.0f, 1.0f}, 0.0f, false};
	// General case. We compute the ends of the intersection
	// by rotating each cone direction towards the other direction
	// by their own half angle. This is done avoiding trigonometry,
	vec3f dir1 = vec3f(a.dir, 0.0f);
	vec3f dir2 = vec3f(b.dir, 0.0f);
	if(glm::epsilonEqual(dir1, dir2, 0.000001f) == vec3b(true)) {
		VBE_LOG("TOP_KEK");
		if(a.halfAngle > b.halfAngle)
			return b;
		return a;
	}
	// cross will point opposite directions depending on whether
	// dir1 is on the clockwise side of dir2 or the other way around,
	// in the context of the plane given by (0,0,0), dir1 and dir2,
	// where dir1 and dir2 are coplanar. cross will point orthogonally away
	// from that plane one side or the other
	vec3f cross = glm::normalize(glm::cross(dir1, dir2));
	// dir3 is obtained by rotating dir1 towards dir2 by a's halfangle.
	// the cross product of (cross, dir1) tells us which direction is
	// towards dir2.
	vec3f dir3 = glm::normalize(glm::cross(cross, dir1)*a.halfAngle+dir1);
	// dir4 is obtained by rotating dir2 towards dir1 by b's halfangle.
	// the cross product of (-cross, dir2) tells us which direction is
	// towards dir1. We negate cross because -cross(dir1, dir2) is the same
	// as cross(dir2, dir1)
	vec3f dir4 = glm::normalize(glm::cross(-cross, dir2)*b.halfAngle+dir2);
	// d is the direction of the intersection angle
	vec3f d = glm::normalize(dir3 + dir4);
	// we compute the tangent of the new halfangle by scaling one of the
	// outer vectors by the inverse of it's projection onto the new
	// angle's direction, and computing the length of the vector that
	// results from going from the central direction to this scaled outer direction
	return {vec2f(d), glm::length(d-(dir3/glm::dot(dir3,d))), false};
}

void Angle::set(const AngleDef& newDef) {
	def.dir = glm::normalize(newDef.dir);
	VBE_ASSERT(!glm::isnan(def.dir).x && !glm::isnan(def.dir).y, "setAngle needs a non-zero dir " << newDef.dir);
	if(newDef.full) {
		def.halfAngle = 0.0f;
		def.full = true;
	}
	else {
		VBE_ASSERT(newDef.halfAngle >= 0.0f, "Angle must be positive");
		VBE_ASSERT(glm::atan(newDef.halfAngle) <= M_PI, "Angle too big!");
		def.halfAngle = newDef.halfAngle;
		def.full = false;
	}
	updateVerts();
}

void Angle::updateVerts() {
	float radHalf = 0.0f;
	if(def.full) radHalf = M_PI;
	else radHalf = glm::atan(def.halfAngle);
	vec3f p1 = vec3f(glm::rotate(def.dir, radHalf), 0.0f); // small angle
	vec3f p2 = vec3f(glm::rotate(def.dir, -radHalf), 0.0f); // big angle
	std::vector<vec3f> data;
	data.push_back(vec3f(0.0f));
	data.push_back(p1);
	data.push_back(vec3f(0.0f));
	data.push_back(p2);
	data.push_back(vec3f(0.0f));
	data.push_back(vec3f(def.dir, 0.0f));
	lines.setVertexData(&data[0], data.size());

	data.clear();
	data.push_back(vec3f(0.0f));
	for(float current = -radHalf; current < radHalf; current = current+0.1f) {
		data.push_back(vec3f(glm::rotate(def.dir, current), 0.0f));
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
