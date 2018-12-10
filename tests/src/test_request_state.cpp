#include "gtest/gtest.h"

#include <libsettings/settings.h>

#include <internal/request_state.h>

TEST(test_request_state, init_deinit) {
  request_state_t state = {0};
  std::string test_str = "testing";
  const char *test_data = test_str.c_str();

  request_state_init(&state, 7, test_data, strlen(test_data));

  EXPECT_EQ(7, state.msg_id);
  EXPECT_EQ(false, state.match);
  EXPECT_EQ(true, state.pending);
  EXPECT_EQ(strlen(test_data), state.compare_data_len);
  EXPECT_STREQ(test_data, (char *)state.compare_data);

  request_state_deinit(&state);

  EXPECT_EQ(false, state.pending);
}

TEST(test_request_state, match) {
  request_state_t state = {0};

  EXPECT_EQ(false, request_state_match(&state));
}

TEST(test_request_state, check) {
  request_state_t state = {0};
  settings_api_t api = {0};
  std::string test_str = "testing";
  const char *test_data = test_str.c_str();

  EXPECT_DEATH(request_state_check(&state, NULL, NULL, 0), "");

  request_state_init(&state, 9, test_data, strlen(test_data));

  EXPECT_EQ(-1, request_state_check(&state, &api, test_data, strlen(test_data) - 1));

  EXPECT_DEATH(request_state_check(&state, &api, test_data, strlen(test_data)), "");

  state.pending = false;

  EXPECT_EQ(1, request_state_check(&state, &api, test_data, strlen(test_data)));
}
