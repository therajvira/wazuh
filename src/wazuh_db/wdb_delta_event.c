#include "wdb.h"

bool wdb_single_row_insert_dbsync(wdb_t * wdb, struct kv const *kv_value, const char *data) {
    bool ret_val = false;
    if (NULL != kv_value) {
        char query[OS_SIZE_2048] = { 0 };
        strcat(query, "DELETE FROM ");
        strcat(query, kv_value->value);
        strcat(query, ";");
        sqlite3_stmt *stmt = wdb_get_cache_stmt(wdb, query);

        if (NULL != stmt) {
            ret_val = SQLITE_DONE == wdb_step(stmt) ? true : false;
        } else {
            merror(DB_CACHE_NULL_STMT);
        }
        ret_val = ret_val && wdb_insert_dbsync(wdb, kv_value, data);
    }
    return ret_val;
}

bool wdb_insert_dbsync(wdb_t * wdb, struct kv const *kv_value, const char *data) {
    bool ret_val = false;

    if (NULL != data && NULL != wdb && NULL != kv_value) {
        char query[OS_SIZE_2048] = { 0 };
        strcat(query, "INSERT INTO ");
        strcat(query, kv_value->value);
        strcat(query, " VALUES (");
        struct column_list const *column = NULL;

        for (column = kv_value->column_list; column ; column=column->next) {
            strcat(query, "?");
            if (column->next) {
                strcat(query, ",");
            }
        }
        strcat(query, ");");

        sqlite3_stmt *stmt = wdb_get_cache_stmt(wdb, query);

        if (NULL != stmt) {
            char * data_temp = NULL;
            os_strdup(data, data_temp);
            char * r = NULL;
            char *field_value = strtok_r(data_temp, FIELD_SEPARATOR_DBSYNC, &r);
            for (column = kv_value->column_list; column ; column=column->next) {
                if (column->value.is_old_implementation) {
                    if (SQLITE_OK != sqlite3_bind_int(stmt, column->value.index, 0)) {
                        merror(DB_AGENT_SQL_ERROR, wdb->id, sqlite3_errmsg(wdb->db));
                    }
                } else {
                    if (NULL != field_value) {
                        if (FIELD_TEXT == column->value.type) {
                            if (SQLITE_OK != sqlite3_bind_text(stmt, column->value.index, strcmp(field_value, "NULL") == 0 ? "" : field_value, -1, NULL)) {
                                merror(DB_AGENT_SQL_ERROR, wdb->id, sqlite3_errmsg(wdb->db));
                            }
                        } else {
                            if (SQLITE_OK != sqlite3_bind_int(stmt, column->value.index, strcmp(field_value, "NULL") == 0 ? 0 : atoi(field_value))) {
                                merror(DB_AGENT_SQL_ERROR, wdb->id, sqlite3_errmsg(wdb->db));
                            }
                        }
                        if (column->next) {
                            field_value = strtok_r(NULL, FIELD_SEPARATOR_DBSYNC, &r);
                        }
                    }
                }
            }
            ret_val = SQLITE_DONE == wdb_step(stmt);
            os_free(data_temp);
        } else {
            merror(DB_CACHE_NULL_STMT);
        }
    }
    return ret_val;
}

bool wdb_modify_dbsync(wdb_t * wdb, struct kv const *kv_value, const char *data)
{
    bool ret_val = false;
    if (NULL != data && NULL != wdb && NULL != kv_value) {
        char query[OS_SIZE_2048] = { 0 };
        strcat(query, "UPDATE ");
        strcat(query, kv_value->value);
        strcat(query, " SET ");

        const size_t size = sizeof(char*) * (os_strcnt(data, '|') + 1);
        char ** field_values = NULL;
        os_calloc(1, size + 1, field_values);
        char **curr = field_values;

        char * data_temp = NULL;
        os_strdup(data, data_temp);

        char *r = NULL;
        char *tok = strtok_r(data_temp, FIELD_SEPARATOR_DBSYNC, &r);

        while (NULL != tok) {
            *curr = tok;
            tok = strtok_r(NULL, FIELD_SEPARATOR_DBSYNC, &r);
            ++curr;
        }

        if (curr) {
            *curr = NULL;
        }

        bool first_condition_element = true;
        curr = field_values;
        struct column_list const *column = NULL;
        for (column = kv_value->column_list; column ; column=column->next) {
            if (!column->value.is_old_implementation && curr && NULL != *curr) {
                if (!column->value.is_pk && strcmp(*curr, "NULL") != 0) {
                    if (first_condition_element) {
                        strcat(query, column->value.name);
                        strcat(query, "=?");
                        first_condition_element = false;
                    } else {
                        strcat(query, ",");
                        strcat(query, column->value.name);
                        strcat(query, "=?");
                    }
                }
                ++curr;
            }
        }
        strcat(query, " WHERE ");

        first_condition_element = true;
        for (column = kv_value->column_list; column ; column=column->next) {
            if (column->value.is_pk) {
                if (first_condition_element) {
                    strcat(query, column->value.name);
                    strcat(query, "=?");
                    first_condition_element = false;
                } else {
                    strcat(query, " AND ");
                    strcat(query, column->value.name);
                    strcat(query, "=?");
                }
            }
        }
        strcat(query, ";");

        sqlite3_stmt *stmt = wdb_get_cache_stmt(wdb, query);

        if (NULL != stmt) {
            int index = 1;

            curr = field_values;
            for (column = kv_value->column_list; column ; column=column->next) {
                if (!column->value.is_old_implementation && curr && NULL != *curr) {
                    if (!column->value.is_pk && strcmp(*curr, "NULL") != 0) {
                        if (FIELD_TEXT == column->value.type) {
                            if (SQLITE_OK != sqlite3_bind_text(stmt, index, *curr, -1, NULL)) {
                                merror(DB_AGENT_SQL_ERROR, wdb->id, sqlite3_errmsg(wdb->db));
                            }
                        } else {
                            if (SQLITE_OK != sqlite3_bind_int(stmt, index, atoi(*curr))) {
                                merror(DB_AGENT_SQL_ERROR, wdb->id, sqlite3_errmsg(wdb->db));
                            }
                        }
                        ++index;
                    }
                    ++curr;
                }
            }

            curr = field_values;
            for (column = kv_value->column_list; column ; column=column->next) {
                if (!column->value.is_old_implementation && curr && NULL != *curr) {
                    if (column->value.is_pk && strcmp(*curr, "NULL") != 0) {
                        if (FIELD_TEXT == column->value.type) {
                            if (SQLITE_OK != sqlite3_bind_text(stmt, index, *curr, -1, NULL)) {
                                merror(DB_AGENT_SQL_ERROR, wdb->id, sqlite3_errmsg(wdb->db));
                            }
                        } else {
                            if (SQLITE_OK != sqlite3_bind_int(stmt, index, atoi(*curr))) {
                                merror(DB_AGENT_SQL_ERROR, wdb->id, sqlite3_errmsg(wdb->db));
                            }
                        }
                        ++index;
                    }
                    ++curr;
                }
            }
            ret_val = SQLITE_DONE == wdb_step(stmt);
        } else {
            merror(DB_CACHE_NULL_STMT);
        }
        os_free(data_temp);
        os_free(field_values);
    }
    return ret_val;
}

bool wdb_delete_dbsync(wdb_t * wdb, struct kv const *kv_value, const char *data)
{
    bool ret_val = false;
    if (NULL != wdb && NULL != kv_value && NULL != data) {
        char query[OS_SIZE_2048] = { 0 };
        strcat(query, "DELETE FROM ");
        strcat(query, kv_value->value);
        strcat(query, " WHERE ");

        bool first_condition_element = true;
        struct column_list const *column = NULL;
        for (column = kv_value->column_list; column ; column=column->next) {
            if (!column->value.is_old_implementation) {
                if (column->value.is_pk) {
                    if (first_condition_element) {
                        strcat(query, column->value.name);
                        strcat(query, "=?");
                        first_condition_element = false;
                    } else {
                        strcat(query, " AND ");
                        strcat(query, column->value.name);
                        strcat(query, "=?");
                    }
                }
            }
        }
        strcat(query, ";");

        sqlite3_stmt *stmt = wdb_get_cache_stmt(wdb, query);

        if (NULL != stmt) {
            char *data_temp = NULL;
            os_strdup(data, data_temp);
            char * r = NULL;
            char *field_value = strtok_r(data_temp, FIELD_SEPARATOR_DBSYNC, &r);

            struct column_list const *column = NULL;
            int index = 1;
            for (column = kv_value->column_list; column ; column=column->next) {
                if (!column->value.is_old_implementation) {
                    if (NULL != field_value) {
                        if (column->value.is_pk) {
                            if (FIELD_TEXT == column->value.type) {
                                if (SQLITE_OK != sqlite3_bind_text(stmt, index, strcmp(field_value, "NULL") == 0 ? "" : field_value, -1, NULL)) {
                                    merror(DB_AGENT_SQL_ERROR, wdb->id, sqlite3_errmsg(wdb->db));
                                }
                            } else {
                                if (SQLITE_OK != sqlite3_bind_int(stmt, index, strcmp(field_value, "NULL") == 0 ? 0 : atoi(field_value))) {
                                    merror(DB_AGENT_SQL_ERROR, wdb->id, sqlite3_errmsg(wdb->db));
                                }
                            }
                            ++index;
                        }
                        if (column->next) {
                            field_value = strtok_r(NULL, FIELD_SEPARATOR_DBSYNC, &r);
                        }
                    }
                }
            }
            ret_val = SQLITE_DONE == wdb_step(stmt);
            os_free(data_temp);
        } else {
            merror(DB_CACHE_NULL_STMT);
        }
    }
    return ret_val;
}


void wdb_select_dbsync(wdb_t * wdb, struct kv const *kv_value, const char *data, char *output)
{
    if (NULL != wdb && NULL != data) {
        char query[OS_SIZE_2048] = { 0 };
        bool first_condition_element = true;
        struct column_list const *column = NULL;

        strcat(query, "SELECT ");
        for (column = kv_value->column_list; column ; column=column->next) {
            if (!column->value.is_old_implementation) {
                if (first_condition_element) {
                    first_condition_element = false;
                } else {
                    strcat(query, ", ");
                }
                strcat(query, column->value.name);
            }
        }
        strcat(query, " FROM ");
        strcat(query, kv_value->value);
        strcat(query, " WHERE ");

        first_condition_element = true;
        for (column = kv_value->column_list; column ; column=column->next) {
            if (!column->value.is_old_implementation) {
                if (column->value.is_pk) {
                    if (first_condition_element) {
                        strcat(query, column->value.name);
                        strcat(query, "=?");
                        first_condition_element = false;
                    } else {
                        strcat(query, " AND ");
                        strcat(query, column->value.name);
                        strcat(query, "=?");
                    }
                }
            }
        }
        strcat(query, ";");
        sqlite3_stmt *stmt = wdb_get_cache_stmt(wdb, query);

        if (NULL != stmt) {
            char * data_temp = NULL;
            os_strdup(data, data_temp);
            char * r = NULL;
            char *field_value = strtok_r(data_temp, FIELD_SEPARATOR_DBSYNC, &r);

            struct column_list const *column = NULL;
            int index = 1;
            for (column = kv_value->column_list; column ; column=column->next) {
                if (!column->value.is_old_implementation) {
                    if (NULL != field_value) {
                        if (column->value.is_pk) {
                            if (FIELD_TEXT == column->value.type) {
                                if (SQLITE_OK != sqlite3_bind_text(stmt, index, strcmp(field_value, "NULL") == 0 ? "" : field_value, -1, NULL)) {
                                    merror(DB_AGENT_SQL_ERROR, wdb->id, sqlite3_errmsg(wdb->db));
                                }
                            } else {
                                if (SQLITE_OK != sqlite3_bind_int(stmt, index, strcmp(field_value, "NULL") == 0 ? 0 : atoi(field_value))) {
                                    merror(DB_AGENT_SQL_ERROR, wdb->id, sqlite3_errmsg(wdb->db));
                                }
                            }
                            ++index;
                        }
                        if (column->next) {
                            field_value = strtok_r(NULL, FIELD_SEPARATOR_DBSYNC, &r);
                        }
                    }
                }
            }
            index = 0;
            int len = strlen(output);
            switch (wdb_step(stmt)) {
                case SQLITE_ROW:
                    for (column = kv_value->column_list; column ; column=column->next) {
                        if (!column->value.is_old_implementation) {
                            if (FIELD_TEXT == column->value.type) {
                                len += snprintf(output + len, OS_SIZE_6144 - len - WDB_RESPONSE_OK_SIZE - 1, "%s", (char *)sqlite3_column_text(stmt, index));
                            }
                            else if (FIELD_INTEGER == column->value.type) {
                                len += snprintf(output + len, OS_SIZE_6144 - len - WDB_RESPONSE_OK_SIZE - 1, "%d", sqlite3_column_int(stmt, index));
                            }
                            else if (FIELD_REAL == column->value.type) {
                                len += snprintf(output + len, OS_SIZE_6144 - len - WDB_RESPONSE_OK_SIZE - 1, "%f", sqlite3_column_double(stmt, index));
                            }
                            len += snprintf(output + len, OS_SIZE_6144 - len - WDB_RESPONSE_OK_SIZE - 1, FIELD_SEPARATOR_DBSYNC);
                            ++index;
                        }
                    }
                    break;
                case SQLITE_DONE:
                    break;
                default:
                    merror(DB_AGENT_SQL_ERROR, wdb->id, sqlite3_errmsg(wdb->db));
                    break;
            }
            os_free(data_temp);
        } else {
            merror(DB_CACHE_NULL_STMT);
        }
    }
}

