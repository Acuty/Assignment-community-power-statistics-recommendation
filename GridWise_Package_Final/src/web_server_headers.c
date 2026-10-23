#include "mongoose.h"
#include "web_server.h"

static void reply_error(struct mg_connection* connection, int status, const char* message);
static size_t get_request_role(struct mg_http_message* request);
static size_t require_admin(struct mg_connection* connection, struct mg_http_message* request);
static size_t require_login(struct mg_connection* connection, struct mg_http_message* request);
static void handle_health(struct mg_connection* connection);
static void handle_login(struct mg_connection* connection, struct mg_http_message* request);
static size_t read_page(struct mg_http_message* request, size_t* page);
static size_t json_escape(const char* source, char* destination, size_t capacity);
static void handle_get_records(struct mg_connection* connection, struct mg_http_message* request);
static void handle_recommend(struct mg_connection* connection, struct mg_http_message* request);
static size_t parse_record_body(struct mg_http_message* request, ElectricBill* record);
static size_t find_record(struct mg_http_message* request, size_t* found_index);
static void handle_add_record(struct mg_connection* connection, struct mg_http_message* request);
static void handle_update_record(struct mg_connection* connection, struct mg_http_message* request);
static void handle_delete_record(struct mg_connection* connection, struct mg_http_message* request);
static void http_event_handler(struct mg_connection* connection, int event, void* event_data);
size_t web_server_start(SystemData* data, const char* address, const char* csv_filename, const double indicator_weights[EXPANSION_INDICATOR_COUNT]);
