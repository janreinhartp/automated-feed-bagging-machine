#include "web_server.hpp"

#include "hardware_config.hpp"
#include "settings_model.hpp"

#include "cJSON.h"
#include "esp_event.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "lwip/ip4_addr.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace feed {

namespace {
constexpr const char* kTag = "web";

extern const char index_html_start[] asm("_binary_index_html_start");
extern const char index_html_end[] asm("_binary_index_html_end");
extern const char settings_html_start[] asm("_binary_settings_html_start");
extern const char settings_html_end[] asm("_binary_settings_html_end");
extern const char calibration_html_start[] asm("_binary_calibration_html_start");
extern const char calibration_html_end[] asm("_binary_calibration_html_end");
extern const char manual_html_start[] asm("_binary_manual_html_start");
extern const char manual_html_end[] asm("_binary_manual_html_end");
extern const char diagnostics_html_start[] asm("_binary_diagnostics_html_start");
extern const char diagnostics_html_end[] asm("_binary_diagnostics_html_end");
extern const char site_css_start[] asm("_binary_site_css_start");
extern const char site_css_end[] asm("_binary_site_css_end");
extern const char app_js_start[] asm("_binary_app_js_start");
extern const char app_js_end[] asm("_binary_app_js_end");

struct EmbeddedFile {
    const char* data;
    std::size_t length;
    const char* content_type;
};

struct WsState {
    httpd_handle_t server;
    WebServer* app;
    int fds[4];
    int count;
    SemaphoreHandle_t mutex;
};

WsState g_ws = {};

void bind_file(EmbeddedFile& file, const char* start, const char* end, const char* content_type) {
    file.data = start;
    std::size_t length = static_cast<std::size_t>(end - start);
    if (length > 0 && start[length - 1] == '\0') {
        --length;
    }
    file.length = length;
    file.content_type = content_type;
}

void json_escape(char* destination, std::size_t capacity, const char* source) {
    if (destination == nullptr || capacity == 0) {
        return;
    }
    std::size_t used = 0;
    if (source == nullptr) {
        destination[0] = '\0';
        return;
    }
    for (std::size_t index = 0; source[index] != '\0' && used + 2 < capacity; ++index) {
        const unsigned char ch = static_cast<unsigned char>(source[index]);
        if (ch < 0x20) {
            continue;
        }
        if (ch == '"' || ch == '\\') {
            destination[used++] = '\\';
        }
        destination[used++] = static_cast<char>(ch);
    }
    destination[used] = '\0';
}

int base64_digit(char ch) {
    if (ch >= 'A' && ch <= 'Z') {
        return ch - 'A';
    }
    if (ch >= 'a' && ch <= 'z') {
        return ch - 'a' + 26;
    }
    if (ch >= '0' && ch <= '9') {
        return ch - '0' + 52;
    }
    if (ch == '+') {
        return 62;
    }
    if (ch == '/') {
        return 63;
    }
    return -1;
}

bool base64_decode(const char* input, char* output, std::size_t capacity) {
    if (input == nullptr || output == nullptr || capacity == 0) {
        return false;
    }
    int accumulator = 0;
    int bits = -8;
    std::size_t used = 0;
    for (std::size_t index = 0; input[index] != '\0'; ++index) {
        if (input[index] == '=') {
            break;
        }
        const int digit = base64_digit(input[index]);
        if (digit < 0) {
            return false;
        }
        accumulator = (accumulator << 6) + digit;
        bits += 6;
        if (bits >= 0) {
            if (used + 1 >= capacity) {
                return false;
            }
            output[used++] = static_cast<char>((accumulator >> bits) & 0xFF);
            bits -= 8;
        }
    }
    output[used] = '\0';
    return true;
}

bool credentials_match(WebServer& server, const char* user, const char* password) {
    const bool user_ok = user != nullptr && std::strcmp(user, kWebUser) == 0;
    const bool password_ok = server.controller().settings().web_password_matches(password);
    return user_ok && password_ok;
}

bool authorized(httpd_req_t* request, WebServer& server) {
    char header[180] = {};
    if (httpd_req_get_hdr_value_str(request, "Authorization", header, sizeof(header)) != ESP_OK) {
        return false;
    }
    if (std::strncmp(header, "Basic ", 6) != 0) {
        return false;
    }
    char decoded[96] = {};
    if (!base64_decode(header + 6, decoded, sizeof(decoded))) {
        return false;
    }
    char* colon = std::strchr(decoded, ':');
    if (colon == nullptr) {
        return false;
    }
    *colon = '\0';
    return credentials_match(server, decoded, colon + 1);
}

esp_err_t send_json(httpd_req_t* request, const char* status, const char* body) {
    httpd_resp_set_status(request, status);
    httpd_resp_set_type(request, "application/json");
    return httpd_resp_send(request, body, HTTPD_RESP_USE_STRLEN);
}

esp_err_t send_unauthorized(httpd_req_t* request) {
    httpd_resp_set_hdr(request, "WWW-Authenticate", "Basic realm=\"feed-bagging\"");
    return send_json(request, "401 Unauthorized", "{\"ok\":false,\"error\":\"authentication_required\"}");
}

bool require_auth(httpd_req_t* request, WebServer& server) {
    if (authorized(request, server)) {
        return true;
    }
    send_unauthorized(request);
    return false;
}

bool read_body(httpd_req_t* request, char* buffer, std::size_t capacity) {
    if (request->content_len <= 0 || static_cast<std::size_t>(request->content_len) + 1 > capacity) {
        return false;
    }
    int received = 0;
    while (received < request->content_len) {
        const int chunk = httpd_req_recv(request, buffer + received, static_cast<std::size_t>(request->content_len - received));
        if (chunk == HTTPD_SOCK_ERR_TIMEOUT) {
            continue;
        }
        if (chunk <= 0) {
            return false;
        }
        received += chunk;
    }
    buffer[received] = '\0';
    return true;
}

void write_status_json(WebServer& server, char* buffer, std::size_t capacity) {
    ControllerStatus status = {};
    server.controller().copy_status(status);
    char reason[80] = {};
    json_escape(reason, sizeof(reason), status.inhibit_reason);
    char weight[16] = "null";
    if (status.weight_valid) {
        std::snprintf(weight, sizeof(weight), "%ld", static_cast<long>(status.weight_g));
    }
    std::snprintf(
        buffer, capacity,
        "{\"ok\":true,\"state\":\"%s\",\"motion_permitted\":%s,\"emergency_stop_latched\":%s,"
        "\"safety_feedback_trusted\":%s,\"weighing_protocol_verified\":%s,\"io_protocol_verified\":%s,"
        "\"weight_g\":%s,\"uptime_ms\":%lu,\"completed_bags\":%lu,\"drivers_ok\":%s,"
        "\"password_change_required\":%s,\"sequence_implemented\":false,\"phase\":1,"
        "\"weighing_baud\":%lu,\"weighing_slave\":%u,\"io_baud\":%lu,\"io_slave\":%u,"
        "\"inhibit_reason\":\"%s\",\"ap_address\":\"%s\"}",
        status.state_name, status.motion_permitted ? "true" : "false",
        status.emergency_stop_latched ? "true" : "false", status.safety_feedback_trusted ? "true" : "false",
        status.weighing_protocol_verified ? "true" : "false", status.io_protocol_verified ? "true" : "false", weight,
        static_cast<unsigned long>(status.uptime_ms), static_cast<unsigned long>(status.completed_bags),
        status.drivers_ok ? "true" : "false", status.password_change_required ? "true" : "false",
        static_cast<unsigned long>(status.weighing_baud), static_cast<unsigned>(status.weighing_slave),
        static_cast<unsigned long>(status.io_baud), static_cast<unsigned>(status.io_slave), reason, hw::kApAddress);
}

esp_err_t status_handler(httpd_req_t* request) {
    auto* server = static_cast<WebServer*>(request->user_ctx);
    char body[1024] = {};
    write_status_json(*server, body, sizeof(body));
    return send_json(request, "200 OK", body);
}

esp_err_t settings_get_handler(httpd_req_t* request) {
    auto* server = static_cast<WebServer*>(request->user_ctx);
    const MachineParameters& value = server->controller().settings().machine();
    char body[1024] = {};
    std::snprintf(
        body, sizeof(body),
        "{\"ok\":true,\"thresholds_are_placeholders\":true,\"target_weight_g\":%ld,\"lower_limit_g\":%ld,"
        "\"upper_limit_g\":%ld,\"coarse_feeder_percent\":%u,\"medium_feeder_percent\":%u,\"fine_feeder_percent\":%u,"
        "\"coarse_gate_percent\":%u,\"medium_gate_percent\":%u,\"fine_gate_percent\":%u,\"coarse_to_medium_g\":%ld,"
        "\"medium_to_fine_g\":%ld,\"fine_cutoff_g\":%ld,\"overrun_compensation_g\":%ld,\"stable_weight_ms\":%lu,"
        "\"discharge_ms\":%lu,\"empty_hopper_g\":%ld,\"servo_move_ms\":%lu,\"bag_presence_debounce_ms\":%lu,"
        "\"bag_removal_timeout_ms\":%lu}",
        static_cast<long>(value.target_weight_g), static_cast<long>(value.lower_limit_g),
        static_cast<long>(value.upper_limit_g), value.coarse_feeder_percent, value.medium_feeder_percent,
        value.fine_feeder_percent, value.coarse_gate_percent, value.medium_gate_percent, value.fine_gate_percent,
        static_cast<long>(value.coarse_to_medium_g), static_cast<long>(value.medium_to_fine_g),
        static_cast<long>(value.fine_cutoff_g), static_cast<long>(value.overrun_compensation_g),
        static_cast<unsigned long>(value.stable_weight_ms), static_cast<unsigned long>(value.discharge_ms),
        static_cast<long>(value.empty_hopper_g), static_cast<unsigned long>(value.servo_move_ms),
        static_cast<unsigned long>(value.bag_presence_debounce_ms),
        static_cast<unsigned long>(value.bag_removal_timeout_ms));
    return send_json(request, "200 OK", body);
}

bool take_i32(const cJSON* root, const char* key, int32_t& slot) {
    const cJSON* item = cJSON_GetObjectItemCaseSensitive(root, key);
    if (item == nullptr) {
        return true;
    }
    if (!cJSON_IsNumber(item)) {
        return false;
    }
    slot = item->valueint;
    return true;
}

bool take_u8(const cJSON* root, const char* key, uint8_t& slot) {
    const cJSON* item = cJSON_GetObjectItemCaseSensitive(root, key);
    if (item == nullptr) {
        return true;
    }
    if (!cJSON_IsNumber(item) || item->valueint < 0 || item->valueint > 100) {
        return false;
    }
    slot = static_cast<uint8_t>(item->valueint);
    return true;
}

bool take_u32(const cJSON* root, const char* key, uint32_t& slot) {
    const cJSON* item = cJSON_GetObjectItemCaseSensitive(root, key);
    if (item == nullptr) {
        return true;
    }
    if (!cJSON_IsNumber(item) || item->valueint < 0) {
        return false;
    }
    slot = static_cast<uint32_t>(item->valueint);
    return true;
}

bool take_u16(const cJSON* root, const char* key, uint16_t& slot, int maximum) {
    const cJSON* item = cJSON_GetObjectItemCaseSensitive(root, key);
    if (item == nullptr) {
        return true;
    }
    if (!cJSON_IsNumber(item) || item->valueint < 0 || item->valueint > maximum) {
        return false;
    }
    slot = static_cast<uint16_t>(item->valueint);
    return true;
}

esp_err_t settings_post_handler(httpd_req_t* request) {
    auto* server = static_cast<WebServer*>(request->user_ctx);
    if (!require_auth(request, *server)) {
        return ESP_OK;
    }
    char body[1024] = {};
    if (!read_body(request, body, sizeof(body))) {
        return send_json(request, "400 Bad Request", "{\"ok\":false,\"error\":\"invalid_body\"}");
    }
    cJSON* root = cJSON_Parse(body);
    if (root == nullptr) {
        return send_json(request, "400 Bad Request", "{\"ok\":false,\"error\":\"invalid_json\"}");
    }
    MachineParameters value = server->controller().settings().machine();
    const bool parsed = take_i32(root, "target_weight_g", value.target_weight_g) &&
                        take_i32(root, "lower_limit_g", value.lower_limit_g) &&
                        take_i32(root, "upper_limit_g", value.upper_limit_g) &&
                        take_u8(root, "coarse_feeder_percent", value.coarse_feeder_percent) &&
                        take_u8(root, "medium_feeder_percent", value.medium_feeder_percent) &&
                        take_u8(root, "fine_feeder_percent", value.fine_feeder_percent) &&
                        take_u8(root, "coarse_gate_percent", value.coarse_gate_percent) &&
                        take_u8(root, "medium_gate_percent", value.medium_gate_percent) &&
                        take_u8(root, "fine_gate_percent", value.fine_gate_percent) &&
                        take_i32(root, "coarse_to_medium_g", value.coarse_to_medium_g) &&
                        take_i32(root, "medium_to_fine_g", value.medium_to_fine_g) &&
                        take_i32(root, "fine_cutoff_g", value.fine_cutoff_g) &&
                        take_i32(root, "overrun_compensation_g", value.overrun_compensation_g) &&
                        take_u32(root, "stable_weight_ms", value.stable_weight_ms) &&
                        take_u32(root, "discharge_ms", value.discharge_ms) &&
                        take_i32(root, "empty_hopper_g", value.empty_hopper_g) &&
                        take_u32(root, "servo_move_ms", value.servo_move_ms) &&
                        take_u32(root, "bag_presence_debounce_ms", value.bag_presence_debounce_ms) &&
                        take_u32(root, "bag_removal_timeout_ms", value.bag_removal_timeout_ms);
    cJSON_Delete(root);
    if (!parsed) {
        return send_json(request, "400 Bad Request", "{\"ok\":false,\"error\":\"invalid_field\"}");
    }
    const Status saved = server->controller().settings().save_machine(value);
    if (saved != Status::Ok) {
        return send_json(request, "400 Bad Request", "{\"ok\":false,\"error\":\"rejected\"}");
    }
    return send_json(request, "200 OK", "{\"ok\":true,\"applied_to_motion\":false}");
}

void write_axis(char* buffer, std::size_t capacity, const ServoAxisCalibration& axis) {
    std::snprintf(buffer, capacity,
                  "{\"closed_pulse_us\":%u,\"open_pulse_us\":%u,\"min_safe_pulse_us\":%u,\"max_safe_pulse_us\":%u,"
                  "\"calibrated\":false}",
                  axis.closed_pulse_us, axis.open_pulse_us, axis.min_safe_pulse_us, axis.max_safe_pulse_us);
}

esp_err_t calibration_get_handler(httpd_req_t* request) {
    auto* server = static_cast<WebServer*>(request->user_ctx);
    const ServoCalibration& servo = server->controller().calibration().servo();
    const WeighingCalibration& weighing = server->controller().calibration().weighing();
    char feeder[160] = {};
    char discharge[160] = {};
    write_axis(feeder, sizeof(feeder), servo.feeder_gate);
    write_axis(discharge, sizeof(discharge), servo.discharge_gate);
    char body[640] = {};
    std::snprintf(body, sizeof(body),
                  "{\"ok\":true,\"motion_uses_calibration\":false,\"feeder_gate\":%s,\"discharge_gate\":%s,"
                  "\"weighing\":{\"calibrated\":false,\"zero_offset_counts\":%ld,\"span_counts\":%ld,\"span_grams\":%ld}}",
                  feeder, discharge, static_cast<long>(weighing.zero_offset_counts),
                  static_cast<long>(weighing.span_counts), static_cast<long>(weighing.span_grams));
    return send_json(request, "200 OK", body);
}

esp_err_t calibration_post_handler(httpd_req_t* request) {
    auto* server = static_cast<WebServer*>(request->user_ctx);
    if (!require_auth(request, *server)) {
        return ESP_OK;
    }
    char body[1024] = {};
    if (!read_body(request, body, sizeof(body))) {
        return send_json(request, "400 Bad Request", "{\"ok\":false,\"error\":\"invalid_body\"}");
    }
    cJSON* root = cJSON_Parse(body);
    if (root == nullptr) {
        return send_json(request, "400 Bad Request", "{\"ok\":false,\"error\":\"invalid_json\"}");
    }
    const cJSON* calibrated = cJSON_GetObjectItemCaseSensitive(root, "calibrated");
    if (cJSON_IsTrue(calibrated) || (cJSON_IsNumber(calibrated) && calibrated->valueint != 0)) {
        cJSON_Delete(root);
        return send_json(request, "409 Conflict", "{\"ok\":false,\"error\":\"calibration_cannot_be_committed\"}");
    }
    ServoCalibration servo = server->controller().calibration().servo();
    WeighingCalibration weighing = server->controller().calibration().weighing();
    const cJSON* feeder = cJSON_GetObjectItemCaseSensitive(root, "feeder_gate");
    const cJSON* discharge = cJSON_GetObjectItemCaseSensitive(root, "discharge_gate");
    bool parsed = true;
    if (cJSON_IsObject(feeder)) {
        parsed = take_u16(feeder, "closed_pulse_us", servo.feeder_gate.closed_pulse_us, 2500) &&
                 take_u16(feeder, "open_pulse_us", servo.feeder_gate.open_pulse_us, 2500) &&
                 take_u16(feeder, "min_safe_pulse_us", servo.feeder_gate.min_safe_pulse_us, 2500) &&
                 take_u16(feeder, "max_safe_pulse_us", servo.feeder_gate.max_safe_pulse_us, 2500);
    }
    if (parsed && cJSON_IsObject(discharge)) {
        parsed = take_u16(discharge, "closed_pulse_us", servo.discharge_gate.closed_pulse_us, 2500) &&
                 take_u16(discharge, "open_pulse_us", servo.discharge_gate.open_pulse_us, 2500) &&
                 take_u16(discharge, "min_safe_pulse_us", servo.discharge_gate.min_safe_pulse_us, 2500) &&
                 take_u16(discharge, "max_safe_pulse_us", servo.discharge_gate.max_safe_pulse_us, 2500);
    }
    const cJSON* scale = cJSON_GetObjectItemCaseSensitive(root, "weighing");
    if (parsed && cJSON_IsObject(scale)) {
        parsed = take_i32(scale, "zero_offset_counts", weighing.zero_offset_counts) &&
                 take_i32(scale, "span_counts", weighing.span_counts) &&
                 take_i32(scale, "span_grams", weighing.span_grams);
    }
    cJSON_Delete(root);
    if (!parsed) {
        return send_json(request, "400 Bad Request", "{\"ok\":false,\"error\":\"invalid_field\"}");
    }
    if (server->controller().calibration().save_servo_draft(servo) != Status::Ok ||
        server->controller().calibration().save_weighing_draft(weighing) != Status::Ok) {
        return send_json(request, "400 Bad Request", "{\"ok\":false,\"error\":\"rejected\"}");
    }
    return send_json(request, "200 OK", "{\"ok\":true,\"committed\":false}");
}

esp_err_t password_handler(httpd_req_t* request) {
    auto* server = static_cast<WebServer*>(request->user_ctx);
    if (!require_auth(request, *server)) {
        return ESP_OK;
    }
    char body[256] = {};
    if (!read_body(request, body, sizeof(body))) {
        return send_json(request, "400 Bad Request", "{\"ok\":false,\"error\":\"invalid_body\"}");
    }
    cJSON* root = cJSON_Parse(body);
    const cJSON* next = root == nullptr ? nullptr : cJSON_GetObjectItemCaseSensitive(root, "next");
    if (!cJSON_IsString(next) || next->valuestring == nullptr) {
        cJSON_Delete(root);
        return send_json(request, "400 Bad Request", "{\"ok\":false,\"error\":\"invalid_field\"}");
    }
    const Status saved = server->controller().settings().save_web_password(next->valuestring);
    cJSON_Delete(root);
    if (saved != Status::Ok) {
        return send_json(request, "400 Bad Request", "{\"ok\":false,\"error\":\"rejected\"}");
    }
    return send_json(request, "200 OK", "{\"ok\":true}");
}

esp_err_t network_handler(httpd_req_t* request) {
    auto* server = static_cast<WebServer*>(request->user_ctx);
    if (!require_auth(request, *server)) {
        return ESP_OK;
    }
    char body[256] = {};
    if (!read_body(request, body, sizeof(body))) {
        return send_json(request, "400 Bad Request", "{\"ok\":false,\"error\":\"invalid_body\"}");
    }
    cJSON* root = cJSON_Parse(body);
    if (root == nullptr) {
        return send_json(request, "400 Bad Request", "{\"ok\":false,\"error\":\"invalid_json\"}");
    }
    NetworkParameters network = server->controller().settings().network();
    const cJSON* ssid = cJSON_GetObjectItemCaseSensitive(root, "ssid");
    const cJSON* wifi_password = cJSON_GetObjectItemCaseSensitive(root, "wifi_password");
    if (cJSON_IsString(ssid) && ssid->valuestring != nullptr) {
        copy_literal(network.ssid, sizeof(network.ssid), ssid->valuestring);
    }
    if (cJSON_IsString(wifi_password) && wifi_password->valuestring != nullptr) {
        copy_literal(network.wifi_password, sizeof(network.wifi_password), wifi_password->valuestring);
    }
    cJSON_Delete(root);
    if (server->controller().settings().save_network(network) != Status::Ok) {
        return send_json(request, "400 Bad Request", "{\"ok\":false,\"error\":\"rejected\"}");
    }
    return send_json(request, "200 OK", "{\"ok\":true,\"restart_required\":true}");
}

esp_err_t command_handler(httpd_req_t* request) {
    auto* server = static_cast<WebServer*>(request->user_ctx);
    if (!require_auth(request, *server)) {
        return ESP_OK;
    }
    char body[256] = {};
    if (!read_body(request, body, sizeof(body))) {
        return send_json(request, "400 Bad Request", "{\"ok\":false,\"error\":\"invalid_body\"}");
    }
    cJSON* root = cJSON_Parse(body);
    const cJSON* command = root == nullptr ? nullptr : cJSON_GetObjectItemCaseSensitive(root, "command");
    if (!cJSON_IsString(command) || command->valuestring == nullptr) {
        cJSON_Delete(root);
        return send_json(request, "400 Bad Request", "{\"ok\":false,\"error\":\"invalid_field\"}");
    }
    const bool start = std::strcmp(command->valuestring, "start_cycle") == 0;
    const bool acknowledge = std::strcmp(command->valuestring, "acknowledge_fault") == 0;
    cJSON_Delete(root);
    if (start) {
        server->controller().submit_command(MachineCommand::StartCycle);
        return send_json(request, "409 Conflict", "{\"ok\":false,\"error\":\"sequence_not_implemented\"}");
    }
    if (!acknowledge) {
        return send_json(request, "400 Bad Request", "{\"ok\":false,\"error\":\"unknown_command\"}");
    }
    ControllerStatus status = {};
    server->controller().copy_status(status);
    if (status.state == MachineState::Fault && !status.drivers_ok) {
        return send_json(request, "409 Conflict", "{\"ok\":false,\"error\":\"driver_fault_latched\"}");
    }
    const Status result = server->controller().submit_command(MachineCommand::AcknowledgeFault);
    if (result != Status::Ok) {
        return send_json(request, "409 Conflict", "{\"ok\":false,\"error\":\"rejected\"}");
    }
    return send_json(request, "200 OK", "{\"ok\":true,\"motion_permitted\":false}");
}

esp_err_t diagnostics_handler(httpd_req_t* request) {
    char* body = static_cast<char*>(std::malloc(3072));
    if (body == nullptr) {
        return send_json(request, "500 Internal Server Error", "{\"ok\":false}");
    }
    std::size_t used = 0;
    used += static_cast<std::size_t>(std::snprintf(body + used, 3072 - used, "{\"ok\":true,\"clock\":\"milliseconds_since_boot\",\"events\":["));
    const std::size_t count = diagnostics_count();
    bool first = true;
    for (std::size_t index = 0; index < count && index < 16 && used + 160 < 3072; ++index) {
        DiagnosticEvent event = {};
        if (!diagnostics_at(index, event)) {
            break;
        }
        char description[64] = {};
        json_escape(description, sizeof(description), event.description);
        const int wrote = std::snprintf(
            body + used, 3072 - used, "%s{\"time_ms\":%lu,\"state\":%u,\"code\":%u,\"code_name\":\"%s\",\"weight_g\":%ld,\"description\":\"%s\"}",
            first ? "" : ",", static_cast<unsigned long>(event.time_ms), event.state, event.code,
            error_name(static_cast<ErrorCode>(event.code)), static_cast<long>(event.weight_g), description);
        if (wrote < 0) {
            break;
        }
        used += static_cast<std::size_t>(wrote);
        first = false;
    }
    if (used + 3 < 3072) {
        std::snprintf(body + used, 3072 - used, "]}");
    }
    const esp_err_t result = send_json(request, "200 OK", body);
    std::free(body);
    return result;
}

esp_err_t file_handler(httpd_req_t* request) {
    auto* file = static_cast<EmbeddedFile*>(request->user_ctx);
    httpd_resp_set_type(request, file->content_type);
    httpd_resp_set_hdr(request, "Cache-Control", "no-cache");
    return httpd_resp_send(request, file->data, static_cast<ssize_t>(file->length));
}

void remember_client(int fd) {
    if (g_ws.mutex == nullptr || xSemaphoreTake(g_ws.mutex, pdMS_TO_TICKS(20)) != pdTRUE) {
        return;
    }
    for (int index = 0; index < g_ws.count; ++index) {
        if (g_ws.fds[index] == fd) {
            xSemaphoreGive(g_ws.mutex);
            return;
        }
    }
    if (g_ws.count < 4) {
        g_ws.fds[g_ws.count++] = fd;
    }
    xSemaphoreGive(g_ws.mutex);
}

esp_err_t ws_handler(httpd_req_t* request) {
    if (request->method == HTTP_GET) {
        remember_client(httpd_req_to_sockfd(request));
        return ESP_OK;
    }
    httpd_ws_frame_t frame = {};
    frame.type = HTTPD_WS_TYPE_TEXT;
    if (httpd_ws_recv_frame(request, &frame, 0) != ESP_OK) {
        return ESP_OK;
    }
    if (frame.len == 0 || frame.len > 128) {
        return ESP_OK;
    }
    uint8_t discard[128] = {};
    frame.payload = discard;
    httpd_ws_recv_frame(request, &frame, frame.len);
    return ESP_OK;
}

void send_ws(void* context) {
    auto* server = static_cast<WebServer*>(context);
    if (server == nullptr || g_ws.server == nullptr || g_ws.mutex == nullptr) {
        return;
    }
    char body[1024] = {};
    write_status_json(*server, body, sizeof(body));
    int fds[4] = {};
    int count = 0;
    if (xSemaphoreTake(g_ws.mutex, pdMS_TO_TICKS(20)) != pdTRUE) {
        return;
    }
    count = g_ws.count;
    for (int index = 0; index < count; ++index) {
        fds[index] = g_ws.fds[index];
    }
    xSemaphoreGive(g_ws.mutex);

    int live[4] = {};
    int live_count = 0;
    for (int index = 0; index < count; ++index) {
        if (httpd_ws_get_fd_info(g_ws.server, fds[index]) != HTTPD_WS_CLIENT_WEBSOCKET) {
            continue;
        }
        httpd_ws_frame_t frame = {};
        frame.type = HTTPD_WS_TYPE_TEXT;
        frame.payload = reinterpret_cast<uint8_t*>(body);
        frame.len = std::strlen(body);
        if (httpd_ws_send_frame_async(g_ws.server, fds[index], &frame) != ESP_OK) {
            continue;
        }
        live[live_count++] = fds[index];
    }
    if (xSemaphoreTake(g_ws.mutex, pdMS_TO_TICKS(20)) == pdTRUE) {
        g_ws.count = live_count;
        for (int index = 0; index < live_count; ++index) {
            g_ws.fds[index] = live[index];
        }
        xSemaphoreGive(g_ws.mutex);
    }
}

void ws_timer(void* context) {
    if (g_ws.server != nullptr) {
        httpd_queue_work(g_ws.server, &send_ws, context);
    }
}

esp_err_t register_file(httpd_handle_t server, const char* uri, EmbeddedFile* file) {
    httpd_uri_t route = {};
    route.uri = uri;
    route.method = HTTP_GET;
    route.handler = &file_handler;
    route.user_ctx = file;
    return httpd_register_uri_handler(server, &route);
}

esp_err_t register_json(httpd_handle_t server, const char* uri, httpd_method_t method, esp_err_t (*handler)(httpd_req_t*),
                        void* context) {
    httpd_uri_t route = {};
    route.uri = uri;
    route.method = method;
    route.handler = handler;
    route.user_ctx = context;
    return httpd_register_uri_handler(server, &route);
}

Status start_access_point(const NetworkParameters& network) {
    if (esp_netif_init() != ESP_OK || esp_event_loop_create_default() != ESP_OK) {
        return Status::ProtocolError;
    }
    esp_netif_t* netif = esp_netif_create_default_wifi_ap();
    if (netif == nullptr) {
        return Status::ProtocolError;
    }
    esp_netif_dhcps_stop(netif);
    esp_netif_ip_info_t address = {};
    IP4_ADDR(&address.ip, 192, 168, 4, 1);
    IP4_ADDR(&address.gw, 192, 168, 4, 1);
    IP4_ADDR(&address.netmask, 255, 255, 255, 0);
    if (esp_netif_set_ip_info(netif, &address) != ESP_OK || esp_netif_dhcps_start(netif) != ESP_OK) {
        return Status::ProtocolError;
    }

    wifi_init_config_t init = WIFI_INIT_CONFIG_DEFAULT();
    if (esp_wifi_init(&init) != ESP_OK || esp_wifi_set_mode(WIFI_MODE_AP) != ESP_OK) {
        return Status::ProtocolError;
    }
    wifi_config_t config = {};
    const std::size_t ssid_length = text_length(network.ssid);
    const std::size_t copy_length = ssid_length > 32 ? 32 : ssid_length;
    std::memcpy(config.ap.ssid, network.ssid, copy_length);
    config.ap.ssid_len = static_cast<uint8_t>(copy_length);
    copy_text(reinterpret_cast<char*>(config.ap.password), sizeof(config.ap.password), network.wifi_password);
    config.ap.channel = 1;
    config.ap.max_connection = 2;
    config.ap.authmode = WIFI_AUTH_WPA2_PSK;
    if (esp_wifi_set_config(WIFI_IF_AP, &config) != ESP_OK || esp_wifi_start() != ESP_OK) {
        return Status::ProtocolError;
    }
    esp_wifi_set_ps(WIFI_PS_NONE);
    ESP_LOGI(kTag, "Access point %s at %s", network.ssid, hw::kApAddress);
    return Status::Ok;
}

}  // namespace

Status WebServer::start() {
    if (started_) {
        return Status::Ok;
    }
    if (start_access_point(controller_.settings().network()) != Status::Ok) {
        ESP_LOGE(kTag, "Wi-Fi access point did not start");
        return Status::ProtocolError;
    }

    static EmbeddedFile index_file;
    static EmbeddedFile settings_file;
    static EmbeddedFile calibration_file;
    static EmbeddedFile manual_file;
    static EmbeddedFile diagnostics_file;
    static EmbeddedFile css_file;
    static EmbeddedFile js_file;
    bind_file(index_file, index_html_start, index_html_end, "text/html");
    bind_file(settings_file, settings_html_start, settings_html_end, "text/html");
    bind_file(calibration_file, calibration_html_start, calibration_html_end, "text/html");
    bind_file(manual_file, manual_html_start, manual_html_end, "text/html");
    bind_file(diagnostics_file, diagnostics_html_start, diagnostics_html_end, "text/html");
    bind_file(css_file, site_css_start, site_css_end, "text/css");
    bind_file(js_file, app_js_start, app_js_end, "application/javascript");

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;
    config.max_uri_handlers = 20;
    config.max_open_sockets = 7;
    config.lru_purge_enable = true;
    config.stack_size = 8192;
    httpd_handle_t server = nullptr;
    if (httpd_start(&server, &config) != ESP_OK) {
        ESP_LOGE(kTag, "HTTP server did not start");
        return Status::ProtocolError;
    }

    register_file(server, "/", &index_file);
    register_file(server, "/settings.html", &settings_file);
    register_file(server, "/calibration.html", &calibration_file);
    register_file(server, "/manual.html", &manual_file);
    register_file(server, "/diagnostics.html", &diagnostics_file);
    register_file(server, "/site.css", &css_file);
    register_file(server, "/app.js", &js_file);
    register_json(server, "/api/status", HTTP_GET, &status_handler, this);
    register_json(server, "/api/settings", HTTP_GET, &settings_get_handler, this);
    register_json(server, "/api/settings", HTTP_POST, &settings_post_handler, this);
    register_json(server, "/api/calibration", HTTP_GET, &calibration_get_handler, this);
    register_json(server, "/api/calibration", HTTP_POST, &calibration_post_handler, this);
    register_json(server, "/api/password", HTTP_POST, &password_handler, this);
    register_json(server, "/api/network", HTTP_POST, &network_handler, this);
    register_json(server, "/api/command", HTTP_POST, &command_handler, this);
    register_json(server, "/api/diagnostics", HTTP_GET, &diagnostics_handler, this);

    httpd_uri_t websocket = {};
    websocket.uri = "/ws";
    websocket.method = HTTP_GET;
    websocket.handler = &ws_handler;
    websocket.user_ctx = this;
    websocket.is_websocket = true;
    httpd_register_uri_handler(server, &websocket);

    g_ws.server = server;
    g_ws.app = this;
    g_ws.mutex = xSemaphoreCreateMutex();
    const esp_timer_create_args_t timer_args = {
        .callback = &ws_timer,
        .arg = this,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "ws_status",
        .skip_unhandled_events = true,
    };
    esp_timer_handle_t timer = nullptr;
    if (esp_timer_create(&timer_args, &timer) == ESP_OK) {
        esp_timer_start_periodic(timer, 500 * 1000);
    }
    started_ = true;
    ESP_LOGI(kTag, "Panel available at http://%s/", hw::kApAddress);
    return Status::Ok;
}

}  // namespace feed
