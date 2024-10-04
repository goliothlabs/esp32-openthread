set(esp_idf_common "../submodules/golioth-firmware-sdk/examples/esp_idf/common")
set(examples_common "../submodules/golioth-firmware-sdk/examples/common")

set(includes
    "."
    "${esp_idf_common}"
    "${examples_common}"
)

set(srcs
    "app_main.c"
    "app_settings.c"
    "app_rpc.c"
    "${esp_idf_common}/shell.c"
    "${esp_idf_common}/nvs.c"
    "${esp_idf_common}/sample_credentials.c"
)

set(deps
    "golioth_sdk"
    "console"
    "spi_flash"
    "nvs_flash"
    "json"
    "driver"
    "esp_hw_support"
    "esp_event"
    "esp_netif"
    "esp_coex"
    "openthread"
)

idf_component_register(
    INCLUDE_DIRS "${includes}"
    SRCS "${srcs}"
    PRIV_REQUIRES "${deps}"
)

list(APPEND EXTRA_C_FLAGS_LIST
    -Werror
)

component_compile_options(${EXTRA_C_FLAGS_LIST})
