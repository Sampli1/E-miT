#include "nvs.h"
#include "esp_log.h"


#include "oauth2.h"

static const char *TAG = "OAUTH2";

static int jsoneq(const char *json, jsmntok_t *tok, const char *s) {
  if (tok->type == JSMN_STRING && (int)strlen(s) == tok->end - tok->start &&
      strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
    return 0;
  }
  return -1;
}

void decompose_json_2_params(char *json, const char *param1_name, char *param1_value, const char *param2_name, char *param2_value) {
    int i;
    int r;
    jsmn_parser p;
    jsmntok_t t[128];

    jsmn_init(&p);
    r = jsmn_parse(&p, json, strlen(json), t, sizeof(t) / sizeof(t[0]));
    if (r < 0) {
        printf("Failed to parse JSON: %d\n", r);
        return;
    }

    if (r < 1 || t[0].type != JSMN_OBJECT) {
        printf("Object expected\n");
        return;
    }

    for (i = 1; i < r; i++) {
        if (jsoneq(json, &t[i], param1_name) == 0) {
            strncpy(param1_value, json + t[i + 1].start, t[i + 1].end - t[i + 1].start);
            i++;
        } else if (jsoneq(json, &t[i], param2_name) == 0) {
            strncpy(param2_value, json + t[i + 1].start, t[i + 1].end - t[i + 1].start);
            i++;
        }
    }
}

void decompose_json_1_params(char *json, const char *param1_name, char *param1_value) {
    int i;
    int r;
    jsmn_parser p;
    jsmntok_t t[128];

    jsmn_init(&p);
    r = jsmn_parse(&p, json, strlen(json), t, sizeof(t) / sizeof(t[0]));
    if (r < 0) {
        printf("Failed to parse JSON: %d\n", r);
        return;
    }

    if (r < 1 || t[0].type != JSMN_OBJECT) {
        printf("Object expected\n");
        return;
    }

    for (i = 1; i < r; i++) {
        if (jsoneq(json, &t[i], param1_name) == 0) {
            strncpy(param1_value, json + t[i + 1].start, t[i + 1].end - t[i + 1].start);
            i++;
        }
    }
}

void refresh_token_management(char *surname) {
    // Get refresh token from memory
    nvs_handle_t NVS;
    size_t sz;
    nvs_open("oauth2_tokens", NVS_READWRITE, &NVS);
    char *json_info= calloc(500, sizeof(char));    
    nvs_get_str(NVS, surname, json_info, &sz);
    
    char *refresh_token = calloc(MAX_POST_BODY_LENGTH, sizeof(char));
    decompose_json_1_params(json_info, "refresh_token", refresh_token);
    
    // Ask for new token
    void *response;
    char *body = malloc(sizeof(char) * MAX_POST_BODY_LENGTH * 2);
    sprintf(body, "client_id=%s&client_secret=%s&refresh_token=%s&grant_type=refresh_token", CLIENT_ID, CLIENT_SECRET, refresh_token);
    post_api(body, TOKEN_URI, client_http, &response);

    if (response == NULL) {
        ESP_LOGE(TAG, "No refresh token");
        return;
    }
    
    // Memorize new access token
    char *new_access_token = calloc(MAX_POST_BODY_LENGTH, sizeof(char));
    decompose_json_1_params(response, "access_token", new_access_token);
    sprintf(json_info, "{ \"access_token\": \"%s\", \"refresh_token\": \"%s\"}", new_access_token, refresh_token);
    nvs_set_str(NVS, surname, json_info);  
    nvs_commit(NVS);
    nvs_close(NVS);

    free(json_info);
    free(new_access_token);
    free(body);
}

void token_management(char *code, char *scope) {
    // Ask token to Google API
    char *body = malloc(sizeof(char) * MAX_POST_BODY_LENGTH);
    void *response;
    sprintf(body, "code=%s&client_id=%s&client_secret=%s&redirect_uri=%s&grant_type=authorization_code&access_type=offline&prompt=consent", code, CLIENT_ID, CLIENT_SECRET, REDIRECT_URI);

    post_api(body, TOKEN_URI, client_http, &response);

    char *access_token = malloc(sizeof(char) * MAX_POST_BODY_LENGTH);
    char *refresh_token = malloc(sizeof(char) * MAX_POST_BODY_LENGTH);
    memset(access_token, 0, MAX_POST_BODY_LENGTH);
    memset(refresh_token, 0, MAX_POST_BODY_LENGTH);

    decompose_json_2_params(response, "access_token", access_token, "refresh_token", refresh_token);

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
    
    get_api(response, USER_INFO, client_http, headers_keys, headers_values, headers_length);
    
    char email[50] = {0}, surname[50] = {0};

    decompose_json_2_params(response, "email", email, "family_name", surname);

    ESP_LOGI(TAG, "EMAIL: %s", email);
    ESP_LOGI(TAG, "NAME: %s", surname);


    // Memorize -> email primary key
    nvs_handle_t NVS;
    nvs_open("oauth2_tokens", NVS_READWRITE, &NVS);

    char *json_info= calloc(500, sizeof(char));    

    sprintf(json_info, "{ \"access_token\": \"%s\", \"refresh_token\": \"%s\"}", access_token, refresh_token);
    esp_err_t f = nvs_set_str(NVS, surname, json_info);  
    ESP_LOGI(TAG, "STATUS %d", f);
    nvs_commit(NVS);

    free(body);
    free(json_info);
    free(access_token);
    free(refresh_token);
}
