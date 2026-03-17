cmake_minimum_required(VERSION 3.16)
project(SDLTest)

if (POLICY CMP0141)
    cmake_policy(SET CMP0141 NEW)
    set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT
        "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,
        $<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,
        $<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 编译SDL3/SDL_image源码（保留你的原有逻辑）
add_subdirectory(external/SDL)
set(SDLIMAGE_VENDORED OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_SAMPLES OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory(external/SDL_image)

# 生成可执行文件
add_executable(SDLTest
   "include/SDLTest.cpp"
   "src/SDLTest.h"
)

# 链接SDL3库
target_link_libraries(SDLTest PRIVATE
    SDL3::SDL3
    SDL3_image::SDL3_image
)

# ========== 关键：删除所有复制DLL/asset的代码！ ==========
# 完全跳过复制，只做编译链接