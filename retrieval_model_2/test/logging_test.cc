#include "../misc/backend.h"

int main() {
  static char *log_test_id = "test id";
  char *message = "test message";
  log(LOG_DEBUG, log_test_id, message);
}
