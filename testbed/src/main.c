#include <core/asserts.h>
#include <core/logger.h>

int main() {
  SPACE_FATAL("A test message: %f", 3.14f);
  SPACE_ERROR("A test message: %f", 3.14f);
  SPACE_WARN("A test message: %f", 3.14f);
  SPACE_INFO("A test message: %f", 3.14f);
  SPACE_DEBUG("A test message: %f", 3.14f);
  SPACE_TRACE("A test message: %f", 3.14f);

  SPACE_ASSERT(1 == 0);

  return 0;
}
