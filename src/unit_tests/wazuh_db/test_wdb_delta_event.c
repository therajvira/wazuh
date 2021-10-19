/*
 * Copyright (C) 2015-2021, Wazuh Inc.
 * March, 2021.
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 2) as published by the FSF - Free Software
 * Foundation.
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdio.h>
#include <string.h>

#include "wazuh_db/wdb.h"
#include "../wrappers/wazuh/shared/debug_op_wrappers.h"
#include "../wrappers/externals/sqlite/sqlite3_wrappers.h"
#include "../wrappers/wazuh/wazuh_db/wdb_wrappers.h"
#include "wazuhdb_op.h"

/*
* Dummy tables information
*/
static struct column_list const TABLE_NETIFACE_TEST[] = {
    { .value = { FIELD_INTEGER, 1, true, false, "test_1" }, .next = &TABLE_NETIFACE_TEST[1] } ,
    { .value = { FIELD_TEXT, 2, false, false, "test_2" }, .next = NULL } ,
};

static struct column_list const TABLE_NETPROTO_TEST[] = {
    { .value = { FIELD_INTEGER, 1, true, false, "test_1" }, .next = &TABLE_NETPROTO_TEST[1] } ,
    { .value = { FIELD_TEXT, 2, false, false, "test_2" }, .next = NULL } ,
};

static struct column_list const TABLE_NETADDR_TEST[] = {
    { .value = { FIELD_INTEGER, 1, true, false, "test_1" }, .next = &TABLE_NETADDR_TEST[1] } ,
    { .value = { FIELD_TEXT, 2, false, false, "test_2" }, .next = NULL } ,
};

static struct column_list const TABLE_OS_TEST[] = {
    { .value = { FIELD_INTEGER, 1, true, false, "test_1" }, .next = &TABLE_OS_TEST[1] } ,
    { .value = { FIELD_TEXT, 2, false, false, "test_2" }, .next = NULL } ,
};

static struct column_list const TABLE_HARDWARE_TEST[] = {
    { .value = { FIELD_INTEGER, 1, true, false, "test_1" }, .next = &TABLE_HARDWARE_TEST[1] } ,
    { .value = { FIELD_TEXT, 2, false, false, "test_2" }, .next = NULL } ,
};

static struct column_list const TABLE_PORTS_TEST[] = {
    { .value = { FIELD_INTEGER, 1, true, false, "test_1" }, .next = &TABLE_PORTS_TEST[1] } ,
    { .value = { FIELD_TEXT, 2, false, false, "test_2" }, .next = NULL } ,
};

static struct column_list const TABLE_PACKAGES_TEST[] = {
    { .value = { FIELD_INTEGER, 1, true, false, "test_1" }, .next = &TABLE_PACKAGES_TEST[1] } ,
    { .value = { FIELD_TEXT, 2, false, false, "test_2" }, .next = NULL } ,
};

static struct column_list const TABLE_PROCESSES_TEST[] = {
    { .value = { FIELD_INTEGER, 1, true, false, "test_1" }, .next = &TABLE_PROCESSES_TEST[1] } ,
    { .value = { FIELD_TEXT, 2, false, false, "test_2" }, .next = NULL } ,
};

static struct kv_list const TABLE_MAP_TEST[] = {
    { .current = { "network_iface", "sys_netiface", false, TABLE_NETIFACE_TEST }, .next = &TABLE_MAP_TEST[1]},
    { .current = { "network_protocol", "sys_netproto", false, TABLE_NETPROTO_TEST }, .next = &TABLE_MAP_TEST[2]},
    { .current = { "network_address", "sys_netaddr", false, TABLE_NETADDR_TEST }, .next = &TABLE_MAP_TEST[3]},
    { .current = { "osinfo", "sys_osinfo", false, TABLE_OS_TEST }, .next = &TABLE_MAP_TEST[4]},
    { .current = { "hwinfo", "sys_hwinfo", false, TABLE_HARDWARE_TEST }, .next = &TABLE_MAP_TEST[5]},
    { .current = { "ports", "sys_ports", false, TABLE_PORTS_TEST }, .next = &TABLE_MAP_TEST[6]},
    { .current = { "packages", "sys_programs", false, TABLE_PACKAGES_TEST }, .next = &TABLE_MAP_TEST[7]},
    { .current = { "processes", "sys_processes",  false, TABLE_PROCESSES_TEST}, .next = NULL},
};

typedef struct test_struct {
    wdb_t *wdb;
    char *output;
} test_struct_t;

static int test_setup(void **state) {
    test_struct_t *init_data = NULL;
    os_calloc(1,sizeof(test_struct_t),init_data);
    os_calloc(1,sizeof(wdb_t),init_data->wdb);
    os_strdup("global",init_data->wdb->id);
    os_calloc(256,sizeof(char),init_data->output);
    os_calloc(1,sizeof(sqlite3 *),init_data->wdb->db);
    *state = init_data;
    return 0;
}

static int test_teardown(void **state){
    test_struct_t *data  = (test_struct_t *)*state;
    os_free(data->output);
    os_free(data->wdb->id);
    os_free(data->wdb->db);
    os_free(data->wdb);
    os_free(data);
    return 0;
}

//
// wdb_single_row_insert_dbsync
//

void test_wdb_single_row_insert_dbsync_err(void **state)
{
    test_struct_t *data  = (test_struct_t *)*state;
    assert_false(wdb_single_row_insert_dbsync(data->wdb, NULL, "something"));
}

void test_wdb_single_row_insert_dbsync_get_cache_stmt_fail(void **state)
{
    struct kv_list const *head = TABLE_MAP_TEST;
    will_return(__wrap_wdb_get_cache_stmt, NULL);
    expect_string(__wrap__merror, formatted_msg, DB_CACHE_NULL_STMT);
    assert_false(wdb_single_row_insert_dbsync(NULL, &head->current, "something"));
}

void test_wdb_single_row_insert_dbsync_get_cache_stmt_bad_query(void **state)
{
    test_struct_t *data  = (test_struct_t *)*state;
    struct kv_list const *head = TABLE_MAP_TEST;
    will_return(__wrap_wdb_get_cache_stmt, NULL);
    expect_string(__wrap__merror, formatted_msg, DB_CACHE_NULL_STMT);
    assert_false(wdb_single_row_insert_dbsync(data->wdb, &head->current, "bad_query"));
}

void test_wdb_single_row_insert_dbsync_get_cache_stmt_null(void **state)
{
    test_struct_t *data  = (test_struct_t *)*state;
    struct kv_list const *head = TABLE_MAP_TEST;
    will_return(__wrap_wdb_get_cache_stmt, 1);
    will_return(__wrap_wdb_step, SQLITE_DONE);
    will_return(__wrap_wdb_get_cache_stmt, 0);
    expect_string(__wrap__merror, formatted_msg, DB_CACHE_NULL_STMT);
    assert_false(wdb_single_row_insert_dbsync(data->wdb, &head->current, "good query"));
}

void test_wdb_single_row_insert_dbsync_get_cache_stmt_not_null(void **state)
{
    test_struct_t *data  = (test_struct_t *)*state;
    struct kv_list const *head = TABLE_MAP_TEST;
    will_return(__wrap_wdb_get_cache_stmt, 1);
    will_return(__wrap_wdb_step, SQLITE_DONE);

    will_return(__wrap_wdb_get_cache_stmt, 1);
    expect_value(__wrap_sqlite3_bind_int, index, 1);
    expect_value(__wrap_sqlite3_bind_int, value, 0);
    will_return_always(__wrap_sqlite3_bind_int, SQLITE_OK);

    expect_value(__wrap_sqlite3_bind_text, pos, 2);
    expect_string(__wrap_sqlite3_bind_text, buffer, "data");
    will_return_always(__wrap_sqlite3_bind_text, SQLITE_OK);

    will_return(__wrap_wdb_step, SQLITE_DONE);

    assert_true(wdb_single_row_insert_dbsync(data->wdb, &head->current, "data|1"));
}


//
// wdb_insert_dbsync
//

void test_wdb_insert_dbsync_err(void **state)
{
    test_struct_t *data  = (test_struct_t *)*state;
    struct kv_list const *head = TABLE_MAP_TEST;
    assert_false(wdb_insert_dbsync(NULL, &head->current, "something"));
    assert_false(wdb_insert_dbsync(data->wdb, NULL, "something"));
    assert_false(wdb_insert_dbsync(data->wdb, &head->current, NULL));
}

void test_wdb_insert_dbsync_bad_cache(void **state)
{
    test_struct_t *data  = (test_struct_t *)*state;
    struct kv_list const *head = TABLE_MAP_TEST;
    will_return(__wrap_wdb_get_cache_stmt, NULL);
    expect_string(__wrap__merror, formatted_msg, DB_CACHE_NULL_STMT);
    assert_false(wdb_insert_dbsync(data->wdb, &head->current, "something"));
}

void test_wdb_insert_dbsync_bind_fail(void **state)
{
    test_struct_t *data  = (test_struct_t *)*state;
    struct kv_list const *head = TABLE_MAP_TEST;

    will_return(__wrap_wdb_get_cache_stmt, 1);
    expect_value(__wrap_sqlite3_bind_int, index, 1);
    expect_value(__wrap_sqlite3_bind_int, value, 0);
    will_return_always(__wrap_sqlite3_bind_int, SQLITE_ERROR);

    const char error_value[] = { "bad parameter or other API misuse" };
    char error_message[128] = { "\0" };
    sprintf(error_message, DB_AGENT_SQL_ERROR, "global", error_value);

    expect_string(__wrap__merror, formatted_msg, error_message);

    expect_value(__wrap_sqlite3_bind_text, pos, 2);
    expect_string(__wrap_sqlite3_bind_text, buffer, "data");
    will_return_always(__wrap_sqlite3_bind_text, SQLITE_ERROR);
    expect_string(__wrap__merror, formatted_msg, error_message);

    will_return(__wrap_wdb_step, SQLITE_ERROR);

    assert_false(wdb_insert_dbsync(data->wdb, &head->current, "data|1"));
}

void test_wdb_insert_dbsync_ok(void **state)
{
    test_struct_t *data  = (test_struct_t *)*state;
    struct kv_list const *head = TABLE_MAP_TEST;

    will_return(__wrap_wdb_get_cache_stmt, 1);
    expect_value(__wrap_sqlite3_bind_int, index, 1);
    expect_value(__wrap_sqlite3_bind_int, value, 0);
    will_return_always(__wrap_sqlite3_bind_int, SQLITE_OK);

    expect_value(__wrap_sqlite3_bind_text, pos, 2);
    expect_string(__wrap_sqlite3_bind_text, buffer, "data");
    will_return_always(__wrap_sqlite3_bind_text, SQLITE_OK);

    will_return(__wrap_wdb_step, SQLITE_DONE);

    assert_true(wdb_insert_dbsync(data->wdb, &head->current, "data|1"));
}

//
// wdb_modify_dbsync
//

void test_wdb_modify_dbsync_err(void **state)
{
    test_struct_t *data  = (test_struct_t *)*state;
    struct kv_list const *head = TABLE_MAP_TEST;
    assert_false(wdb_modify_dbsync(NULL, &head->current, "something"));
    assert_false(wdb_modify_dbsync(data->wdb, NULL, "something"));
    assert_false(wdb_modify_dbsync(data->wdb, &head->current, NULL));
}

void test_wdb_modify_dbsync_bad_cache(void **state)
{
    test_struct_t *data  = (test_struct_t *)*state;
    struct kv_list const *head = TABLE_MAP_TEST;
    will_return(__wrap_wdb_get_cache_stmt, NULL);
    expect_string(__wrap__merror, formatted_msg, DB_CACHE_NULL_STMT);
    assert_false(wdb_modify_dbsync(data->wdb, &head->current, "something"));
}

void test_wdb_modify_dbsync_step_nok(void **state)
{
    test_struct_t *data  = (test_struct_t *)*state;
    struct kv_list const *head = TABLE_MAP_TEST;
    will_return(__wrap_wdb_get_cache_stmt, 1);

    expect_value(__wrap_sqlite3_bind_text, pos, 1);
    expect_string(__wrap_sqlite3_bind_text, buffer, "data1");
    will_return_always(__wrap_sqlite3_bind_text, SQLITE_OK);
    will_return(__wrap_wdb_step, SQLITE_ERROR);

    assert_false(wdb_modify_dbsync(data->wdb, &head->current, "data1|1|data2|2"));
}

void test_wdb_modify_dbsync_ok(void **state)
{
    test_struct_t *data  = (test_struct_t *)*state;
    struct kv_list const *head = TABLE_MAP_TEST;
    will_return(__wrap_wdb_get_cache_stmt, 1);

    expect_value(__wrap_sqlite3_bind_text, pos, 1);
    expect_string(__wrap_sqlite3_bind_text, buffer, "data1");
    will_return_always(__wrap_sqlite3_bind_text, SQLITE_OK);
    will_return(__wrap_wdb_step, SQLITE_DONE);

    assert_true(wdb_modify_dbsync(data->wdb, &head->current, "data1|1|data2|2"));
}

//
// wdb_delete_dbsync
//

void test_wdb_delete_dbsync_err(void **state)
{
    test_struct_t *data  = (test_struct_t *)*state;
    struct kv_list const *head = TABLE_MAP_TEST;
    assert_false(wdb_delete_dbsync(NULL, &head->current, "something"));
    assert_false(wdb_delete_dbsync(data->wdb, NULL, "something"));
    assert_false(wdb_delete_dbsync(data->wdb, &head->current, NULL));
}

void test_wdb_delete_dbsync_bad_cache(void **state)
{
    test_struct_t *data  = (test_struct_t *)*state;
    struct kv_list const *head = TABLE_MAP_TEST;
    will_return(__wrap_wdb_get_cache_stmt, NULL);
    expect_string(__wrap__merror, formatted_msg, DB_CACHE_NULL_STMT);
    assert_false(wdb_delete_dbsync(data->wdb, &head->current, "something"));
}

void test_wdb_delete_dbsync_step_nok(void **state)
{
    test_struct_t *data  = (test_struct_t *)*state;
    struct kv_list const *head = TABLE_MAP_TEST;
    will_return(__wrap_wdb_get_cache_stmt, 1);
    will_return(__wrap_wdb_step, SQLITE_ERROR);
    assert_false(wdb_delete_dbsync(data->wdb, &head->current, "data|1"));
}

void test_wdb_delete_dbsync_ok(void **state)
{
    test_struct_t *data  = (test_struct_t *)*state;
    struct kv_list const *head = TABLE_MAP_TEST;
    will_return(__wrap_wdb_get_cache_stmt, 1);
    will_return(__wrap_wdb_step, SQLITE_DONE);
    assert_true(wdb_delete_dbsync(data->wdb, &head->current, "test_1"));
}

int main()
{
    const struct CMUnitTest tests[] = {
        /* wdb_single_row_insert_dbsync */
        cmocka_unit_test_setup_teardown(test_wdb_single_row_insert_dbsync_err, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_wdb_single_row_insert_dbsync_get_cache_stmt_fail, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_wdb_single_row_insert_dbsync_get_cache_stmt_bad_query, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_wdb_single_row_insert_dbsync_get_cache_stmt_null, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_wdb_single_row_insert_dbsync_get_cache_stmt_not_null, test_setup, test_teardown),
        /* wdb_insert_dbsync */
        cmocka_unit_test_setup_teardown(test_wdb_insert_dbsync_err, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_wdb_insert_dbsync_bad_cache, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_wdb_insert_dbsync_bind_fail, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_wdb_insert_dbsync_ok, test_setup, test_teardown),
        /* wdb_modify_dbsync */
        cmocka_unit_test_setup_teardown(test_wdb_modify_dbsync_err, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_wdb_modify_dbsync_bad_cache, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_wdb_modify_dbsync_step_nok, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_wdb_modify_dbsync_ok, test_setup, test_teardown),
        /* wdb_delete_dbsync */
        cmocka_unit_test_setup_teardown(test_wdb_delete_dbsync_err, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_wdb_delete_dbsync_bad_cache, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_wdb_delete_dbsync_step_nok, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_wdb_delete_dbsync_ok, test_setup, test_teardown),

    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
