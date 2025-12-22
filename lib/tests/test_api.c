#include "api.h"

#include <criterion/criterion.h>

#ifndef DEBUG
TestSuite(api, .timeout = 10);
#endif

Test(api, test_create_topic_throws_if_args_null);
Test(api, test_create_topic_throws_if_server_host_null);
Test(api, test_create_topic_throws_if_topic_name_null);
Test(api, test_create_topic_throws_if_topic_already_exists);
Test(api, test_create_topic_throws_if_not_enough_partitions);
Test(api, test_create_topic_success);

Test(api, test_update_topic_throws_if_args_null);
Test(api, test_update_topic_throws_if_server_host_null);
Test(api, test_update_topic_throws_if_topic_name_null);
Test(api, test_update_topic_throws_if_topic_does_not_exist);
Test(api, test_update_topic_throws_if_not_enough_partitions);
Test(api, test_update_topic_success_scale_up);
Test(api, test_update_topic_success_scale_down);
Test(api, test_update_topic_success_no_scale);

// TODO: what happens when we update a topic while its serving consumers?
// TODO: what happens when we delete a topic while its serving consumers?
Test(api, test_delete_topic_throws_if_args_null);
Test(api, test_delete_topic_throws_if_server_host_null);
Test(api, test_delete_topic_throws_if_topic_name_null);
Test(api, test_delete_topic_throws_if_topic_does_not_exist);
Test(api, test_delete_topic_success);

Test(api, test_consumer_init_throws_if_args_null);
Test(api, test_consumer_init_throws_if_server_host_null);
Test(api, test_consumer_init_throws_if_topic_name_null);
Test(api, test_consumer_init_throws_if_topic_does_not_exist);
Test(api, test_consumer_init_throws_if_consumer_limit_reached);
Test(api, test_consumer_init_success);

// TODO: complete the rest of tests
