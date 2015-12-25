#include "Scene.hpp"
#include "Grid.hpp"

Scene::Scene() {
	this->setName("SCENE");

	Window::getInstance()->setTitle("VoxelGame");
	Mouse::setGrab(false);
	Mouse::setRelativeMode(true);

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

	camera = new Camera("mainCamera", vec3f(0, 0, -10));
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
