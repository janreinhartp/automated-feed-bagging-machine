idf_component_register(
    SRCS "src/web_server.cpp"
    INCLUDE_DIRS "include"
    REQUIRES machine_controller esp_http_server esp_wifi esp_netif esp_event esp_timer json
    EMBED_TXTFILES
        "../../web/index.html"
        "../../web/settings.html"
        "../../web/calibration.html"
        "../../web/manual.html"
        "../../web/diagnostics.html"
        "../../web/css/site.css"
        "../../web/js/app.js"
)
