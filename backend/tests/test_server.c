//
// Created by Artur Twardzik on 24/04/2026.
//

#include "unity.h"

#include "server.c"

/* Called before each test */
void setUp(void) {}

/* Called after each test */
void tearDown(void) {}

void test_replace_in_html_none(void) {
        const char *test_string = "<html></html>";
        const char *expected_string = "<html></html>";

        char *str = malloc(strlen(test_string));
        strcpy(str, test_string);
        replace_in_html(&str, "${{token}}", "replaced");

        TEST_ASSERT_EQUAL_CHAR_ARRAY(str, expected_string, strlen(expected_string));
        free(str);
}


void test_replace_in_html_single(void) {
        const char *test_string = "<html>${{token}}</html>";
        const char *expected_string = "<html>replaced</html>";

        char *str = malloc(strlen(test_string));
        strcpy(str, test_string);
        replace_in_html(&str, "${{token}}", "replaced");

        TEST_ASSERT_EQUAL_CHAR_ARRAY(str, expected_string, strlen(expected_string));
        free(str);
}

void test_replace_in_html_multiple(void) {
        const char *test_string = "<html>${{token}}<h1>TITLE</h1>${{token}}</html>";
        const char *expected_string = "<html>replaced<h1>TITLE</h1>replaced</html>";

        char *str = malloc(strlen(test_string));
        strcpy(str, test_string);
        replace_in_html(&str, "${{token}}", "replaced");

        TEST_ASSERT_EQUAL_CHAR_ARRAY(str, expected_string, strlen(expected_string));
        free(str);
}


int main(void) {
        UNITY_BEGIN();
        RUN_TEST(test_replace_in_html_none);
        RUN_TEST(test_replace_in_html_single);
        RUN_TEST(test_replace_in_html_multiple);
        return UNITY_END();
}
