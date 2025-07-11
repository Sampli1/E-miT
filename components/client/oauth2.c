#include "nvs.h"
#include "esp_log.h"
#include "oauth2.h"

static const char *TAG = "OAUTH2";

// ! NO THREAD SAFE
int refresh_token_managment(int id, char *response) {
 
    ESP_LOGI(TAG, "ASK REFRESH TOKEN FOR USER_%d", id);
    
    char *error = calloc(1000, sizeof(char));
    char *status = calloc(100, sizeof(char));

    
    if (!error || !status) {
        ESP_LOGE(TAG, "Out of memory");
        free(error);
        free(status);
        return 0;
    }

    decompose_json_dynamic_params(response, 1, "error", error);
    decompose_json_dynamic_params(error, 1, "status", status);


    if (strcmp(status, "UNAUTHENTICATED") != 0) {
        free(error);
        free(status);
        return 0;
    }
   

    // Get refresh token from NVS
    nvs_handle_t NVS;
    if (nvs_open("general_data", NVS_READWRITE, &NVS) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS");
        free(error);
        free(status);
        return 0;
    }

    char rt_key[17] = {0}; 
    sprintf(rt_key, "user_%d_rt", id);

    size_t sz = 0;
    if (nvs_get_str(NVS, rt_key, NULL, &sz) != ESP_OK || sz == 0) {
        ESP_LOGE(TAG, "Refresh token not found");
        nvs_close(NVS);
        free(error);
        free(status);
        return 0;
    }

    char *rt_val = calloc(sz, sizeof(char));
    if (!rt_val) {
        ESP_LOGE(TAG, "Out of memory");
        nvs_close(NVS);
        free(error);
        free(status);
        return 0;
    }

    if (nvs_get_str(NVS, rt_key, rt_val, &sz) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get refresh token");
        free(rt_val);
        nvs_close(NVS);
        free(error);
        free(status);
        return 0;
    }

    // Prepare POST body
    char *body = malloc(MAX_POST_BODY_LENGTH * 2);
    if (!body) {
        ESP_LOGE(TAG, "Out of memory");
        free(rt_val);
        nvs_close(NVS);
        free(error);
        free(status);
        return 0;
    }
    snprintf(body, MAX_POST_BODY_LENGTH * 2, "client_id=%s&client_secret=%s&refresh_token=%s&grant_type=refresh_token", CLIENT_ID, CLIENT_SECRET, rt_val);

    // Response buffer for POST
    char *post_response = calloc(MAX_HTTP_OUTPUT_BUFFER, sizeof(char));
    if (!post_response) {
        ESP_LOGE(TAG, "Out of memory");
        free(body);
        free(rt_val);
        nvs_close(NVS);
        free(error);
        free(status);
        return 0;
    }

    int post_res = post_api(body, TOKEN_URI, post_response, MAX_HTTP_OUTPUT_BUFFER);
    if (!post_res) {
        ESP_LOGE(TAG, "Failed to refresh token");
        free(post_response);
        free(body);
        free(rt_val);
        nvs_close(NVS);
        free(error);
        free(status);
        return 0;
    }

    printf("POST RESPONSE: %s", post_response);

    // Extract new access token from response
    char *new_access_token = calloc(MAX_POST_BODY_LENGTH, sizeof(char));
    if (!new_access_token) {
        ESP_LOGE(TAG, "Out of memory");
        free(post_response);
        free(body);
        free(rt_val);
        nvs_close(NVS);
        free(error);
        free(status);
        return 0;
    }

    decompose_json_dynamic_params(post_response, 1, "access_token", new_access_token);

    if (strlen(new_access_token) == 0) {
        ESP_LOGE(TAG, "No access token in refresh response");
        free(new_access_token);
        free(post_response);
        free(body);
        free(rt_val);
        nvs_close(NVS);
        free(error);
        free(status);
        return 0;
    }

    // Store new access token in NVS
    char at_key[17] = {0};
    sprintf(at_key, "user_%d_at", id);
    nvs_set_str(NVS, at_key, new_access_token);
    nvs_commit(NVS);

    free(new_access_token);
    free(post_response);
    free(body);
    free(rt_val);
    free(error);
    free(status);
    nvs_close(NVS);

    return 1;
}


// ! THREAD SAFE
void token_management(char *code, char *scope, char* id) {
    // Ask token to Google API
    char *body = malloc(sizeof(char) * MAX_POST_BODY_LENGTH);
    sprintf(body, "code=%s&client_id=%s&client_secret=%s&redirect_uri=%s&grant_type=authorization_code&access_type=offline&prompt=consent", code, CLIENT_ID, CLIENT_SECRET, REDIRECT_URI);


    char *response = calloc(MAX_HTTP_OUTPUT_BUFFER, sizeof(char));
    if (!response) {
        ESP_LOGE(TAG, "Out of memory");
        free(body);
        return;
    }

    int ret = post_api(body, TOKEN_URI, response, MAX_HTTP_OUTPUT_BUFFER);
    if (!ret) {
        ESP_LOGE(TAG, "Token request failed");
        free(body);
        free(response);
        return;
    }

    char *access_token = calloc(MAX_POST_BODY_LENGTH, sizeof(char)); 
    char *refresh_token = calloc(MAX_POST_BODY_LENGTH, sizeof(char));

    decompose_json_dynamic_params(response, 2, "access_token", access_token, "refresh_token", refresh_token);

    ESP_LOGI(TAG, "ACCESS_TOKEN = %s", access_token);
    ESP_LOGI(TAG, "REFRESH TOKEN = %s", refresh_token);

    // Get Name
    int headers_length = 2;
    char *headers_keys[2] = {"Content-Type", "Authorization"};
    char *headers_values[2];
    headers_values[0] = "application/json";
    headers_values[1] = calloc(MAX_POST_BODY_LENGTH, sizeof(char));
    if (headers_values[1] == NULL) {
        ESP_LOGE(TAG, "OUT OF MEMORY");
        return;
    }
 
    snprintf(headers_values[1], MAX_POST_BODY_LENGTH, "Bearer %s", access_token);

    get_api(response, USER_INFO, headers_keys, headers_values, headers_length);
    
    char email[50] = {0}, surname[50] = {0};

    decompose_json_dynamic_params(response, 2, "email", email, "family_name", surname);

    ESP_LOGI(TAG, "EMAIL: %s", email);
    ESP_LOGI(TAG, "NAME: %s", surname);


    // Memorize -> email primary key
    nvs_handle_t NVS;
    nvs_open("general_data", NVS_READWRITE, &NVS);

    char at_key[15] = {0}, rt_key[15] = {0};

    ESP_LOGI(TAG, "ID %d", atoi(id));
    
    sprintf(at_key, "user_%d_at", atoi(id));
    sprintf(rt_key, "user_%d_rt", atoi(id));

    esp_err_t f = nvs_set_str(NVS, at_key, access_token);
    ESP_LOGI(TAG, "STATUS %d", f);
    f = nvs_set_str(NVS, rt_key, refresh_token);  
    ESP_LOGI(TAG, "STATUS %d", f);
    
    nvs_commit(NVS);
    nvs_close(NVS);
    
    free(body);
    free(response);
    free(access_token);
    free(refresh_token);
}

// ! THREAD SAFE
esp_err_t get_api_oauth2(char *content, size_t content_length, char *api_address, nvs_handle_t NVS, int id) {
    char at_key[17] = {0};
    ESP_LOGI(TAG, "ID %d", id);
    sprintf(at_key, "user_%d_at", id);

    // Init headers
    char *headers_keys[2] = {"Content-Type", "Authorization"};
    char *headers_values[2];
    headers_values[0] = "application/json";
    headers_values[1] = calloc(MAX_POST_BODY_LENGTH, sizeof(char));
    if (headers_values[1] == NULL) {
        ESP_LOGE(TAG, "OUT OF MEMORY");
        return ESP_FAIL;  
    }
    
    
    // Get access token
    char *access_token = NULL;
    size_t total_size = 0;
    get_from_nvs(NVS, at_key, &access_token, &total_size);


    if (access_token == NULL || strlen(access_token) <= 1) {
        if (access_token) free(access_token);
        free(headers_values[1]);
        ESP_LOGE(TAG, "Invalid/Do not found access token");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "access_token len: %d", strlen(access_token));
    ESP_LOGI(TAG, "access token: %s", access_token);

    snprintf(headers_values[1], MAX_POST_BODY_LENGTH, "Bearer %s", access_token);

    int attempt = 0;

    while (attempt < 2) {
        if (get_api(content, api_address, headers_keys, headers_values, 2)) break;

        // Access token may be expired
        if (attempt == 0) {
            if (refresh_token_managment(id, content)) {
                free(access_token); // get_from_nvs re allocate access_token 
                get_from_nvs(NVS, at_key, &access_token, &total_size);
                memset(content, 0, content_length);
                sprintf(headers_values[1], "Bearer %s", access_token);
                attempt++;
                continue; 
            }
        }

        // Maybe this user doesn't exist
        memset(content, 0, content_length);
        free(access_token);
        free(headers_values[1]);
        return ESP_FAIL;
    }

    free(access_token);
    free(headers_values[1]);

    return ESP_OK;
}