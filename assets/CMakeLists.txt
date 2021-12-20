add_custom_target(make_assets_dir ${CMAKE_COMMAND} make_directory $<TARGET_FILE_DIR:Basic_Tests/assets)

add_subdirectory(shaders)