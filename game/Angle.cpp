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

AngleDef Angle::angleUnion(const Angle* a, const Angle* b) {
	float diff = glm::acos(glm::dot(a->getDir(),b->getDir()));
	if(glm::isnan(diff)) diff = 0;
	if(a->getHalfAngle() >= diff+b->getHalfAngle()) // a contains b
		return {a->getDir(), a->getHalfAngle()};
	else if(b->getHalfAngle() >= diff+a->getHalfAngle()) // b contains a
		return {b->getDir(), b->getHalfAngle()};
	// general case
	float radA = glm::atan(a->getDir().x, a->getDir().y);
	float radB = glm::atan(b->getDir().x, b->getDir().y);
	if(glm::epsilonNotEqual(modAngle(radA + diff*0.5f), modAngle(radB - diff*0.5f), 0.0001f)) {
		// a is not the left one.. swap!
		std::swap(radA, radB);
		std::swap(a, b);
	}
	float angle = glm::min((diff + a->getHalfAngle() + b->getHalfAngle())*0.5f, float(M_PI*0.9999));
	float start = modAngle(radA - a->getHalfAngle());
	vec2f dir = vec2f(glm::sin(start+angle), glm::cos(start+angle));
	return {
		glm::normalize(dir),
		angle
	};
}

AngleDef Angle::angleIntersection(const Angle* a, const Angle* b) {
	float diff = glm::acos(glm::dot(a->getDir(),b->getDir()));
	if(glm::isnan(diff)) diff = 0;
	if(a->getHalfAngle() >= diff+b->getHalfAngle()) // a contains b
		return {b->getDir(), b->getHalfAngle()};
	else if(b->getHalfAngle() >= diff+a->getHalfAngle()) // b contains a
		return {a->getDir(), a->getHalfAngle()};
	else if(diff > a->getHalfAngle() + b->getHalfAngle()) // no intersection
		return {vec2f(0.0f, 1.0f), 0.0f};
	// general case
	float radA = glm::atan(a->getDir().x, a->getDir().y);
	float radB = glm::atan(b->getDir().x, b->getDir().y);
	if(glm::epsilonNotEqual(modAngle(radA + diff*0.5f), modAngle(radB - diff*0.5f), 0.0001f)) {
		// a is not the left one.. swap!
		std::swap(radA, radB);
		std::swap(a, b);
	}
	float start = modAngle(radB - b->getHalfAngle());
    float end = modAngle(radA + a->getHalfAngle());
	if(start > end) start -= 2*M_PI;
    float angle = (end-start)*0.5f;
	vec2f dir = vec2f(glm::sin(start+angle), glm::cos(start+angle));
	return {
		glm::normalize(dir),
		angle
	};
}

void Angle::set(vec2f dir, float halfAngle) {
	VBE_ASSERT(halfAngle >= 0.0f, "Angle must be positive");
	VBE_ASSERT(halfAngle <= M_PI, "Angle too big!");
	dir = glm::normalize(dir);
	VBE_ASSERT(!glm::isnan(dir).x && !glm::isnan(dir).y, "setAngle needs a non-zero dir");
	this->dir = dir;
	half = halfAngle;
	updateVerts();
}

void Angle::updateVerts() {
	vec3f p1 = vec3f(glm::rotate(dir, half), 0.0f); // small angle
	vec3f p2 = vec3f(glm::rotate(dir, -half), 0.0f); // big angle
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
	for(float current = -half; current < half; current = current+0.1f) {
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
