# Define target name
set (TARGET_NAME Urho3D)

# Define source files
file (GLOB CPP_FILES *.cpp)
file (GLOB H_FILES *.h)
set (SOURCE_FILES ${CPP_FILES} ${H_FILES})

# Include directories
include_directories (
    ../Engine/Core ../Engine/Engine ../Engine/IO ../Engine/Math ../Engine/Resource ../Engine/Script
)

# Define target & libraries to link
add_executable (${TARGET_NAME} WIN32 ${SOURCE_FILES})
set_target_properties (${TARGET_NAME} PROPERTIES DEBUG_POSTFIX _d)
target_link_libraries (${TARGET_NAME} Core Engine IO Math Resource Script ${DBGHELP_LIB})
finalize_exe ()