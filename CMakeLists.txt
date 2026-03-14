idf_component_register(
    SRC_DIRS "src"
    INCLUDE_DIRS "src"
    PRIV_REQUIRES esp_lcd
    REQUIRES arduino lvgl
)

add_compile_options("-Wno-missing-field-initializers")
