#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <curl/curl.h>
#include <jansson.h>

#define URL "https://169.254.231.229/cloud"
#define BASIC_AUTH "Authorization: Basic YWRtaW46R3RpMjAyNUA="
#define JSON_ACCEPT "accept: application/json"
#define JSON_TYPE "Content-Type: application/json"

struct memory {
  char *response;
  size_t size;
};

static size_t write_callback(void *data, size_t size, size_t nmemb, void *userp) {
  size_t total = size * nmemb;
  struct memory *mem = userp;
  char *ptr = realloc(mem->response, mem->size + total + 1);
  if (!ptr) return 0;
  mem->response = ptr;
  memcpy(&mem->response[mem->size], data, total);
  mem->size += total;
  mem->response[mem->size] = '\0';
  return total;
}

char* get_token(void) {
  CURL *curl = curl_easy_init();
  if (!curl) return NULL;

  struct memory chunk = {0};
  struct curl_slist *headers = NULL;

  headers = curl_slist_append(headers, JSON_ACCEPT);
  headers = curl_slist_append(headers, BASIC_AUTH);

  curl_easy_setopt(curl, CURLOPT_URL, URL "/localRestLogin");
  curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &chunk);

  CURLcode res = curl_easy_perform(curl);
  curl_easy_cleanup(curl);
  curl_slist_free_all(headers);

  if (res != CURLE_OK) {
    fprintf(stderr, "Token request failed: %s\n", curl_easy_strerror(res));
    free(chunk.response);
    return NULL;
  }

  json_error_t error;
  json_t *root = json_loads(chunk.response, 0, &error);
  free(chunk.response);

  if (!root) {
    fprintf(stderr, "JSON parse error: %s\n", error.text);
    return NULL;
  }

  json_t *msg = json_object_get(root, "message");
  const char *val = json_string_value(msg);
  char *copy = val ? strdup(val) : NULL;
  json_decref(root);
  return copy;
}

void post_command(const char *command, const char *token) {
  CURL *curl = curl_easy_init();
  if (!curl) return;

  char bearer[512];
  snprintf(bearer, sizeof(bearer), "Authorization: Bearer %s", token);

  struct curl_slist *headers = NULL;
  headers = curl_slist_append(headers, JSON_ACCEPT);
  headers = curl_slist_append(headers, bearer);
  headers = curl_slist_append(headers, JSON_TYPE);

  char body[512];
  snprintf(body, sizeof(body),
           "{\"command\":\"%s\",\"command_id\":16266718797272556,\"payload\":{}}",
           command);

  struct memory chunk = {0};
  curl_easy_setopt(curl, CURLOPT_URL, URL);
  curl_easy_setopt(curl, CURLOPT_POST, 1L);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body);
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &chunk);

  if (curl_easy_perform(curl) == CURLE_OK) {
    printf("POST %s response:\n%s\n", command, chunk.response);
  } else {
    fprintf(stderr, "POST %s failed\n", command);
  }

  curl_easy_cleanup(curl);
  curl_slist_free_all(headers);
  free(chunk.response);
}

char* read_file(const char *path, long *length) {
  FILE *f = fopen(path, "rb");
  if (!f) return NULL;
  fseek(f, 0, SEEK_END);
  long len = ftell(f);
  fseek(f, 0, SEEK_SET);
  char *data = malloc(len + 1);
  if (!data) {
    fclose(f);
    return NULL;
  }
  fread(data, 1, len, f);
  data[len] = '\0';
  fclose(f);
  if (length) *length = len;
  return data;
}

void put_config(const char *token, const char *path) {
  long len;
  char *data = read_file(path, &len);
  if (!data) {
    fprintf(stderr, "Failed to read config file: %s\n", path);
    return;
  }

  CURL *curl = curl_easy_init();
  if (!curl) {
    free(data);
    return;
  }

  char bearer[512];
  snprintf(bearer, sizeof(bearer), "Authorization: Bearer %s", token);

  struct curl_slist *headers = NULL;
  headers = curl_slist_append(headers, JSON_ACCEPT);
  headers = curl_slist_append(headers, bearer);
  headers = curl_slist_append(headers, JSON_TYPE);

  char full_url[512];
  snprintf(full_url, sizeof(full_url), "%s/mode", URL);

  struct memory chunk = {0};
  curl_easy_setopt(curl, CURLOPT_URL, full_url);
  curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, len);
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &chunk);

  if (curl_easy_perform(curl) == CURLE_OK) {
    printf("PUT mode config response:\n%s\n", chunk.response);
  } else {
    fprintf(stderr, "PUT mode failed\n");
  }

  curl_easy_cleanup(curl);
  curl_slist_free_all(headers);
  free(chunk.response);
  free(data);
}

int main(void) {
  char *token = get_token();
  if (!token) {
    fprintf(stderr, "Failed to get token\n");
    return EXIT_FAILURE;
  }

  printf("Step 1: Stopping...\n");
  post_command("stop", token);

  sleep(1);

  printf("Step 2: Setting config.json...\n");
  put_config(token, "config.json");

  sleep(1);

  printf("Step 3: Starting...\n");
  post_command("start", token);

  free(token);
  return EXIT_SUCCESS;
}

