#include <emt/window/application.h>
#include "scene/render_scene.h"

int main(int args, char* argv[])
{
	emt::application app(args, argv, 1024, 860);

	emt::dx_context_core* context = (emt::dx_context_core*)app.get_context();

	emt::render_scene scene(context);

	int exit_code = app.execute_scene(&scene);

	return exit_code;
}