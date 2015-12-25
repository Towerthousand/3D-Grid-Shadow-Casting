#include "Scene.hpp"

int main() {
	ContextSettings s;
	s.versionMajor = 4;
	s.versionMinor = 3;
	Game* game = new Game(Window::getFullscreenModes()[0] , s);
	Scene* scene = new Scene();
	scene->addTo(game);
	game->run();
	delete game;
	return 0;
}
