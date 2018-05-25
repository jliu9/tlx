/*******************************************************************************
 * tests/string_test.cpp
 *
 * Part of tlx - http://panthema.net/tlx
 *
 * Copyright (C) 2007-2017 Timo Bingmann <tb@panthema.net>
 *
 * All rights reserved. Published under the Boost Software License, Version 1.0
 ******************************************************************************/

#include <random>
#include <stdexcept>

#include <tlx/die.hpp>
#include <tlx/string.hpp>

//! Returns an initialized unsigned char[] array inside an std::string
#define ARRAY_AS_STRING(array) \
    std::string(reinterpret_cast<const char*>(array), sizeof(array))

/*!
 * Generate a random binary string of given length. Any byte from 0-256 is
 * equally probable. Uses the pseudo-random number generator from stdlib; take
 * care to seed it using srand() before calling this function.
 *
 * \param size  length of result
 * \return      random binary string of given length
 */
static inline
std::string random_binary(std::string::size_type size) {
    static std::random_device random_device;
    std::minstd_rand prng(random_device());

    std::string out;
    out.resize(size);

    for (size_t i = 0; i < size; ++i)
        out[i] = static_cast<unsigned char>(prng() % 256);

    return out;
}

static void test_base64() {
    // take some static hex data and dump it using base64 encoding, then decode
    // it again.
    const unsigned char rand1data[42] = {
        0x16, 0x35, 0xCA, 0x03, 0x90, 0x6B, 0x47, 0x11,
        0x85, 0x02, 0xE7, 0x40, 0x9E, 0x3A, 0xCE, 0x43,
        0x0C, 0x57, 0x3E, 0x35, 0xE7, 0xA6, 0xB2, 0x37,
        0xEC, 0x6D, 0xF6, 0x68, 0xF6, 0x0E, 0x74, 0x0C,
        0x44, 0x3F, 0x0F, 0xD4, 0xAA, 0x56, 0xE5, 0x2F,
        0x58, 0xCC
    };

    std::string rand1 = ARRAY_AS_STRING(rand1data);

    std::string rand1base64 = tlx::base64_encode(rand1);

    die_unequal(rand1base64,
                "FjXKA5BrRxGFAudAnjrOQwxXPjXnprI37G32aPYOdAxEPw/UqlblL1jM");

    die_unequal(tlx::base64_decode(rand1base64), rand1);

    // check line-splitting
    std::string rand1base64lines = tlx::base64_encode(rand1, 16);

    die_unequal(rand1base64lines,
                "FjXKA5BrRxGFAudA\n" "njrOQwxXPjXnprI3\n"
                "7G32aPYOdAxEPw/U\n" "qlblL1jM");

    // take three random binary data string with different sizes and run
    // the base64 encoding->decoding->checking drill.

    std::string rand12 = random_binary(12);
    die_unequal(tlx::base64_decode(tlx::base64_encode(rand12)), rand12);

    std::string rand13 = random_binary(13);
    die_unequal(tlx::base64_decode(tlx::base64_encode(rand13)), rand13);

    std::string rand14 = random_binary(14);
    die_unequal(tlx::base64_decode(tlx::base64_encode(rand14)), rand14);

    // run a larger set of random tests
    for (unsigned int ti = 0; ti < 1000; ++ti)
    {
        unsigned int randlen = ti; // rand() % 1000;
        std::string randstr = random_binary(randlen);

        die_unequal(
            tlx::base64_decode(tlx::base64_encode(randstr)), randstr);
    }

    die_unless_throws(
        tlx::base64_decode("FjXKA5!!RxGFAudA"), std::runtime_error);
}

static void test_compare_icase() {
    die_unless(std::string("ABC") != std::string("abc"));

    die_unless(tlx::equal_icase("ABC", "abc"));
    die_unless(!tlx::equal_icase("ABC", "abd"));
    die_unless(!tlx::equal_icase("ABC", "abcedf"));

    die_unless(std::string("ABC") < std::string("abc"));
    die_unless(!tlx::less_icase("ABC", "abc"));
    die_unless(tlx::less_icase("abc", "abcdef"));
    die_unless(!tlx::less_icase("abcdef", "abcd"));

    die_unless(tlx::compare_icase("ABC", "abc") == 0);
    die_unless(tlx::compare_icase("ABC", "abd") < 0);
    die_unless(tlx::compare_icase("ABC", "abb") > 0);
}

static void test_contains_word() {
    std::string data = "test admin write readall read do";

    die_unless(tlx::contains_word(data, "test"));
    die_unless(!tlx::contains_word(data, "testit"));

    die_unless(tlx::contains_word(data, "read"));
    die_unless(tlx::contains_word(data, "readall"));

    die_unless(tlx::contains_word(data, std::string("read")));
    die_unless(tlx::contains_word(data, std::string("readall")));

    die_unless(!tlx::contains_word(data, "doit"));
}

static void test_escape_html() {

    die_unequal(
        tlx::escape_html("hello <tag> \"abc\" & \"def\""),
        "hello &lt;tag&gt; &quot;abc&quot; &amp; &quot;def&quot;");
}

static void test_escape_uri() {

    die_unequal(
        tlx::escape_uri("hello <tag>\""), "hello%20%3Ctag%3E%22");
}

static void test_expand_environment_variables() {

    setenv("TEST_1", "def", /* overwrite */ true);
    setenv("VAR_2", "uvw", /* overwrite */ true);

    die_unequal(
        tlx::expand_environment_variables("abc$TEST_1 ---${VAR_2}xyz"),
        "abcdef ---uvwxyz");

    die_unequal(
        tlx::expand_environment_variables("abc$4TEST_1 -$$--${VAR_2}xyz"),
        "abc$4TEST_1 -$$--uvwxyz");

    die_unequal(
        tlx::expand_environment_variables("abc${NON_EXISTING_VARIABLE}xyz"),
        "abcxyz");
}

static void test_extract_between() {
    std::string data =
        "Content-Disposition: form-data; name='testfile'; filename='test.html'";

    die_unequal(tlx::extract_between(data, "name='", "'"), "testfile");
    die_unequal(tlx::extract_between(data, "filename='", "'"), "test.html");
    die_unequal(tlx::extract_between(data, "other='", "'"), "");

    die_unequal(tlx::extract_between(data, "Name='", "'"), "");
}

static void test_format_si_iec_units() {

    die_unequal(tlx::format_si_units(33 * 1024 * 1024 * 1024LLU), "35.433 G");
    die_unequal(tlx::format_iec_units(33 * 1024 * 1024 * 1024LLU), "33.000 Gi");
}

static void test_erase_all() {

    die_unequal(
        tlx::erase_all(" abcdef   ghi jk "), "abcdefghijk");

    die_unequal(
        tlx::erase_all("abcdef   ghi jk"), "abcdefghijk");

    die_unequal(
        tlx::erase_all(" abcdef   ghi jk ", " bg"), "acdefhijk");

    die_unequal(
        tlx::erase_all("abcdef   ghi jk", " bg"), "acdefhijk");

    std::string s1 = " abcdef   ghi jk ";
    die_unequal(tlx::erase_all(&s1), "abcdefghijk");

    std::string s2 = "abcdef   ghi jk";
    die_unequal(tlx::erase_all(&s2), "abcdefghijk");

    std::string s3 = " abcdef   ghi jk ";
    die_unequal(tlx::erase_all(&s3, " bg"), "acdefhijk");

    std::string s4 = "abcdef   ghi jk";
    die_unequal(tlx::erase_all(&s4, " bg"), "acdefhijk");
}

static void test_hexdump() {

    // take hex data and dump it into a string, then parse back into array
    const unsigned char hexdump[8] = {
        0x8D, 0xE2, 0x85, 0xD4, 0xBF, 0x98, 0xE6, 0x03
    };

    std::string hexdata = ARRAY_AS_STRING(hexdump);
    std::string hexstring = tlx::hexdump(hexdata);

    die_unequal(hexstring, "8DE285D4BF98E603");
    die_unequal(tlx::hexdump(hexdump, sizeof(hexdump)), "8DE285D4BF98E603");

    std::string hexparsed = tlx::parse_hexdump(hexstring);
    die_unequal(hexparsed, hexdata);

    // dump random binary string into hex and parse it back
    std::string rand1 = random_binary(42);
    die_unequal(tlx::parse_hexdump(tlx::hexdump(rand1)), rand1);

    // take the first hex list and dump it into source code format, then
    // compare it with correct data (which was also dumped with
    // hexdump_sourcecode())
    std::string hexsource = tlx::hexdump_sourcecode(hexdata, "abc");

    const unsigned char hexsourcecmp[68] = {
        0x63, 0x6F, 0x6E, 0x73, 0x74, 0x20, 0x75, 0x69,
        0x6E, 0x74, 0x38, 0x5F, 0x74, 0x20, 0x61, 0x62,
        0x63, 0x5B, 0x38, 0x5D, 0x20, 0x3D, 0x20, 0x7B,
        0x0A, 0x30, 0x78, 0x38, 0x44, 0x2C, 0x30, 0x78,
        0x45, 0x32, 0x2C, 0x30, 0x78, 0x38, 0x35, 0x2C,
        0x30, 0x78, 0x44, 0x34, 0x2C, 0x30, 0x78, 0x42,
        0x46, 0x2C, 0x30, 0x78, 0x39, 0x38, 0x2C, 0x30,
        0x78, 0x45, 0x36, 0x2C, 0x30, 0x78, 0x30, 0x33,
        0x0A, 0x7D, 0x3B, 0x0A
    };

    die_unequal(hexsource, ARRAY_AS_STRING(hexsourcecmp));

    // test parse_hexdump with illegal strings
    die_unless_throws(tlx::parse_hexdump("illegal"), std::runtime_error);
    die_unless_throws(tlx::parse_hexdump("8DE285D4BF98E60"), std::runtime_error);
}

static void test_parse_si_iec_units() {

    uint64_t size;
    die_unless(tlx::parse_si_iec_units(" 33 GiB ", &size));
    die_unequal(33 * 1024 * 1024 * 1024LLU, size);

    die_if(tlx::parse_si_iec_units(" 33 GiBX ", &size));
}

static void test_join() {
    // simple string split and join
    std::vector<std::string> sv = tlx::split('/', "/usr/bin/test");
    die_unequal(sv.size(), 4u);

    die_unequal(tlx::join("--", sv), "--usr--bin--test");
    die_unequal(tlx::join(";", sv), ";usr;bin;test");

    std::vector<std::string> sv2;
    for (unsigned int i = 0; i < 6; ++i)
        sv2.emplace_back("abc");

    die_unequal(tlx::join(".", sv2), "abc.abc.abc.abc.abc.abc");
}

static void test_split() {
    // simple char split
    std::vector<std::string> sv = tlx::split('/', "/usr/bin/test/");

    die_unequal(sv.size(), 5u);
    die_unequal(sv[0], "");
    die_unequal(sv[1], "usr");
    die_unequal(sv[2], "bin");
    die_unequal(sv[3], "test");
    die_unequal(sv[4], "");

    sv = tlx::split('/', "/usr/bin/test", 3);

    die_unequal(sv.size(), 3u);
    die_unequal(sv[0], "");
    die_unequal(sv[1], "usr");
    die_unequal(sv[2], "bin/test");

    // char split with some strange limits
    sv = tlx::split('/', "/usr//bin/test", 0);
    die_unequal(sv.size(), 0u);

    sv = tlx::split('/', "/usr//bin/test", 1);
    die_unequal(sv.size(), 1u);
    die_unequal(sv[0], "/usr//bin/test");

    // simple str split
    sv = tlx::split("/", "/usr/bin/test");

    die_unequal(sv.size(), 4u);
    die_unequal(sv[0], "");
    die_unequal(sv[1], "usr");
    die_unequal(sv[2], "bin");
    die_unequal(sv[3], "test");

    sv = tlx::split("/", "/usr/bin/test", 3);

    die_unequal(sv.size(), 3u);
    die_unequal(sv[0], "");
    die_unequal(sv[1], "usr");
    die_unequal(sv[2], "bin/test");

    // str split with some strange limits
    sv = tlx::split("/", "/usr//bin/test", 0);
    die_unequal(sv.size(), 0u);

    sv = tlx::split("/", "/usr//bin/test", 1);
    die_unequal(sv.size(), 1u);
    die_unequal(sv[0], "/usr//bin/test");

    // str split with parital needle at end
    sv = tlx::split("abc", "testabcblahabcabcab");
    die_unequal(sv.size(), 4u);
    die_unequal(sv[0], "test");
    die_unequal(sv[1], "blah");
    die_unequal(sv[2], "");
    die_unequal(sv[3], "ab");

    // str split with "" separator
    sv = tlx::split("", "abcdef");
    die_unequal(sv.size(), 6u);
    die_unequal(sv[0], "a");
    die_unequal(sv[1], "b");
    die_unequal(sv[2], "c");
    die_unequal(sv[3], "d");
    die_unequal(sv[4], "e");
    die_unequal(sv[5], "f");

    /**************************************************************************/

    // str split with min-limit
    sv = tlx::split('/', "/usr/bin/test", 2, 2);
    die_unequal(sv.size(), 2u);
    die_unequal(sv[0], "");
    die_unequal(sv[1], "usr/bin/test");

    // str split with min-limit
    sv = tlx::split('/', "/usr/bin/test", 5, 5);
    die_unequal(sv.size(), 5u);
    die_unequal(sv[0], "");
    die_unequal(sv[1], "usr");
    die_unequal(sv[2], "bin");
    die_unequal(sv[3], "test");
    die_unequal(sv[4], "");

    // str split with min-limit
    sv = tlx::split("/", "/usr/bin/test", 5, 5);
    die_unequal(sv.size(), 5u);
    die_unequal(sv[0], "");
    die_unequal(sv[1], "usr");
    die_unequal(sv[2], "bin");
    die_unequal(sv[3], "test");
    die_unequal(sv[4], "");
}

static void test_split_join_quoted() {
    // simple whitespace split
    std::vector<std::string> sv = tlx::split_quoted("  ab c df  fdlk f  ");

    die_unequal(sv.size(), 5u);
    die_unequal(sv[0], "ab");
    die_unequal(sv[1], "c");
    die_unequal(sv[2], "df");
    die_unequal(sv[3], "fdlk");
    die_unequal(sv[4], "f");

    die_unequal(tlx::join_quoted(sv), "ab c df fdlk f");

    // simple whitespace split

    sv = tlx::split_quoted("ab c df  fdlk f  ");

    die_unequal(sv.size(), 5u);
    die_unequal(sv[0], "ab");
    die_unequal(sv[1], "c");
    die_unequal(sv[2], "df");
    die_unequal(sv[3], "fdlk");
    die_unequal(sv[4], "f");

    die_unequal(tlx::join_quoted(sv), "ab c df fdlk f");

    // simple whitespace split

    sv = tlx::split_quoted("ab c df  fdlk f");

    die_unequal(sv.size(), 5u);
    die_unequal(sv[0], "ab");
    die_unequal(sv[1], "c");
    die_unequal(sv[2], "df");
    die_unequal(sv[3], "fdlk");
    die_unequal(sv[4], "f");

    die_unequal(tlx::join_quoted(sv), "ab c df fdlk f");

    // with quoted entry
    sv = tlx::split_quoted("ab c \"df  fdlk \" f  ");

    die_unequal(sv.size(), 4u);
    die_unequal(sv[0], "ab");
    die_unequal(sv[1], "c");
    die_unequal(sv[2], "df  fdlk ");
    die_unequal(sv[3], "f");

    die_unequal(tlx::join_quoted(sv), "ab c \"df  fdlk \" f");

    // with quoted entry containing quote
    sv = tlx::split_quoted("ab c \"d\\\\f\\n  \\\"fdlk \" f  ");

    die_unequal(sv.size(), 4u);
    die_unequal(sv[0], "ab");
    die_unequal(sv[1], "c");
    die_unequal(sv[2], "d\\f\n  \"fdlk ");
    die_unequal(sv[3], "f");

    die_unequal(tlx::join_quoted(sv), "ab c \"d\\\\f\\n  \\\"fdlk \" f");
}

static void test_split_words() {
    // simple whitespace split
    std::vector<std::string> sv = tlx::split_words("  ab c df  fdlk f  ");

    die_unequal(sv.size(), 5u);
    die_unequal(sv[0], "ab");
    die_unequal(sv[1], "c");
    die_unequal(sv[2], "df");
    die_unequal(sv[3], "fdlk");
    die_unequal(sv[4], "f");

    sv = tlx::split_words("ab c df  fdlk f  ");

    die_unequal(sv.size(), 5u);
    die_unequal(sv[0], "ab");
    die_unequal(sv[1], "c");
    die_unequal(sv[2], "df");
    die_unequal(sv[3], "fdlk");
    die_unequal(sv[4], "f");

    sv = tlx::split_words("ab c df  fdlk f");

    die_unequal(sv.size(), 5u);
    die_unequal(sv[0], "ab");
    die_unequal(sv[1], "c");
    die_unequal(sv[2], "df");
    die_unequal(sv[3], "fdlk");
    die_unequal(sv[4], "f");

    sv = tlx::split_words("");
    die_unequal(sv.size(), 0u);

    sv = tlx::split_words("    ");
    die_unequal(sv.size(), 0u);

    // whitespace split with limit
    sv = tlx::split_words("  ab c   df  fdlk f  ", 3);

    die_unequal(sv.size(), 3u);
    die_unequal(sv[0], "ab");
    die_unequal(sv[1], "c");
    die_unequal(sv[2], "df  fdlk f  ");

    // whitespace split with some strange limits
    sv = tlx::split_words("  ab c df  fdlk f  ", 0);
    die_unequal(sv.size(), 0u);

    sv = tlx::split_words("  ab c df  fdlk f  ", 1);

    die_unequal(sv.size(), 1u);
    die_unequal(sv[0], "ab c df  fdlk f  ");

    // whitespace split with large limit
    sv = tlx::split_words("  ab  c  df  fdlk f  ", 10);

    die_unequal(sv.size(), 5u);
    die_unequal(sv[0], "ab");
    die_unequal(sv[1], "c");
    die_unequal(sv[2], "df");
    die_unequal(sv[3], "fdlk");
    die_unequal(sv[4], "f");

    // whitespace split with limit at exactly the end
    sv = tlx::split_words("  ab  c  df  fdlk f  ", 5);

    die_unequal(sv.size(), 5u);
    die_unequal(sv[0], "ab");
    die_unequal(sv[1], "c");
    die_unequal(sv[2], "df");
    die_unequal(sv[3], "fdlk");
    die_unequal(sv[4], "f  ");
}

static void test_replace() {
    // copy variants
    die_unequal(
        tlx::replace_first("abcdef abcdef", "abc", "a"), "adef abcdef");
    die_unequal(
        tlx::replace_first("abcdef abcdef", "cba", "a"), "abcdef abcdef");
    die_unequal(
        tlx::replace_all("abcdef abcdef", "abc", "a"), "adef adef");
    die_unequal(
        tlx::replace_all("abcdef abcdef", "cba", "a"), "abcdef abcdef");

    die_unequal(
        tlx::replace_first("abcdef abcdef", "a", "aaa"),
        "aaabcdef abcdef");
    die_unequal(
        tlx::replace_all("abcdef abcdef", "a", "aaa"),
        "aaabcdef aaabcdef");

    // in-place variants
    std::string str1 = "abcdef abcdef";
    std::string str2 = "abcdef abcdef";
    die_unequal(
        tlx::replace_first(&str1, "abc", "a"), "adef abcdef");
    die_unequal(
        tlx::replace_first(&str2, "cba", "a"), "abcdef abcdef");

    str1 = "abcdef abcdef";
    str2 = "abcdef abcdef";
    die_unequal(
        tlx::replace_all(&str1, "abc", "a"), "adef adef");
    die_unequal(
        tlx::replace_all(&str2, "cba", "a"), "abcdef abcdef");

    str1 = "abcdef abcdef";
    str2 = "abcdef abcdef";
    die_unequal(
        tlx::replace_first(&str1, "a", "aaa"), "aaabcdef abcdef");
    die_unequal(
        tlx::replace_all(&str2, "a", "aaa"), "aaabcdef aaabcdef");
}

static void test_starts_with_ends_with() {

    die_unless(tlx::starts_with("abcdef", "abc"));
    die_unless(!tlx::starts_with("abcdef", "def"));
    die_unless(tlx::ends_with("abcdef", "def"));
    die_unless(!tlx::ends_with("abcdef", "abc"));

    die_unless(!tlx::starts_with("abcdef", "ABC"));

    die_unless(tlx::starts_with_icase("abcdef", "ABC"));
    die_unless(!tlx::starts_with_icase("abcdef", "DEF"));
    die_unless(tlx::ends_with_icase("abcdef", "DEF"));
    die_unless(!tlx::ends_with_icase("abcdef", "ABC"));

    die_unless(tlx::starts_with("abcdef", ""));
    die_unless(tlx::ends_with("abcdef", ""));

    die_unless(!tlx::starts_with("", "abc"));
    die_unless(!tlx::ends_with("", "abc"));

    die_unless(tlx::starts_with("", ""));
    die_unless(tlx::ends_with("", ""));
}

static void test_toupper_tolower() {
    // string-copy functions
    die_unequal(tlx::to_upper(" aBc "), " ABC ");
    die_unequal(tlx::to_lower(" AbCdEfG "), " abcdefg ");

    // in-place functions
    std::string str1 = "  aBc  ";
    std::string str2 = "AbCdEfGh ";

    die_unequal(tlx::to_upper(&str1), "  ABC  ");
    die_unequal(tlx::to_lower(&str2), "abcdefgh ");
}

static void test_trim() {
    // string-copy functions
    die_unequal(tlx::trim("  abc  "), "abc");
    die_unequal(tlx::trim("abc  "), "abc");
    die_unequal(tlx::trim("  abc"), "abc");
    die_unequal(tlx::trim("  "), "");

    die_unequal(tlx::trim_left("  abc  "), "abc  ");
    die_unequal(tlx::trim_left("abc  "), "abc  ");
    die_unequal(tlx::trim_left("  "), "");

    die_unequal(tlx::trim_right("  abc  "), "  abc");
    die_unequal(tlx::trim_right("  abc"), "  abc");
    die_unequal(tlx::trim_right("  "), "");

    // in-place functions
    std::string str1 = "  abc  ";
    std::string str2 = "abc  ";
    std::string str3 = "  ";

    die_unequal(tlx::trim_left(&str1), "abc  ");
    die_unequal(tlx::trim_left(&str2), "abc  ");
    die_unequal(tlx::trim_left(&str3), "");

    str1 = "  abc  ";
    str2 = "  abc";
    str3 = "  ";

    die_unequal(tlx::trim_right(&str1), "  abc");
    die_unequal(tlx::trim_right(&str2), "  abc");
    die_unequal(tlx::trim_right(&str3), "");

    str1 = "  abc  ";
    str2 = "  abc";
    str3 = "abc  ";
    std::string str4 = "  ";

    die_unequal(tlx::trim(&str1), "abc");
    die_unequal(tlx::trim(&str2), "abc");
    die_unequal(tlx::trim(&str3), "abc");
    die_unequal(tlx::trim(&str4), "");
}

static void test_word_wrap() {

    const char* text =
        "Alice was beginning to get very tired of sitting by her sister on the "
        "bank, and of having nothing to do: once or twice she had peeped into "
        "the book her sister was reading, but it had no pictures or "
        "conversations in it, 'and what is the use of a book,' thought Alice "
        "'without pictures or  conversations?'\n\nSo she was considering in "
        "her own mind (as well as she could, for the hot day made her feel "
        "very sleepy and stupid), whether the pleasure of making a daisy-chain "
        "would be worth the trouble of getting up and picking the daisies, "
        "when suddenly a White Rabbit with pink eyes ran close by "
        "her.\n\nThere was nothing so VERY remarkable in that; nor did Alice "
        "think it so VERY much out of the way to hear the Rabbit say to "
        "itself, 'Oh dear! Oh dear! I shall be late!' (when she thought it "
        "over afterwards, it occurred to her that she ought to have wondered "
        "at this, but at the time it all seemed quite natural); but when the "
        "Rabbit actually TOOK A WATCH OUT OF ITS WAISTCOAT-POCKET, and looked "
        "at it, and then hurried on, Alice started to her feet, for it flashed "
        "across her mind that she had never before seen a rabbit with either a "
        "waistcoat-pocket, or a watch to take out of it, and burning with "
        "curiosity, she ran across the field after it, and fortunately was "
        "just in time to see it pop down a large rabbit-hole under the "
        "hedge.\nIn another moment down went Alice after it, never once "
        "considering how in the world she was to get out again.\n\nThe "
        "rabbit-hole went straight on like a tunnel for some way, and then  "
        "dipped suddenly down, so suddenly that Alice had not a moment to "
        "think about stopping herself before she found herself falling down a "
        "very deep well.\n\nEither the well was very deep, or she fell very "
        "slowly, for she had plenty of time as she went down to look about her "
        "and to wonder what was going to happen next. First, she tried to look "
        "down and make out what she was coming to, but it was too dark to see "
        "anything; then she looked at the sides of the well, and noticed that "
        "they were filled with cupboards and book-shelves; here and there she "
        "saw maps and pictures hung upon pegs. She took down a jar from one of "
        "the shelves as she passed; it was labelled 'ORANGE MARMALADE', but to "
        "her great disappointment it was empty: she did not like to drop the "
        "jar for fear of killing somebody, so managed to put it into one of "
        "the cupboards as she fell past it.\n\n'Well!' thought Alice to "
        "herself, 'after such a fall as this, I shall think nothing of "
        "tumbling down stairs! How brave they'll all think me at home! Why, I "
        "wouldn't say anything about it, even if I fell off the top of the "
        "house!' (Which was very likely true.)";

    const char* text_correct =
        "Alice was beginning to get very tired of sitting by her\n"
        "sister on the bank, and of having nothing to do: once or\n"
        "twice she had peeped into the book her sister was reading,\n"
        "but it had no pictures or conversations in it, 'and what is\n"
        "the use of a book,' thought Alice 'without pictures or \n"
        "conversations?'\n"
        "\n"
        "So she was considering in her own mind (as well as she\n"
        "could, for the hot day made her feel very sleepy and\n"
        "stupid), whether the pleasure of making a daisy-chain would\n"
        "be worth the trouble of getting up and picking the daisies,\n"
        "when suddenly a White Rabbit with pink eyes ran close by\n"
        "her.\n"
        "\n"
        "There was nothing so VERY remarkable in that; nor did\n"
        "Alice think it so VERY much out of the way to hear the\n"
        "Rabbit say to itself, 'Oh dear! Oh dear! I shall be late!'\n"
        "(when she thought it over afterwards, it occurred to her\n"
        "that she ought to have wondered at this, but at the time it\n"
        "all seemed quite natural); but when the Rabbit actually\n"
        "TOOK A WATCH OUT OF ITS WAISTCOAT-POCKET, and looked at it,\n"
        "and then hurried on, Alice started to her feet, for it\n"
        "flashed across her mind that she had never before seen a\n"
        "rabbit with either a waistcoat-pocket, or a watch to take\n"
        "out of it, and burning with curiosity, she ran across the\n"
        "field after it, and fortunately was just in time to see it\n"
        "pop down a large rabbit-hole under the hedge.\n"
        "In another moment down went Alice after it, never once\n"
        "considering how in the world she was to get out again.\n"
        "\n"
        "The rabbit-hole went straight on like a tunnel for some\n"
        "way, and then  dipped suddenly down, so suddenly that Alice\n"
        "had not a moment to think about stopping herself before she\n"
        "found herself falling down a very deep well.\n"
        "\n"
        "Either the well was very deep, or she fell very slowly,\n"
        "for she had plenty of time as she went down to look about\n"
        "her and to wonder what was going to happen next. First, she\n"
        "tried to look down and make out what she was coming to, but\n"
        "it was too dark to see anything; then she looked at the\n"
        "sides of the well, and noticed that they were filled with\n"
        "cupboards and book-shelves; here and there she saw maps and\n"
        "pictures hung upon pegs. She took down a jar from one of\n"
        "the shelves as she passed; it was labelled 'ORANGE\n"
        "MARMALADE', but to her great disappointment it was empty:\n"
        "she did not like to drop the jar for fear of killing\n"
        "somebody, so managed to put it into one of the cupboards as\n"
        "she fell past it.\n"
        "\n"
        "'Well!' thought Alice to herself, 'after such a fall as\n"
        "this, I shall think nothing of tumbling down stairs! How\n"
        "brave they'll all think me at home! Why, I wouldn't say\n"
        "anything about it, even if I fell off the top of the\n"
        "house!' (Which was very likely true.)";

    die_unequal(tlx::word_wrap(text, 60), text_correct);

    const char* long_line =
        "abc abc abc abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz"
        "abcdefghijklmnopqrstuvwxyz xyz xyz abcdefghijklmnopqrstuvwxyz"
        "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz def def def";

    const char* long_line_correct =
        "abc abc abc\n"
        "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz"
        "abcdefghijklmnopqrstuvwxyz\n"
        "xyz xyz\n"
        "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz"
        "abcdefghijklmnopqrstuvwxyz\n"
        "def def def";

    die_unequal(tlx::word_wrap(long_line, 60), long_line_correct);
}

int main() {

    test_base64();
    test_compare_icase();
    test_contains_word();
    test_erase_all();
    test_escape_html();
    test_escape_uri();
    test_expand_environment_variables();
    test_extract_between();
    test_format_si_iec_units();
    test_hexdump();
    test_join();
    test_parse_si_iec_units();
    test_replace();
    test_split();
    test_split_join_quoted();
    test_split_words();
    test_starts_with_ends_with();
    test_toupper_tolower();
    test_trim();
    test_word_wrap();

    return 0;
}

/******************************************************************************/
