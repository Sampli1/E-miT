#include "nvs_flash.h"

#include "calendar.h"
#include "spiffs_handler.h"
#include "utils.h"
#include "server_utils.h"
#include "client.h"

static const char *TAG = "CALENDAR";

esp_err_t get_from_nvs(nvs_handle_t nvs_handler, char *key, char **val, size_t *total_size) {
    size_t required_size;
    esp_err_t err;
    err = nvs_get_str(nvs_handler, key, NULL, &required_size);
    ESP_LOGI(TAG, "required size: %d", required_size);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        (*val) = (char *) malloc(10 * sizeof(char));
        sprintf(*val, "NULL");
    } 
    if (err == ESP_OK) {
        (*val) = (char *) malloc(required_size * sizeof(char));
        *total_size += required_size;
        err = nvs_get_str(nvs_handler, key, *val, &required_size);
        ESP_LOGI(TAG, "%s: %s", key, *val);
    }
    return err;
}

void decompose_calendar_names(char *calendar_response, char *calendar_names) {
    char *items = calloc(MAX_HTTP_OUTPUT_BUFFER, sizeof(char));
    char *title = calloc(1024, sizeof(char));
    char *id = calloc(1024, sizeof(char));
    char *buffer = calloc(1024* 5, sizeof(char));
    char *json_vec[20];
    for (int i= 0; i < 20; i++) json_vec[i] =  calloc(1024 , sizeof(char));
    int length = 0;

    decompose_json_dynamic_params(calendar_response,1, "items", items);
    from_string_to_json_string_vec(items, json_vec, &length);


    for (int i = 0; i < length; i++) {
        decompose_json_dynamic_params(json_vec[i], 2, "id", id, "summary", title);
        ESP_LOGI(TAG, "Title: %s, Id: %s", title, id);

        sprintf(buffer, "%s{\"title\": \"%s\", \"id\": \"%s\"}", (i > 0 ? "," : ""), title, id);

        strcat(calendar_names, buffer);
    }

    free(title);
    free(items);
    free(id);
    free(buffer);
    for (int i = 0; i < 20; i++) free(json_vec[i]);
}

static esp_err_t get_calendar_handler(httpd_req_t *req) {
    nvs_handle_t NVS;
    nvs_open("oauth2_tokens", NVS_READWRITE, &NVS);

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
                ESP_LOGI(TAG, "Found URL query parameter => code=%s", param);
                example_uri_decode(name, param, strnlen(param, HTTP_QUERY_KEY_MAX_LEN));
                ESP_LOGI(TAG, "Decoded query parameter => %s", name);
            }
            if (httpd_query_key_value(buf, "id", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(TAG, "Found URL query parameter => code=%s", param);
                example_uri_decode(id, param, strnlen(param, HTTP_QUERY_KEY_MAX_LEN));
                ESP_LOGI(TAG, "Decoded query parameter => %s", name);
            }
        }
        free(buf);
    }


    // Get access_token from id
    char at_key[15] = {0}, rt_key[15] = {0};
    ESP_LOGI(TAG, "ID %d", atoi(id));
    sprintf(at_key, "user_%d_at", atoi(id));
    sprintf(rt_key, "user_%d_rt", atoi(id));
    
    // Get list of calendars
    int headers_length = 2;
    char *headers_keys[2] = {"Content-Type", "Authorization"};
    char *headers_values[2];
    headers_values[0] = "application/json";
    headers_values[1] = calloc(MAX_POST_BODY_LENGTH, sizeof(char));
    if (headers_values[1] == NULL) {
        ESP_LOGE(TAG, "OUT OF MEMORY");
        return ESP_FAIL;  
    }
    char *response = calloc(MAX_HTTP_OUTPUT_BUFFER, sizeof(char)), *access_token;
    
    size_t total_size = 0;

    get_from_nvs(NVS, at_key, &access_token, &total_size);

    if (access_token == NULL || strlen(access_token) <= 1) {
        free(access_token);
        free(response);
        return ESP_FAIL;
    }

    sprintf(headers_values[1], "Bearer %s", access_token);

    if (xSemaphoreTake(client_http_mutex, portMAX_DELAY) == pdTRUE) {
        if (!get_api(response, CALENDAR_LIST_LINK, client_http, headers_keys, headers_values, 2)) {
            free(access_token);
            free(response);
            nvs_close(NVS);
            httpd_resp_send_404(req);
            xSemaphoreGive(client_http_mutex); // <=== I'm a retard
            return ESP_FAIL;
        }
        xSemaphoreGive(client_http_mutex);
    }

    ESP_LOGI(TAG, "CALENDARS: %s", response);   

    
    char *calendar_names = calloc(1024 * 20 + 20, sizeof(char));

    strcat(calendar_names, "[");
    decompose_calendar_names(response, calendar_names);
    strcat(calendar_names, "]");


    httpd_resp_send(req, calendar_names, HTTPD_RESP_USE_STRLEN);

    free(calendar_names);
    free(access_token);
    free(response);
    nvs_close(NVS);
    return ESP_OK;
}

static esp_err_t set_calendar_handler(httpd_req_t *req) {
    char content[50];
    int ret = httpd_req_recv(req, content, 50);


    if (ret <= 0) {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            httpd_resp_send_408(req);
        }
        return ESP_FAIL;
    }
    

    const char* resp_str = (const char*) req->user_ctx;
    httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);
    httpd_resp_send_chunk(req, NULL, 0);
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