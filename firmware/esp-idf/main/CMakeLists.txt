set(R ${CMAKE_CURRENT_LIST_DIR}/../../..)
idf_component_register(
    SRCS main.c
         ${R}/common/dudect.c
         ${R}/impl/variants.c ${R}/impl/dipper.c ${R}/impl/dipper_fast.c
         ${R}/impl/dipper_fast_const.c ${R}/impl/dipper_ct.c ${R}/impl/dipper_leaky.c
    INCLUDE_DIRS . ${R}/common ${R}/impl
    PRIV_REQUIRES esp_wifi nvs_flash esp_timer)
# same flags for all variants: -O2 (CONFIG_COMPILER_OPTIMIZATION_PERF), no FMA contraction
target_compile_options(${COMPONENT_LIB} PRIVATE -ffp-contract=off -Wno-unused-function)
