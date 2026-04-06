set(SPRITE_PLAYER_SRCS
    ${CMAKE_SOURCE_DIR}/assets/spirites/player/p1_walk01.c
    ${CMAKE_SOURCE_DIR}/assets/spirites/player/p1_walk02.c
    ${CMAKE_SOURCE_DIR}/assets/spirites/player/p1_walk03.c
    ${CMAKE_SOURCE_DIR}/assets/spirites/player/p1_walk04.c
    ${CMAKE_SOURCE_DIR}/assets/spirites/player/p1_walk05.c
    ${CMAKE_SOURCE_DIR}/assets/spirites/player/p1_walk06.c
    ${CMAKE_SOURCE_DIR}/assets/spirites/player/p1_walk07.c
    ${CMAKE_SOURCE_DIR}/assets/spirites/player/p1_walk08.c
    ${CMAKE_SOURCE_DIR}/assets/spirites/player/p1_walk09.c
    ${CMAKE_SOURCE_DIR}/assets/spirites/player/p1_walk10.c
)

set(SPRITE_ENEMY_SRCS
    ${CMAKE_SOURCE_DIR}/assets/spirites/enemys/slimeWalk1.c
    ${CMAKE_SOURCE_DIR}/assets/spirites/enemys/slimeWalk2.c
    ${CMAKE_SOURCE_DIR}/assets/spirites/enemys/slimeDead.c
    ${CMAKE_SOURCE_DIR}/assets/spirites/enemys/fishSwim1.c
    ${CMAKE_SOURCE_DIR}/assets/spirites/enemys/fishSwim2.c
    ${CMAKE_SOURCE_DIR}/assets/spirites/enemys/fishDead.c
    ${CMAKE_SOURCE_DIR}/assets/spirites/enemys/flyFly1.c
    ${CMAKE_SOURCE_DIR}/assets/spirites/enemys/flyFly2.c
    ${CMAKE_SOURCE_DIR}/assets/spirites/enemys/flyDead.c
    ${CMAKE_SOURCE_DIR}/assets/spirites/enemys/blockerBody.c
    ${CMAKE_SOURCE_DIR}/assets/spirites/enemys/blockerMad.c
    ${CMAKE_SOURCE_DIR}/assets/spirites/enemys/blockerSad.c
    ${CMAKE_SOURCE_DIR}/assets/spirites/enemys/pokerMad.c
    ${CMAKE_SOURCE_DIR}/assets/spirites/enemys/pokerSad.c
)

idf_component_register(
SRCS
"events.c"
"game_coordinator.c"
"workers.c"
"storage_task.c"
"display_task.c"
"screens.c"
"usb_init.c"
"lcd_init.c"
"rules.c"
"sprites.c"
"spi_bus.c"
"combat_engine.c"
"animation_system.c"
"animation_loops.c"
${SPRITE_PLAYER_SRCS}
${SPRITE_ENEMY_SRCS}
INCLUDE_DIRS "."
PRIV_REQUIRES esp_lcd spi_flash sdmmc fatfs freertos driver esp_timer cjson lvgl esp_driver_usb_serial_jtag
)

target_compile_definitions(${COMPONENT_LIB} PRIVATE LV_LVGL_H_INCLUDE_SIMPLE=1)
