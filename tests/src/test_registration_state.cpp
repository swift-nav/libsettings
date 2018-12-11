#include "gtest/gtest.h"

#include <libsettings/settings.h>

#include <internal/registration_state.h>

TEST(test_registration_state, init_deinit) {
  registration_state_t state = {0};
  std::string test_str = "testing";
  const char *test_data = test_str.c_str();

  registration_state_init(&state, test_data, strlen(test_data));

  EXPECT_EQ(false, state.match);
  EXPECT_EQ(true, state.pending);
  EXPECT_EQ(strlen(test_data), state.compare_data_len);
  EXPECT_STREQ(test_data, (char *)state.compare_data);

  registration_state_deinit(&state);

  EXPECT_EQ(false, state.pending);
}

TEST(test_registration_state, match) {
  registration_state_t state = {0};

  EXPECT_EQ(false, registration_state_match(&state));
}

TEST(test_registration_state, check) {
  registration_state_t state = {0};
  settings_api_t api = {0};
  std::string test_str = "testing";
  const char *test_data = test_str.c_str();

  EXPECT_DEATH(registration_state_check(&state, NULL, NULL, 0), "");

  registration_state_init(&state, test_data, strlen(test_data));

  EXPECT_EQ(-1, registration_state_check(&state, &api, test_data, strlen(test_data) - 1));

  EXPECT_DEATH(registration_state_check(&state, &api, test_data, strlen(test_data)), "");

  state.pending = false;

  EXPECT_EQ(1, registration_state_check(&state, &api, test_data, strlen(test_data)));
}
