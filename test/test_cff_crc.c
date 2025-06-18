#include "cff.h"
#include "unity.h"
#include <string.h>

void setUp(void)
{
    // Set up code to run before each test
}

void tearDown(void)
{
    // Clean up code to run after each test
}

void test_crc16_known_test_vector(void)
{
    // Test vector from CRC-16/CCITT-FALSE specification
    const char *test_data = "123456789";
    uint16_t expected_crc = 0x29B1;
    uint16_t actual_crc;

    cff_error_en_t result = cff_crc16((const uint8_t *) test_data, strlen(test_data), &actual_crc);

    TEST_ASSERT_EQUAL(cff_error_none, result);
    TEST_ASSERT_EQUAL_HEX16(expected_crc, actual_crc);
}

void test_crc16_empty_data(void)
{
    uint16_t crc;
    cff_error_en_t result = cff_crc16((const uint8_t *) "", 0, &crc);

    TEST_ASSERT_EQUAL(cff_error_none, result);
    TEST_ASSERT_EQUAL_HEX16(CFF_CRC_INIT, crc);
}

void test_crc16_null_pointer(void)
{
    uint16_t crc;
    cff_error_en_t result = cff_crc16(NULL, 10, &crc);

    TEST_ASSERT_EQUAL(cff_error_null_pointer, result);
}

void test_crc16_single_byte(void)
{
    uint8_t single_byte = 'A';
    uint16_t crc;

    cff_error_en_t result = cff_crc16(&single_byte, 1, &crc);

    TEST_ASSERT_EQUAL(cff_error_none, result);
    TEST_ASSERT_NOT_EQUAL(0, crc);
    TEST_ASSERT_NOT_EQUAL(CFF_CRC_INIT, crc);
}

void test_crc16_multiple_calculations(void)
{
    // Test that multiple calculations with the same data produce the same result
    const uint8_t test_data[] = {0x01, 0x02, 0x03, 0x04, 0x05};
    uint16_t crc1, crc2;

    cff_error_en_t result1 = cff_crc16(test_data, sizeof(test_data), &crc1);
    cff_error_en_t result2 = cff_crc16(test_data, sizeof(test_data), &crc2);

    TEST_ASSERT_EQUAL(cff_error_none, result1);
    TEST_ASSERT_EQUAL(cff_error_none, result2);
    TEST_ASSERT_EQUAL_HEX16(crc1, crc2);
}

void test_crc16_different_lengths(void)
{
    const uint8_t test_data[] = {0x01, 0x02, 0x03, 0x04, 0x05};
    uint16_t crc_short, crc_long;

    cff_error_en_t result1 = cff_crc16(test_data, 3, &crc_short);
    cff_error_en_t result2 = cff_crc16(test_data, 5, &crc_long);

    TEST_ASSERT_EQUAL(cff_error_none, result1);
    TEST_ASSERT_EQUAL(cff_error_none, result2);
    TEST_ASSERT_NOT_EQUAL(crc_short, crc_long);
}
