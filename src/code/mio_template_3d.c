#define MIO_IMPLEMENTATION
#include "mio.h"

/* @PHYSICS:ENTITY ***********************************************************/

mioCamera camera;

int32 mio_user_init(void *state) {
	camera = mio_3d_camera(1, 1.0f, 0.01f, 100.0f, 10.0f, 1.0f);
	camera.pos = mio_vec3(-10.0f, 10.0f, 10.0f);
	camera.pitch = MIO_DEG_TO_RAD(35);
	camera.yaw = MIO_DEG_TO_RAD(45);
	return 1;
}

int32 mio_user_update(void *state, real64 dt) {
	mio_3d_camera_update(&camera,G_APP.render.width, G_APP.render.height, TRUE, dt);
	return 1;
}

int32 mio_user_draw(void *state) {
	mio_set_colour(0xFF000000);
	mio_draw_clear();
	mio_3d_draw_grid(&camera, 1.0f, 10, 50.0f, 0xFFFFFFFF, FALSE, 1.0f, 1.0f, 1.0f, 0.1f, 0.5f, TRUE, mio_mat4_identity());
	return 1;
}

int32 mio_user_exit(void *state) { return 1; }

int main(void) {
  mio_run(NULL, "Test", 1280, 800, 1, 60);
  return 0;
}
