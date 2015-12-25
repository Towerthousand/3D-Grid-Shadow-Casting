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
	float ratio = 16.0f/9.0f;
	camera->projection = glm::ortho(-zoom*ratio, zoom*ratio, -zoom, zoom, -100.0f, 100.0f);
	camera->addTo(this);

	Grid* g = new Grid();
	g->addTo(this);
}

Scene::~Scene() {
}

void Scene::update(float deltaTime) {
	(void) deltaTime;
	if(Keyboard::justPressed(Keyboard::Escape)) getGame()->isRunning = false;
}

void Scene::draw() const {
	GL_ASSERT(glClear(GL_COLOR_BUFFER_BIT));
}
