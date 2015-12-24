#include <VBE/VBE.hpp>
#include <VBE-Scenegraph/VBE-Scenegraph.hpp>

int main() {
	ContextSettings s;
	s.versionMajor = 4;
	s.versionMinor = 3;
	Game* game = new Game(Window::DisplayMode::createWindowedMode(500, 500) , s);
	game->run();
	delete game;
	return 0;
}
