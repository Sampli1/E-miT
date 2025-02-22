#include "nvs.h"
#include "esp_log.h"


#include "oauth2.h"

static const char *TAG = "OAUTH2";

int refresh_token_managment(int id, char *response) {
    // Check if response has UNAUTHENTICATED status
    char *error = calloc(MAX_HTTP_OUTPUT_BUFFER, sizeof(char)), *status = calloc(100, sizeof(char));
    decompose_json_dynamic_params(response, 1, "error", error);
    decompose_json_dynamic_params(error, 1, "status", status);


    ESP_LOGI(TAG, "REFRESH TOKEN REQ BY ID:%d", id);

    if (strcmp(status, "UNAUTHENTICATED") != 0) {
        // 
        free(error);
        free(status);
        return 0;
    }
    // Get refresh token from memory
    nvs_handle_t NVS;
    size_t sz;
    nvs_open("general_data", NVS_READWRITE, &NVS);
    char rt_key[17] = {0}; 
    sprintf(rt_key, "user_%d_rt", id);
    char *rt_val = calloc(500, sizeof(char));    
    nvs_get_str(NVS, rt_key, rt_val, &sz);
     
    // Ask for new token
    void *res;
    char *body = malloc(sizeof(char) * MAX_POST_BODY_LENGTH * 2);
    sprintf(body, "client_id=%s&client_secret=%s&refresh_token=%s&grant_type=refresh_token", CLIENT_ID, CLIENT_SECRET, rt_val);
    char path[MAX_SPIFFS_PATH_LENGTH] = "/spiffs/upload-video-google-chain.pem";
    post_api(body, TOKEN_URI, client_http_google, &res, path);

    if (res == NULL) {
        ESP_LOGE(TAG, "No refresh token");

        free(rt_val);
        free(body);
        // free(res); post_api frees res
        free(error);
        free(status);
        nvs_close(NVS);
        return 0;
    }
    
    // Memorize new access token
    char at_key[17] = {0};
    sprintf(at_key, "user_%d_at", id);
    char *new_access_token = calloc(MAX_POST_BODY_LENGTH, sizeof(char));
    decompose_json_dynamic_params(res, 1, "access_token", new_access_token);
    nvs_set_str(NVS, at_key, new_access_token);  

    nvs_commit(NVS);
    
    free(new_access_token);
    free(rt_val);
    
    free(body);
    // free(res); post_api frees res
    
    free(error);
    free(status);
    
    nvs_close(NVS);
    return 1;
}

void token_management(char *code, char *scope, char* id) {
    // Ask token to Google API
    char *body = malloc(sizeof(char) * MAX_POST_BODY_LENGTH);
    void *response;
    sprintf(body, "code=%s&client_id=%s&client_secret=%s&redirect_uri=%s&grant_type=authorization_code&access_type=offline&prompt=consent", code, CLIENT_ID, CLIENT_SECRET, REDIRECT_URI);

    char path[MAX_SPIFFS_PATH_LENGTH] = "/spiffs/upload-video-google-chain.pem";
    post_api(body, TOKEN_URI, client_http_google, &response, path);

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
 
    sprintf(headers_values[1], "Bearer %s", access_token);

    for (int i = 0; i < headers_length; i++) ESP_LOGI(TAG, "{%s : %s}", headers_keys[i], headers_values[i]);
    
    get_api(response, USER_INFO, client_http_google, headers_keys, headers_values, headers_length);
    
    char email[50] = {0}, surname[50] = {0};

    decompose_json_dynamic_params(response, 2, "email", email, "family_name", surname);

    ESP_LOGI(TAG, "EMAIL: %s", email);
    ESP_LOGI(TAG, "NAME: %s", surname);


    // Memorize -> email primary key
    nvs_handle_t NVS;
    nvs_open("general_data", NVS_READWRITE, &NVS);

    char at_key[15] = {0}, rt_key[15] = {0};

    // NON FUNZIONA DA QUA (ogni tanto, perché?)

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
    free(access_token);
    free(refresh_token);
}
