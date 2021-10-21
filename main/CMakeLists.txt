
set(component_srcs  "main.c"
                    "user_esp32_hardware.c"
                    "user_esp32_i2c.c"
                    "user_esp32_modbus.c"
                    "user_esp32_mqtt.c"
                    "user_esp32_ota.c"
                    "user_esp32_pwm.c"
                    "user_esp32_rmt.c"
                    "user_esp32_uart.c"
                    "user_esp32_wifi.c")

# set(include_dirs ".")

# $(PROJECT_PATH)

idf_component_register(SRCS "${component_srcs}"
                       INCLUDE_DIRS "include"
                       EMBED_TXTFILES ${project_dir}/server_certs/ota_ca_cert.pem
                       EMBED_TXTFILES ${project_dir}/server_certs/mqtt_ca_cert.pem)
