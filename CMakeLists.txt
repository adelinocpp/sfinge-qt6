cmake_minimum_required(VERSION 3.16)

project(SFINGE-Qt6 VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

set(CORE_SOURCES
    src/core/math_utils.h
    src/core/shape_generator.h
    src/core/shape_generator.cpp
    src/core/density_generator.h
    src/core/density_generator.cpp
    src/core/orientation_generator.h
    src/core/orientation_generator.cpp
    src/core/fomfe_orientation_generator.h
    src/core/fomfe_orientation_generator.cpp
    src/core/orientation_smoother.h
    src/core/orientation_smoother.cpp
    src/core/gabor_filter.h
    src/core/gabor_filter.cpp
    src/core/ridge_generator.h
    src/core/ridge_generator.cpp
    src/core/fingerprint_generator.h
    src/core/fingerprint_generator.cpp
    src/core/fingerprint_worker.cpp
    src/core/batch_generator.h
    src/core/batch_generator.cpp
    src/core/batch_worker.cpp
    src/core/minutiae_generator.h
    src/core/minutiae_generator.cpp
    # Módulo 2: Rendering Avançado
    src/core/rendering/perlin_noise.h
    src/core/rendering/perlin_noise.cpp
    src/core/rendering/texture_renderer.h
    src/core/rendering/texture_renderer.cpp
    # Módulo 3: Variação e Distorção
    src/core/variation/variation_effects.h
    src/core/variation/variation_effects.cpp
    # Módulo 4: Controle de Minutiae Melhorado
    src/core/phase_field_generator.h
    src/core/phase_field_generator.cpp
    src/core/quality_mask_generator.h
    src/core/quality_mask_generator.cpp
    src/core/frequency_field_smoother.h
    src/core/frequency_field_smoother.cpp
)

set(MODEL_SOURCES
    src/models/fingerprint_parameters.h
    src/models/fingerprint_parameters.cpp
    src/models/singular_points.h
    src/models/singular_points.cpp
)

set(UI_SOURCES
    src/ui/mainwindow.h
    src/ui/mainwindow.cpp
    src/ui/mainwindow.ui
    src/ui/widgets/shape_control_widget.h
    src/ui/widgets/shape_control_widget.cpp
    src/ui/widgets/density_control_widget.h
    src/ui/widgets/density_control_widget.cpp
    src/ui/widgets/orientation_control_widget.h
    src/ui/widgets/orientation_control_widget.cpp
    src/ui/widgets/preview_widget.h
    src/ui/widgets/preview_widget.cpp
    src/ui/dialogs/export_dialog.h
    src/ui/dialogs/export_dialog.cpp
    src/ui/dialogs/parameters_dialog.h
    src/ui/dialogs/parameters_dialog.cpp
    src/ui/dialogs/batch_dialog.h
    src/ui/dialogs/batch_dialog.cpp
)

set(UTIL_SOURCES
    src/utils/image_converter.h
    src/utils/image_converter.cpp
    src/utils/file_handler.h
    src/utils/file_handler.cpp
)

set(RESOURCES
    resources/resources.qrc
)

add_executable(sfinge-qt6
    src/main.cpp
    ${CORE_SOURCES}
    ${MODEL_SOURCES}
    ${UI_SOURCES}
    ${UTIL_SOURCES}
    ${RESOURCES}
)

target_link_libraries(sfinge-qt6 PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)

target_include_directories(sfinge-qt6 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

if(UNIX AND NOT APPLE)
    set_target_properties(sfinge-qt6 PROPERTIES
        OUTPUT_NAME "sfinge"
    )
endif()

if(WIN32)
    set_target_properties(sfinge-qt6 PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()

if(APPLE)
    set_target_properties(sfinge-qt6 PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.sfinge.qt6"
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    )
endif()

enable_testing()
add_subdirectory(tests)

install(TARGETS sfinge-qt6
    BUNDLE DESTINATION .
    RUNTIME DESTINATION bin
)

# CLI-only version (uses Qt Gui for QImage) - DESABILITADO TEMPORARIAMENTE
# CLI deve usar o projeto sfinge-cli-pure independente
# add_executable(sfinge-cli
#     src/cli_main.cpp
#     src/core/batch_generator.cpp
#     ${CORE_SOURCES}
#     ${MODEL_SOURCES}
# )

# target_link_libraries(sfinge-cli
#     Qt6::Core
#     Qt6::Gui
# )

# target_include_directories(sfinge-cli PRIVATE
#     ${CMAKE_CURRENT_SOURCE_DIR}/src
# )

# if(UNIX AND NOT APPLE)
#     set_target_properties(sfinge-cli PROPERTIES
#         OUTPUT_NAME "sfinge-cli"
#     )
# endif()

# install(TARGETS sfinge-cli
#     RUNTIME DESTINATION bin
# )

# CLI puro deve ser um projeto independente
# Removido do projeto principal para evitar conflitos
# TODO: Criar diretório sfinge-cli-pure separado com sua própria estrutura
