/* Unit tests for spx_hmap */
#include "test_framework.h"
#include "../../src/spx_hmap.h"
#include <string.h>

static uint64_t string_hash(const void *key) {
    const char *str = (const char *)key;
    uint64_t hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}

static int string_cmp(const void *a, const void *b) {
    return strcmp((const char *)a, (const char *)b);
}

TEST(hmap_create_destroy) {
    spx_hmap_t *hmap = spx_hmap_create(100, string_hash, string_cmp);
    ASSERT_NOT_NULL(hmap);
    spx_hmap_destroy(hmap);
}

TEST(hmap_set_and_get) {
    spx_hmap_t *hmap = spx_hmap_create(100, string_hash, string_cmp);
    ASSERT_NOT_NULL(hmap);

    int value = 42;
    spx_hmap_entry_t *entry = spx_hmap_ensure_entry(hmap, "key1", NULL);
    ASSERT_NOT_NULL(entry);

    spx_hmap_entry_set_value(entry, &value);

    void *retrieved = spx_hmap_get_value(hmap, "key1");
    ASSERT_TRUE(retrieved == &value);

    spx_hmap_destroy(hmap);
}

TEST(hmap_multiple_entries) {
    spx_hmap_t *hmap = spx_hmap_create(100, string_hash, string_cmp);
    ASSERT_NOT_NULL(hmap);

    int v1 = 1, v2 = 2, v3 = 3;

    spx_hmap_entry_t *e1 = spx_hmap_ensure_entry(hmap, "key1", NULL);
    spx_hmap_entry_t *e2 = spx_hmap_ensure_entry(hmap, "key2", NULL);
    spx_hmap_entry_t *e3 = spx_hmap_ensure_entry(hmap, "key3", NULL);

    spx_hmap_entry_set_value(e1, &v1);
    spx_hmap_entry_set_value(e2, &v2);
    spx_hmap_entry_set_value(e3, &v3);

    void *r1 = spx_hmap_get_value(hmap, "key1");
    void *r2 = spx_hmap_get_value(hmap, "key2");
    void *r3 = spx_hmap_get_value(hmap, "key3");

    ASSERT_TRUE(r1 == &v1 && r2 == &v2 && r3 == &v3);

    spx_hmap_destroy(hmap);
}

TEST(hmap_ensure_entry_new_flag) {
    spx_hmap_t *hmap = spx_hmap_create(100, string_hash, string_cmp);
    ASSERT_NOT_NULL(hmap);

    int new_flag1 = 0, new_flag2 = 0;

    spx_hmap_entry_t *e1 = spx_hmap_ensure_entry(hmap, "key1", &new_flag1);
    ASSERT_NOT_NULL(e1);
    ASSERT_EQ(new_flag1, 1);

    spx_hmap_entry_t *e2 = spx_hmap_ensure_entry(hmap, "key1", &new_flag2);
    ASSERT_NOT_NULL(e2);
    ASSERT_EQ(new_flag2, 0);
    ASSERT_TRUE(e1 == e2);

    spx_hmap_destroy(hmap);
}

TEST(hmap_get_nonexistent) {
    spx_hmap_t *hmap = spx_hmap_create(100, string_hash, string_cmp);
    ASSERT_NOT_NULL(hmap);

    void *value = spx_hmap_get_value(hmap, "nonexistent");
    ASSERT_NULL(value);

    spx_hmap_destroy(hmap);
}

TEST(hmap_update_value) {
    spx_hmap_t *hmap = spx_hmap_create(100, string_hash, string_cmp);
    ASSERT_NOT_NULL(hmap);

    int v1 = 1, v2 = 2;

    spx_hmap_entry_t *entry = spx_hmap_ensure_entry(hmap, "key1", NULL);
    spx_hmap_entry_set_value(entry, &v1);

    void *retrieved = spx_hmap_get_value(hmap, "key1");
    ASSERT_TRUE(retrieved == &v1);

    spx_hmap_entry_set_value(entry, &v2);
    retrieved = spx_hmap_get_value(hmap, "key1");
    ASSERT_TRUE(retrieved == &v2);

    spx_hmap_destroy(hmap);
}

BEGIN_TEST_SUITE(SPX_Hash_Map)
    RUN_TEST(hmap_create_destroy);
    RUN_TEST(hmap_set_and_get);
    RUN_TEST(hmap_multiple_entries);
    RUN_TEST(hmap_ensure_entry_new_flag);
    RUN_TEST(hmap_get_nonexistent);
    RUN_TEST(hmap_update_value);
END_TEST_SUITE()
