#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <jansson.h>
#include <ncurses.h>
#include <MQTTClient.h>
#include <sys/stat.h>

#define ADDRESS     "tcp://192.168.12.19:1883"
#define CLIENTID    "CClient"
#define TOPIC       "GTI/Zebra/Testing"
#define QOS         1
#define CSV_FILE    "tag_data.csv"

MQTTClient client;
volatile int running = 1;
char current_type[64] = "default";

// -----------------------------
// Stubbed ReaderData Functions
// -----------------------------
void start_inventory() { mvprintw(12, 0, "Starting inventory...\n"); }
void stop_inventory()  { mvprintw(12, 0, "Stopping inventory...\n"); }
const char* get_mode() { return "reader-mode-X"; }
void set_mode()        { mvprintw(12, 0, "Set mode called\n"); }
void change_type(const char* t) { strncpy(current_type, t, sizeof(current_type)); }
void change_power(int pwr)      { mvprintw(12, 0, "Changing power to %d\n", pwr); }
void connect_wifi()             { mvprintw(12, 0, "Connecting to Wi-Fi...\n"); }

// -----------------------------
// CSV Writer
// -----------------------------
void write_to_csv(json_t* row) {
    int write_header = access(CSV_FILE, F_OK) != 0;
    FILE *f = fopen(CSV_FILE, "a");
    if (!f) return;

    void *iter = json_object_iter(row);

    // Write header
    if (write_header) {
        while (iter) {
            fprintf(f, "%s%s", json_object_iter_key(iter),
                    json_object_iter_next(row, iter) ? "," : "\n");
            iter = json_object_iter_next(row, iter);
        }
        iter = json_object_iter(row); // Reset for value writing
    }

    // Write values
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

// -----------------------------
// MQTT Callback
// -----------------------------
#define MAX_JSON_SIZE 2000  // or whatever limit makes sense

int is_valid_utf8(const char *s, size_t len) {
    size_t i = 0;
    while (i < len) {
        unsigned char c = s[i];
        if (c <= 0x7F) {
            i++;
        } else if ((c & 0xE0) == 0xC0 && i + 1 < len &&
                   (s[i+1] & 0xC0) == 0x80) {
            i += 2;
        } else if ((c & 0xF0) == 0xE0 && i + 2 < len &&
                   (s[i+1] & 0xC0) == 0x80 &&
                   (s[i+2] & 0xC0) == 0x80) {
            i += 3;
        } else if ((c & 0xF8) == 0xF0 && i + 3 < len &&
                   (s[i+1] & 0xC0) == 0x80 &&
                   (s[i+2] & 0xC0) == 0x80 &&
                   (s[i+3] & 0xC0) == 0x80) {
            i += 4;
        } else {
            return 0;  // Invalid UTF-8
        }
    }
    return 1;
}

int on_message(void *context, char *topicName, int topicLen, MQTTClient_message *message) {
    char *payload = (char *)message->payload;
    int len = message->payloadlen;

    // Skip if not valid UTF-8
    if (!is_valid_utf8(payload, len)) {
        goto cleanup;
    }

    // Skip if not a valid JSON object
    if (payload[0] != '{' || payload[len - 1] != '}') {
        goto cleanup;
    }

    // Only process if it looks like a tag read
    if (!strstr(payload, "\"idHex\"") || !strstr(payload, "\"channel\"")) {
        goto cleanup;
    }

    json_error_t error;
    json_t *root = json_loadb(payload, len, 0, &error);
    if (!root) {
        goto cleanup;
    }

    /*
    // Write exactly what was received
    write_to_csv(root);

    // Optional screen output
    char *pretty = json_dumps(root, JSON_COMPACT);
    if (pretty) {
        mvprintw(11, 0, "Tag: %s", pretty);
        free(pretty);
    }
    refresh();

    json_decref(root);
    */
    json_t *data = json_object_get(root, "data");
json_t *timestamp = json_object_get(root, "timestamp");
json_t *type = json_object_get(root, "type");

if (json_is_object(data)) {
    json_t *flat = json_deep_copy(data);
    if (timestamp) json_object_set(flat, "timestamp", timestamp);
    if (type) json_object_set(flat, "type", type);

    write_to_csv(flat);

    char *pretty = json_dumps(flat, JSON_COMPACT);
    if (pretty) {
        mvprintw(11, 0, "Tag: %s", pretty);
        free(pretty);
    }

    json_decref(flat);
}




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

// -----------------------------
// Main Menu Loop (Ncurses)
// -----------------------------
void command_loop() {
  mvprintw(0, 0, "Commands:");
  mvprintw(1, 0, "[1] Start Inventory");
  mvprintw(2, 0, "[2] Stop Inventory");
  mvprintw(3, 0, "[m] Print Mode");
  mvprintw(4, 0, "[n] Set Mode");
  mvprintw(5, 0, "[t] Change Type");
  mvprintw(6, 0, "[p] Change Power");
  mvprintw(7, 0, "[w] Connect WiFi");
  mvprintw(8, 0, "[q] Quit");

  refresh();

  while (running) {
    int ch = getch();
    char buf[64];

    switch (ch) {
    case '1': start_inventory(); break;
    case '2': stop_inventory(); break;
    case 'm': mvprintw(13, 0, "Mode: %s\n", get_mode()); break;
    case 'n': set_mode(); break;
    case 't':
              echo(); mvprintw(14, 0, "New type: "); getstr(buf); noecho();
              change_type(buf);
              break;
    case 'p':
              echo(); mvprintw(15, 0, "Power: "); getstr(buf); noecho();
              change_power(atoi(buf));
              break;
    case 'w': connect_wifi(); break;
    case 'q': running = 0; break;
    }

    MQTTClient_yield(); // allow message processing
    usleep(100000); // 100ms
  }
}

// -----------------------------
// Main
// -----------------------------
int main() {
  initscr(); noecho(); cbreak();
  timeout(100); // non-blocking getch()

  init_mqtt();
  command_loop();

  endwin();
  MQTTClient_disconnect(client, 1000);
  MQTTClient_destroy(&client);
  return 0;
}

