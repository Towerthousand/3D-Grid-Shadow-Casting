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

void Angle::set(vec2f dir, float halfAngle) {
	VBE_ASSERT(halfAngle >= 0.0f, "Angle must be positive");
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
	data.push_back(vec3f(1000.0f));
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
