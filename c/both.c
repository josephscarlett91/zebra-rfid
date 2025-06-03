#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <jansson.h>
#include <ncurses.h>
#include <MQTTClient.h>
#include <curl/curl.h>
#include <sys/stat.h>

// ========== REST CONFIG ==========
#define URL "https://169.254.231.229/cloud"
#define BASIC_AUTH "Authorization: Basic YWRtaW46R3RpMjAyNUA="
#define JSON_ACCEPT "accept: application/json"
#define JSON_TYPE "Content-Type: application/json"

// ========== MQTT CONFIG ==========
#define ADDRESS     "tcp://192.168.12.19:1883"
#define CLIENTID    "CClient"
#define TOPIC       "GTI/Zebra/Testing"
#define QOS         1
#define CSV_FILE    "tag_data.csv"

// ========== GLOBALS ==========
MQTTClient client;
volatile int running = 1;
char current_type[64] = "default";
char *auth_token = NULL;

// ========== UTILS ==========
struct memory { char *response; size_t size; };
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

// ========== REST FUNCTIONS ==========
char* get_token() {
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
    if (!root) return NULL;

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

    if (curl_easy_perform(curl) == CURLE_OK)
        mvprintw(10, 0, "Command '%s' sent.\n", command);
    else
        mvprintw(10, 0, "Failed to send command '%s'\n", command);

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
    if (!data) return NULL;
    fread(data, 1, len, f);
    data[len] = '\0';
    fclose(f);
    if (length) *length = len;
    return data;
}

void put_config(const char *token, const char *path) {
    long len;
    char *data = read_file(path, &len);
    if (!data) return;

    CURL *curl = curl_easy_init();
    if (!curl) { free(data); return; }

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

    if (curl_easy_perform(curl) == CURLE_OK)
        mvprintw(9, 0, "Config uploaded.\n");
    else
        mvprintw(9, 0, "Failed to upload config.\n");

    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);
    free(chunk.response);
    free(data);
}

// ========== MQTT & DISPLAY ==========
int is_valid_utf8(const char *s, size_t len) {
    size_t i = 0;
    while (i < len) {
        unsigned char c = s[i];
        if (c <= 0x7F) i++;
        else if ((c & 0xE0) == 0xC0 && i + 1 < len && (s[i+1] & 0xC0) == 0x80) i += 2;
        else if ((c & 0xF0) == 0xE0 && i + 2 < len && (s[i+1] & 0xC0) == 0x80 && (s[i+2] & 0xC0) == 0x80) i += 3;
        else if ((c & 0xF8) == 0xF0 && i + 3 < len && (s[i+1] & 0xC0) == 0x80 && (s[i+2] & 0xC0) == 0x80 && (s[i+3] & 0xC0) == 0x80) i += 4;
        else return 0;
    }
    return 1;
}

void write_to_csv(json_t* row) {
    int write_header = access(CSV_FILE, F_OK) != 0;
    FILE *f = fopen(CSV_FILE, "a");
    if (!f) return;
    void *iter = json_object_iter(row);

    if (write_header) {
        while (iter) {
            fprintf(f, "%s%s", json_object_iter_key(iter),
                    json_object_iter_next(row, iter) ? "," : "\n");
            iter = json_object_iter_next(row, iter);
        }
        iter = json_object_iter(row);
    }

    while (iter) {
        json_t *val = json_object_iter_value(iter);
        char *val_str = json_dumps(val, JSON_ENCODE_ANY | JSON_COMPACT);
        if (val_str) {
            fprintf(f, "%s%s", val_str, json_object_iter_next(row, iter) ? "," : "\n");
            free(val_str);
        } else {
            fprintf(f, "\"\"%s", json_object_iter_next(row, iter) ? "," : "\n");
        }
        iter = json_object_iter_next(row, iter);
    }
    fclose(f);
}

int on_message(void *context, char *topicName, int topicLen, MQTTClient_message *message) {
    char *payload = (char *)message->payload;
    int len = message->payloadlen;

    if (!is_valid_utf8(payload, len)) goto cleanup;
    if (payload[0] != '{' || payload[len - 1] != '}') goto cleanup;
    if (!strstr(payload, "\"idHex\"") || !strstr(payload, "\"channel\"")) goto cleanup;

    json_error_t error;
    json_t *root = json_loadb(payload, len, 0, &error);
    if (!root) goto cleanup;

    json_t *data = json_object_get(root, "data");
    json_t *timestamp = json_object_get(root, "timestamp");
    json_t *type = json_object_get(root, "type");

    if (json_is_object(data)) {
        json_t *flat = json_deep_copy(data);
        if (timestamp) json_object_set(flat, "timestamp", timestamp);
        if (type) json_object_set(flat, "type", type);
        write_to_csv(flat);
        char *pretty = json_dumps(flat, JSON_COMPACT);
        mvprintw(11, 0, "Tag: %s\n", pretty);
        free(pretty);
        json_decref(flat);
    }
    json_decref(root);

cleanup:
    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    return 1;
}

void init_mqtt() {
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    MQTTClient_create(&client, ADDRESS, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
    MQTTClient_setCallbacks(client, NULL, NULL, on_message, NULL);

    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;

    int rc = MQTTClient_connect(client, &conn_opts);
    if (rc != MQTTCLIENT_SUCCESS) {
        endwin();
        fprintf(stderr, "Failed to connect to broker: %d\n", rc);
        exit(EXIT_FAILURE);
    }

    MQTTClient_subscribe(client, TOPIC, QOS);
}

// ========== INTERACTIVE LOOP ==========
void command_loop() {
    mvprintw(0, 0, "Commands:\n[1] Start\n[2] Stop\n[q] Quit");
    refresh();

    while (running) {
        int ch = getch();
        switch (ch) {
            case '1': post_command("start", auth_token); break;
            case '2': post_command("stop", auth_token); break;
            case 'q': running = 0; break;
        }
        MQTTClient_yield();
        usleep(100000);
    }
}

// ========== MAIN ==========
int main() {
    initscr(); noecho(); cbreak(); timeout(100);

    auth_token = get_token();
    if (!auth_token) {
        endwin();
        fprintf(stderr, "Token auth failed.\n");
        return EXIT_FAILURE;
    }

    post_command("stop", auth_token);
    sleep(1);
    put_config(auth_token, "config.json");
    sleep(1);
    post_command("start", auth_token);
    sleep(1);

    init_mqtt();
    command_loop();

    post_command("stop", auth_token);
    free(auth_token);

    MQTTClient_disconnect(client, 1000);
    MQTTClient_destroy(&client);
    endwin();
    return 0;
}

