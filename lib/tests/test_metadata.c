// This test suite requires that a metadata server is running at 127.0.0.1:2181.

#include "metadata.h"

#include <criterion/criterion.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>

// TODO: test for key name format

const struct server server = {.host = "127.0.0.1", .port = 2181};

void setup(void) {
    errno = 0;
    int res = metadata_init(&server);
    cr_assert(res >= 0);
}

void teardown(void) {
    int res = metadata_destroy();
    cr_assert(res >= 0);
}

#ifdef DEBUG
TestSuite(metadata);
#else
TestSuite(metadata, .timeout = 30);
#endif

Test(metadata, test_metadata_init_throws_when_invalid_args, .init = setup,
     .fini = teardown) {
    // act
    int res = metadata_init(NULL);

    // assert
    cr_assert(res < 0);
    cr_assert_eq(errno, EINVAL);
}

Test(metadata, test_metadata_init_throws_when_already_initialized,
     .init = setup, .fini = teardown) {
    // arrange
    // metadata already initialized by setup()

    // act
    int res = metadata_init(&server);

    // assert
    cr_assert(res < 0);
    cr_assert_eq(errno, EALREADY);
}

// TODO: session connection is asynchronous
// Test(metadata, test_metadata_init_throws_when_server_connection_fails) {
//     // arrange
//     struct server nonexistent_server = {.host = "127.0.0.1", .port = 3181};

//     // act
//     int res = metadata_init(&nonexistent_server);

//     // assert
//     cr_assert(res < 0);
//     cr_assert_eq(errno, EIO);
// }

Test(metadata, test_metadata_init_success) {
    // arrange
    errno = 0;

    // act
    int res = metadata_init(&server);

    // assert
    cr_assert(res >= 0);
    cr_assert_eq(errno, 0);

    // cleanup
    metadata_destroy();
}

Test(metadata, test_metadata_destroy_throws_when_not_initialized) {
    // arrange
    errno = 0;

    // act
    int res = metadata_destroy();

    // assert
    cr_assert(res < 0);
    cr_assert_eq(errno, EINVAL);
}

Test(metadata, test_metadata_destroy_success) {
    // arrange
    errno = 0;
    metadata_init(&server);

    // act
    int res = metadata_destroy();

    // assert
    cr_assert(res >= 0);
    cr_assert_eq(errno, 0);
}

Test(metadata, test_metadata_get_throws_when_invalid_args, .init = setup,
     .fini = teardown) {
    // arrange
    char key[512] = "/";
    strcat(key, criterion_current_test->name);
    char buf[512];

    // act
    int res1 = metadata_get(NULL, NULL);
    int errno1 = errno;

    // arrange
    errno = 0;

    // act
    int res2 = metadata_get(NULL, buf);
    int errno2 = errno;

    // arrange
    errno = 0;

    // act
    int res3 = metadata_get(key, NULL);
    int errno3 = errno;

    // assert
    cr_assert(res1 < 0);
    cr_assert_eq(errno1, EINVAL);

    cr_assert(res2 < 0);
    cr_assert_eq(errno2, EINVAL);

    cr_assert(res3 < 0);
    cr_assert_eq(errno3, EINVAL);
}

Test(metadata, test_metadata_get_throws_when_not_initialized) {
    // arrange
    errno = 0;
    char key[512] = "/";
    strcat(key, criterion_current_test->name);
    char buf[512];

    // act
    int res = metadata_get(key, buf);

    // assert
    cr_assert(res < 0);
    cr_assert_eq(errno, EINVAL);
}

Test(metadata, test_metadata_get_throws_when_entry_not_found, .init = setup,
     .fini = teardown) {
    // arrange
    char key[512] = "/test/";
    strcat(key, criterion_current_test->name);
    char buf[512];

    // act
    int res = metadata_get(key, buf);

    // assert
    cr_assert(res < 0);
    cr_assert_eq(errno, ENODATA);
}

Test(metadata, test_metadata_get_success, .init = setup, .fini = teardown) {
    // arrange
    char key[512] = "/";
    strcat(key, criterion_current_test->name);
    char *value = "Hello, World!";
    metadata_set(key, value, strlen(value), PERSISTENT);

    char buf[512];

    // act
    int res = metadata_get(key, buf);

    // assert
    cr_assert(res >= 0);
    cr_assert_eq(errno, 0);
    cr_assert_arr_eq(buf, value, strlen(value));

    // cleanup
    metadata_delete(key);
}

Test(metadata, test_metadata_get_children_throws_when_invalid_args,
     .init = setup, .fini = teardown) {
    // arrange
    char key[512] = "/";
    strcat(key, criterion_current_test->name);

    char **child_entries;

    // act
    int res1 = metadata_get_children(NULL, NULL, NULL);
    int errno1 = errno;

    // arrange
    errno = 0;

    // act
    int res2 = metadata_get_children(key, NULL, NULL);
    int errno2 = errno;

    // arrange
    errno = 0;

    // act
    int res3 = metadata_get_children(key, &child_entries, NULL);
    int errno3 = errno;

    // assert
    cr_assert(res1 < 0);
    cr_assert_eq(errno1, EINVAL);

    cr_assert(res2 < 0);
    cr_assert_eq(errno2, EINVAL);

    cr_assert(res3 < 0);
    cr_assert_eq(errno3, EINVAL);
}

Test(metadata, test_metadata_get_children_throws_when_not_initialized) {
    // arrange
    errno = 0;
    char key[512] = "/";
    strcat(key, criterion_current_test->name);

    char **child_entries;
    unsigned int num_child_entries;

    // act
    int res = metadata_get_children(key, &child_entries, &num_child_entries);

    // assert
    cr_assert(res < 0);
    cr_assert_eq(errno, EINVAL);
}

Test(metadata, test_metadata_get_children_throws_when_entry_not_found,
     .init = setup, .fini = teardown) {
    // arrange
    char key[512] = "/test/";
    strcat(key, criterion_current_test->name);

    char **child_entries;
    unsigned int num_child_entries;

    // act
    int res = metadata_get_children(key, &child_entries, &num_child_entries);

    // assert
    cr_assert(res < 0);
    cr_assert_eq(errno, ENODATA);
}

Test(metadata, test_metadata_get_children_success_when_no_children,
     .init = setup, .fini = teardown) {
    // arrange
    char key[512] = "/";
    strcat(key, criterion_current_test->name);

    char **child_entries;
    unsigned int num_child_entries;

    char *value = "Hello, World!";
    metadata_set(key, value, strlen(value), PERSISTENT);

    // act
    int res = metadata_get_children(key, &child_entries, &num_child_entries);

    // assert
    cr_assert(res >= 0);
    cr_assert_eq(errno, 0);
    cr_assert_eq(num_child_entries, 0);

    // cleanup
    metadata_delete(key);
}

Test(metadata, test_metadata_get_children_success, .init = setup,
     .fini = teardown) {
    // arrange
    char key[512] = "/";
    strcat(key, criterion_current_test->name);
    metadata_set(key, NULL, 0, PERSISTENT);

    char **child_entries;
    unsigned int num_child_entries;

    char child_key1[512];
    memset(child_key1, 0, sizeof(child_key1));
    strcat(child_key1, key);
    strcat(child_key1, "/child1");
    metadata_set(child_key1, NULL, 0, PERSISTENT);

    char child_key2[512];
    memset(child_key2, 0, sizeof(child_key2));
    memset(child_key2, 0, sizeof(child_key2));
    strcat(child_key2, key);
    strcat(child_key2, "/child2");
    metadata_set(child_key2, NULL, 0, PERSISTENT);

    char child_key3[512];
    memset(child_key3, 0, sizeof(child_key3));
    memset(child_key3, 0, sizeof(child_key3));
    strcat(child_key3, key);
    strcat(child_key3, "/child3");
    metadata_set(child_key3, NULL, 0, PERSISTENT);

    int child1_found = 0;
    int child2_found = 0;
    int child3_found = 0;

    // act
    int res = metadata_get_children(key, &child_entries, &num_child_entries);
    for (unsigned int i = 0; i < num_child_entries; i++) {
        if (strcmp(child_entries[i], "child1") == 0) {
            child1_found = 1;
        } else if (strcmp(child_entries[i], "child2") == 0) {
            child2_found = 1;
        } else if (strcmp(child_entries[i], "child3") == 0) {
            child3_found = 1;
        }
    }

    // assert
    cr_assert(res >= 0);
    cr_assert_eq(errno, 0);
    cr_assert(child1_found);
    cr_assert(child2_found);
    cr_assert(child3_found);
    cr_assert_eq(num_child_entries, 3);

    // cleanup
    metadata_delete(child_key1);
    metadata_delete(child_key2);
    metadata_delete(child_key3);
    metadata_delete(key);
}

Test(metadata, test_metadata_set_throws_when_invalid_args, .init = setup,
     .fini = teardown) {
    // arrange
    char key[512] = "/";
    strcat(key, criterion_current_test->name);
    char *value = "Hello, World!";

    // act
    int res1 = metadata_set(NULL, NULL, strlen(value), PERSISTENT);
    int errno1 = errno;

    // arrange
    errno = 0;

    // act
    int res2 = metadata_set(NULL, value, strlen(value), PERSISTENT);
    int errno2 = errno;

    // arrange
    errno = 0;

    // act
    int res3 = metadata_set(key, NULL, strlen(value), PERSISTENT);
    int errno3 = errno;

    // arrange
    errno = 0;

    // act
    int res4 = metadata_set(key, NULL, 0, PERSISTENT);
    int errno4 = errno;

    // assert
    cr_assert(res1 < 0);
    cr_assert_eq(errno1, EINVAL);

    cr_assert(res2 < 0);
    cr_assert_eq(errno2, EINVAL);

    cr_assert(res3 < 0);
    cr_assert_eq(errno3, EINVAL);

    cr_assert(res4 >= 0);
    cr_assert_eq(errno4, 0);

    // cleanup
    metadata_delete(key);
}

Test(metadata, test_metadata_set_throws_when_not_initialized) {
    // arrange
    errno = 0;
    char key[512] = "/";
    strcat(key, criterion_current_test->name);
    char *value = "Hello, World!";

    // act
    int res = metadata_set(key, value, strlen(value), PERSISTENT);

    // assert
    cr_assert(res < 0);
    cr_assert_eq(errno, EINVAL);
}

Test(metadata, test_metadata_set_throws_when_entry_already_exists,
     .init = setup, .fini = teardown) {
    // arrange
    char key[512] = "/";
    strcat(key, criterion_current_test->name);
    char *value = "Hello, World!";
    metadata_set(key, value, strlen(value), PERSISTENT);

    // act
    int res = metadata_set(key, value, strlen(value), PERSISTENT);

    // assert
    cr_assert(res < 0);
    cr_assert_eq(errno, EEXIST);

    // cleanup
    metadata_delete(key);
}

Test(metadata, test_metadata_set_success_persistent, .init = setup,
     .fini = teardown) {
    // arrange
    char key[512] = "/";
    strcat(key, criterion_current_test->name);
    char *value = "Hello, World!";
    char buf[512];

    // act
    int res1 = metadata_set(key, value, strlen(value), PERSISTENT);
    int errno1 = errno;

    // arrange
    errno = 0;

    // act
    int res2 = metadata_get(key, buf);
    int errno2 = errno;

    // assert
    cr_assert(res1 >= 0);
    cr_assert(res2 >= 0);
    cr_assert_eq(errno1, 0);
    cr_assert_eq(errno2, 0);
    cr_assert_arr_eq(buf, value, strlen(value));

    // cleanup
    metadata_delete(key);
}

Test(metadata, test_metadata_set_success_ephemeral, .init = setup,
     .fini = teardown) {
    // arrange
    char key[512] = "/";
    strcat(key, criterion_current_test->name);
    char *value = "Hello, World!";
    char buf[512];

    // act
    int res1 = metadata_set(key, value, strlen(value), EPHEMERAL);
    int errno1 = errno;

    // arrange
    errno = 0;

    // act
    int res2 = metadata_destroy();
    int errno2 = errno;

    // arrange
    errno = 0;
    usleep(15000); // wait for zookeeper to recognize close connection
    metadata_init(&server);

    // act
    int res3 = metadata_get(key, buf);
    int errno3 = errno;

    // arrange
    errno = 0;

    // act
    int res4 = metadata_set(key, value, strlen(value), EPHEMERAL);
    int errno4 = errno;

    // arrange
    errno = 0;

    // act
    int res5 = metadata_get(key, buf);
    int errno5 = errno;

    // assert
    cr_assert(res1 >= 0);
    cr_assert(res2 >= 0);
    cr_assert(res3 < 0);
    cr_assert(res4 >= 0);
    cr_assert(res5 >= 0);
    cr_assert_eq(errno1, 0);
    cr_assert_eq(errno2, 0);
    cr_assert_eq(errno3, ENODATA);
    cr_assert_eq(errno4, 0);
    cr_assert_eq(errno5, 0);
    cr_assert_arr_eq(buf, value, strlen(value));

    // cleanup
    metadata_delete(key);
}

Test(metadata, test_metadata_delete_throws_when_invalid_args, .init = setup,
     .fini = teardown) {
    // act
    int res = metadata_delete(NULL);

    // assert
    cr_assert(res < 0);
    cr_assert_eq(errno, EINVAL);
}

Test(metadata, test_metadata_delete_throws_when_not_initialized) {
    // arrange
    errno = 0;
    char key[512] = "/";
    strcat(key, criterion_current_test->name);

    // act
    int res = metadata_delete(key);

    // assert
    cr_assert(res < 0);
    cr_assert_eq(errno, EINVAL);
}

Test(metadata, test_metadata_delete_throws_when_entry_not_found, .init = setup,
     .fini = teardown) {
    // arrange
    char key[512] = "/";
    strcat(key, criterion_current_test->name);

    // act
    int res = metadata_delete(key);

    // assert
    cr_assert(res < 0);
    cr_assert_eq(errno, ENODATA);
}

Test(metadata, test_metadata_delete_persistent_entry_success, .init = setup,
     .fini = teardown) {
    // arrange
    char key[512] = "/";
    strcat(key, criterion_current_test->name);
    char *value = "Hello, World!";
    char buf[512];
    metadata_set(key, value, strlen(value), PERSISTENT);

    // act
    int res1 = metadata_delete(key);
    int errno1 = errno;

    // arrange
    errno = 0;

    // act
    int res2 = metadata_delete(key);
    int errno2 = errno;

    // arrange
    errno = 0;

    // act
    int res3 = metadata_get(key, buf);
    int errno3 = errno;

    // assert
    cr_assert(res1 >= 0);
    cr_assert_eq(errno1, 0);

    cr_assert(res2 < 0);
    cr_assert_eq(errno2, ENODATA);

    cr_assert(res3 < 0);
    cr_assert_eq(errno3, ENODATA);
}

Test(metadata, test_metadata_delete_ephemeral_entry_success, .init = setup,
     .fini = teardown) {
    // arrange
    char key[512] = "/";
    strcat(key, criterion_current_test->name);
    char *value = "Hello, World!";
    char buf[512];
    metadata_set(key, value, strlen(value), EPHEMERAL);

    // act
    int res1 = metadata_delete(key);
    int errno1 = errno;

    // arrange
    errno = 0;

    // act
    int res2 = metadata_delete(key);
    int errno2 = errno;

    // arrange
    errno = 0;

    // act
    int res3 = metadata_get(key, buf);
    int errno3 = errno;

    // arrange
    errno = 0;
    metadata_set(key, value, strlen(value), EPHEMERAL);
    metadata_destroy();
    usleep(15000); // wait for zookeeper to recognize closed connection
    metadata_init(&server);

    // act
    int res4 = metadata_delete(key);
    int errno4 = errno;

    // arrange
    errno = 0;

    // act
    int res5 = metadata_get(key, buf);
    int errno5 = errno;

    // assert
    cr_assert(res1 >= 0);
    cr_assert_eq(errno1, 0);

    cr_assert(res2 < 0);
    cr_assert_eq(errno2, ENODATA);

    cr_assert(res3 < 0);
    cr_assert_eq(errno3, ENODATA);

    cr_assert(res4 < 0);
    cr_assert_eq(errno4, ENODATA);

    cr_assert(res5 < 0);
    cr_assert_eq(errno5, ENODATA);
}

Test(metadata, test_metadata_delete_recursive_throws_when_invalid_args,
     .init = setup, .fini = teardown) {
    // act
    int res = metadata_delete_recursive(NULL);

    // assert
    cr_assert(res < 0);
    cr_assert_eq(errno, EINVAL);
}

Test(metadata, test_metadata_delete_recursive_throws_when_not_initialized) {
    // arrange
    errno = 0;
    char key[512] = "/";
    strcat(key, criterion_current_test->name);

    // act
    int res = metadata_delete_recursive(key);

    // assert
    cr_assert(res < 0);
    cr_assert_eq(errno, EINVAL);
}

Test(metadata, test_metadata_delete_recursive_throws_when_entry_not_found,
     .init = setup, .fini = teardown) {
    // arrange
    char key[512] = "/";
    strcat(key, criterion_current_test->name);

    // act
    int res = metadata_delete(key);

    // assert
    cr_assert(res < 0);
    cr_assert_eq(errno, ENODATA);
}

Test(metadata, test_metadata_delete_recursive_success_when_no_children,
     .init = setup, .fini = teardown) {
    // arrange
    char key[512] = "/";
    strcat(key, criterion_current_test->name);

    char *value = "Hello, World!";
    char buf[512];
    metadata_set(key, value, strlen(value), PERSISTENT);

    // act
    int res1 = metadata_delete_recursive(key);
    int errno1 = errno;

    // arrange
    errno = 0;

    // act
    int res2 = metadata_delete_recursive(key);
    int errno2 = errno;

    // arrange
    errno = 0;

    // act
    int res3 = metadata_get(key, buf);
    int errno3 = errno;

    // assert
    cr_assert(res1 >= 0);
    cr_assert_eq(errno1, 0);

    cr_assert(res2 < 0);
    cr_assert_eq(errno2, ENODATA);

    cr_assert(res3 < 0);
    cr_assert_eq(errno3, ENODATA);
}

Test(metadata, test_metadata_delete_recursive_success, .init = setup,
     .fini = teardown) {
    // arrange
    char key[512];
    memset(key, 0, sizeof(key));
    strcat(key, "/");
    strcat(key, criterion_current_test->name);
    metadata_set(key, NULL, 0, PERSISTENT);

    strcat(key, "/child1");
    metadata_set(key, NULL, 0, PERSISTENT);

    memset(key, 0, sizeof(key));
    strcat(key, "/");
    strcat(key, criterion_current_test->name);
    strcat(key, "/child2");
    metadata_set(key, NULL, 0, PERSISTENT);

    memset(key, 0, sizeof(key));
    strcat(key, "/");
    strcat(key, criterion_current_test->name);
    strcat(key, "/child2/grandchild1");
    metadata_set(key, NULL, 0, PERSISTENT);

    memset(key, 0, sizeof(key));
    strcat(key, "/");
    strcat(key, criterion_current_test->name);
    strcat(key, "/child2/grandchild2");
    metadata_set(key, NULL, 0, PERSISTENT);

    memset(key, 0, sizeof(key));
    strcat(key, "/");
    strcat(key, criterion_current_test->name);
    char buf[512];

    // act
    int res1 = metadata_delete_recursive(key);
    int errno1 = errno;

    // arrange
    errno = 0;

    // act
    int res2 = metadata_get(key, buf);
    int errno2 = errno;

    // assert
    cr_assert(res1 >= 0);
    cr_assert_eq(errno1, 0);

    cr_assert(res2 < 0);
    cr_assert_eq(errno2, ENODATA);
}
