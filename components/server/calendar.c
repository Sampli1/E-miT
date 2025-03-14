#include "nvs_flash.h"

#include "calendar.h"
#include "spiffs_handler.h"
#include "utils.h"
#include "nvs_utils.h"

#include "server_utils.h"
#include "client.h"
#include "oauth2.h"

static const char *TAG = "CALENDAR";

int count_elements(const char *input) {
    int count = 1;
    for (int i = 0; input[i] != '\0'; i++) {
        if (input[i] == ',') {
            count++;
        }
    }
    return count;
}


void decompose_calendar_names(nvs_handle_t NVS, int id_val, char *calendar_response, char *calendar_names) {
    char *items = calloc(MAX_HTTP_OUTPUT_BUFFER, sizeof(char));
    char *title = calloc(1024, sizeof(char));
    char *id = calloc(1024, sizeof(char));
    char *buffer = calloc(1024 * 5, sizeof(char));
    char **ids_enabled = NULL;
    char *json_vec[20];
    for (int i= 0; i < 20; i++) json_vec[i] = calloc(1024 , sizeof(char));
    
    size_t length = 0;
    int enabled_length = 0;
    int items_length = 0;


    // Get enabled calendars for that user
    char key[15] = {0}, *val;
    sprintf(key, "user_%d_ids", id_val);
    if (get_from_nvs(NVS, key, &val, &length) == ESP_OK) {
        from_string_to_string_array(val, &ids_enabled, &enabled_length);
        ESP_LOGI(TAG, "ENABLED CALENDARS:");
        for (int i = 0; i < enabled_length; i++) ESP_LOGI(TAG, "%s", ids_enabled[i]);
    }
    else {
        ESP_LOGI(TAG, "NO ENABLED CALENDARS FOR THIS USER!");
    }

    // Get calendars and pass to frontend enabled calendars and list of them
    decompose_json_dynamic_params(calendar_response, 1, "items", items);
    from_string_to_json_string_vec(items, json_vec, &items_length);
    int enabled;
    for (int i = 0; i < items_length; i++) {
        enabled = 0;
        decompose_json_dynamic_params(json_vec[i], 2, "id", id, "summary", title);

        for (int j = 0; j < enabled_length; j++) if (strcmp(id, ids_enabled[j]) == 0) enabled = 1;

        sprintf(buffer, "%s{\"title\": \"%s\", \"id\": \"%s\", \"enabled\": \"%d\"}", (i > 0 ? "," : ""), title, id, enabled);
        
        ESP_LOGI(TAG, "Title: %s, Id: %s, Enabled: %d", title, id, enabled);
        strcat(calendar_names, buffer);

    }

    if (ids_enabled != NULL) free_string_array(ids_enabled, enabled_length);
    free(title);
    free(items);
    free(id);
    free(buffer);
    free(val);
    for (int i = 0; i < 20; i++) free(json_vec[i]);
}

static esp_err_t get_calendar_handler(httpd_req_t *req) {
    nvs_handle_t NVS;
    nvs_open("general_data", NVS_READWRITE, &NVS);

    char*  buf;
    size_t buf_len;

    buf_len = httpd_req_get_url_query_len(req) + 1;
    char name[HTTP_QUERY_KEY_MAX_LEN] = {0}, param[HTTP_QUERY_KEY_MAX_LEN] = {0}, id[3] = {0};

    if (buf_len > 1) {
        buf = malloc(buf_len);
        ESP_RETURN_ON_FALSE(buf, ESP_ERR_NO_MEM, TAG, "buffer alloc failed");
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
            ESP_LOGI(TAG, "Found URL query => %s", buf);
            /* Get value of expected key from query string */
            if (httpd_query_key_value(buf, "name", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(TAG, "Found URL query parameter => name=%s", param);
                example_uri_decode(name, param, strnlen(param, HTTP_QUERY_KEY_MAX_LEN));
                ESP_LOGI(TAG, "Decoded query parameter => %s", name);
            }
            if (httpd_query_key_value(buf, "id", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(TAG, "Found URL query parameter => id=%s", param);
                example_uri_decode(id, param, strnlen(param, HTTP_QUERY_KEY_MAX_LEN));
                ESP_LOGI(TAG, "Decoded query parameter => %s", name);
            }
        }
        free(buf);
    }

 
    // Get list of calendars
    char *response = calloc(MAX_HTTP_OUTPUT_BUFFER, sizeof(char));

    if (get_api_oauth2(response, MAX_HTTP_OUTPUT_BUFFER, CALENDAR_LIST_LINK, NVS, atoi(id)) != ESP_OK) {
        free(response);
        httpd_resp_send_404(req);
        nvs_close(NVS);
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "CALENDARS: %s", response);   

    char *calendar_names = calloc(1024 * 20 + 20, sizeof(char));

    strcat(calendar_names, "[");
    decompose_calendar_names(NVS, atoi(id), response, calendar_names);
    strcat(calendar_names, "]");


    httpd_resp_send(req, calendar_names, HTTPD_RESP_USE_STRLEN);

    free(calendar_names);
    free(response);
    nvs_close(NVS);
    return ESP_OK;
}

static esp_err_t set_calendar_handler(httpd_req_t *req) {
    char*  buf;
    size_t buf_len;

    buf_len = httpd_req_get_url_query_len(req) + 1;
    char param[HTTP_QUERY_KEY_MAX_LEN] = {0}, user_id[3] = {0};

    if (buf_len > 1) {
        buf = malloc(buf_len);
        ESP_RETURN_ON_FALSE(buf, ESP_ERR_NO_MEM, TAG, "buffer alloc failed");
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
            ESP_LOGI(TAG, "Found URL query => %s", buf);
            /* Get value of expected key from query string */
            if (httpd_query_key_value(buf, "user_id", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(TAG, "Found URL query parameter => code=%s", param);
                example_uri_decode(user_id, param, strnlen(param, HTTP_QUERY_KEY_MAX_LEN));
                ESP_LOGI(TAG, "Decoded query parameter => %s", user_id);
            }
        }
        free(buf);
    }
        
    if (strlen(user_id) == 0) {
        httpd_resp_send_custom_err(req, "400", "User empty");
        return ESP_FAIL;
    } 

    
    nvs_handle_t NVS;
    nvs_open("general_data", NVS_READWRITE, &NVS);

    // Get enabled calendars for that user
    size_t buffer_size = MAX_CALENDAR_ID_SIZE * 5 + 2 + 8 + 2;
    char *buffer = calloc(buffer_size, sizeof(char)); // + 2 ('[', ']'), + 8 (', ' in front of element) + 2 ('\0')
    char key[15] = {0};

    sprintf(key, "user_%d_ids", atoi(user_id));
        
    // Set new calendar string
    int ret = httpd_req_recv(req, buffer, buffer_size);

    if (ret <= 0) {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) httpd_resp_send_408(req);
        free(buffer);
        nvs_close(NVS);
        return ESP_FAIL;
    }

    int length = count_elements(buffer);

    if (length > MAX_CALENDARS_EPAPER) {
        httpd_resp_send_custom_err(req, "400", "Reached maximum calendars in the E-Paper");
        free(buffer);
        nvs_close(NVS);
        return ESP_FAIL;
    }


    nvs_set_str(NVS, key, buffer);
    nvs_commit(NVS);

    ESP_LOGI(TAG, "WRITE ON NVS {%s: %s}", key, buffer);

    httpd_resp_set_status(req, "200");
    httpd_resp_send(req, buffer, HTTPD_RESP_USE_STRLEN);
    httpd_resp_send_chunk(req, NULL, 0);
    

    free(buffer);
    nvs_close(NVS);
    return ESP_OK;
}

static esp_err_t user_validity_handler(httpd_req_t *req) {
    const char* resp_str = (const char*) req->user_ctx;
    httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);
    httpd_resp_send_chunk(req, NULL, 0);

    return ESP_OK;
}

const httpd_uri_t get_calendar = {
    .uri = "/get_calendar",
    .method = HTTP_GET,
    .handler   = get_calendar_handler,
    .user_ctx  = "OK!"
};


const httpd_uri_t set_calendar = {
    .uri = "/set_calendar",
    .method = HTTP_POST,
    .handler   = set_calendar_handler,
    .user_ctx  = "OK!"
};



const httpd_uri_t user_validity = {
    .uri = "/check_user",
    .method = HTTP_GET,
    .handler   = user_validity_handler,
    .user_ctx  = "OK!"
};