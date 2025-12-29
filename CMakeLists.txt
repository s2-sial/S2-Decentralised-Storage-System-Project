cmake_minimum_required(VERSION 3.20)
project(decent_store VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#Good Warnings
add_compile_options(-Wall -Wextra -Wpedantic)

#common include folder
include_directories(${CMAKE_SOURCE_DIR}/include)

# tracker 
add_executable(tracker
	src/tracker/main.cpp
)

# peer
add_executable(peer
	src/peer/main.cpp
)

# client
add_executable(client
	src/client/main.cpp
)

#Optional: link OpenSSL if you start using SHA-256 soon
find_package(OpenSSL)
if (OpenSSL_FOUND)
	target_link_libraries(peer PRIVATE OpenSSL::Crypto)
	target_link_libraries(client PRIVATE OpenSSL::Crypto)
endif()
