add_custom_target(make_shaders ${CMAKE_COMMAND} make_directory $<TARGET_FILE_DIR:Basic_Tests/assets/shaders)
execute_process(COMMAND glslc ${CMAKE_SOURCE_DIR}/assets/shaders/fragment.frag -o assets/shaders/frag.spv)
execute_process(COMMAND glslc ${CMAKE_SOURCE_DIR}/assets/shaders/vertex.vert -o assets/shaders/vert.spv)