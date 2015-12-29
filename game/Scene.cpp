#include "Scene.hpp"
#include "Grid.hpp"
#include "Manager.hpp"

Scene::Scene() {
	this->setName("SCENE");

	Window::getInstance()->setTitle("VoxelGame");
	Mouse::setCursorVisible(true);
	Mouse::setGrab(false);

	ProgramManager.add("textured", ShaderProgram(
		Storage::openAsset("shaders/textured.vert"),
		Storage::openAsset("shaders/textured.frag")
	));
	ProgramManager.add("colored", ShaderProgram(
		Storage::openAsset("shaders/colored.vert"),
		Storage::openAsset("shaders/colored.frag")
	));

	//GL stuff..:
	GL_ASSERT(glClearColor(0, 0, 0, 1));
	GL_ASSERT(glEnable(GL_DEPTH_TEST));
	GL_ASSERT(glEnable(GL_BLEND));
	GL_ASSERT(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
	GL_ASSERT(glDepthFunc(GL_LEQUAL));
	GL_ASSERT(glEnable(GL_CULL_FACE)); //enable backface culling
	GL_ASSERT(glCullFace(GL_BACK));
	GL_ASSERT(glEnable(GL_FRAMEBUFFER_SRGB));

	Profiler* p = new Profiler();
	p->addTo(this);

	camera = new Camera("mainCamera");
	camera->addTo(this);

	Grid* g = new Grid();
	g->addTo(this);
}

Scene::~Scene() {
}

void Scene::updateView(float deltaTime) {
	float speed = 1.0f;
	if(Keyboard::pressed(Keyboard::E)) zoom -= zoom*0.9*deltaTime;
	if(Keyboard::pressed(Keyboard::Q)) zoom += zoom*0.9*deltaTime;
	zoom = glm::clamp(zoom, 1.0f, 15.0f);
	if(Keyboard::pressed(Keyboard::W)) pos.y += zoom*deltaTime*speed;
	if(Keyboard::pressed(Keyboard::A)) pos.x -= zoom*deltaTime*speed;
	if(Keyboard::pressed(Keyboard::S)) pos.y -= zoom*deltaTime*speed;
	if(Keyboard::pressed(Keyboard::D)) pos.x += zoom*deltaTime*speed;
	float ratio = float(Window::getInstance()->getSize().x)/float(Window::getInstance()->getSize().y);
	camera->projection = glm::ortho(-zoom*ratio, zoom*ratio, -zoom, zoom, -100.0f, 100.0f);
	camera->pos = pos;
}

void Scene::update(float deltaTime) {
	updateView(deltaTime);
	if(Keyboard::justPressed(Keyboard::Escape)) getGame()->isRunning = false;
}

void Scene::draw() const {
	GL_ASSERT(glClear(GL_COLOR_BUFFER_BIT));
}
