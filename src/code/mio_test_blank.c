
#define MIO_IMPLEMENTATION
#include "mio.h"

int32 mio_user_init(void *state) {

	return TRUE;
}

int32 mio_user_update(void *state, real64 dt) {

	return TRUE;
}

int32 mio_user_draw(void *state) {

	return TRUE;
}

int main(void) {
  mio_run(NULL, "Test", 1280, 800, 1, 60);
  return 0;
}

