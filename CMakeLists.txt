cmake_minimum_required(VERSION 3.20)
project(decent_store VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#Good Warnings
add_compile_options(-Wall -Wextra -Wpedantic)

#common include folders
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/src)

# core library for client/GUI use
add_library(dss_core
	src/core/net/tcp.cpp
	src/core/crypto/shq256.cpp
	src/core/chunk/chunker.cpp
	src/core/manifest/manifest.cpp
	src/core/storage/storage_manager.cpp
	src/core/dht/kademlia_id.cpp
	src/core/dht/kademlia_routing_table.cpp
	src/core/dht/dht_node.cpp
	src/core/tracker/tracker_client.cpp
	src/core/peer/peer_client.cpp
	src/core/dss/dss_client.cpp
)

# tracker 
add_executable(tracker
	src/tracker/main.cpp
)
target_link_libraries(tracker PRIVATE dss_core)

# peer
add_executable(peer
	src/peer/main.cpp
)
target_link_libraries(peer PRIVATE dss_core)

# client (CLI)
add_executable(client
	src/client/main.cpp
)
target_link_libraries(client PRIVATE dss_core)

# client GUI (Qt6) — optional
set(CMAKE_AUTOMOC ON)
find_package(Qt6 QUIET COMPONENTS Widgets Network)
if (Qt6_FOUND)
	add_executable(client_gui
		src/client/main_gui.cpp
		src/client/main_window.cpp
		src/client/dss_worker.cpp
	)
	target_include_directories(client_gui PRIVATE ${CMAKE_SOURCE_DIR}/src/client)
	target_link_libraries(client_gui PRIVATE dss_core Qt6::Widgets Qt6::Network)
	message(STATUS "Qt6 found — building client_gui")
else()
	message(STATUS "Qt6 not found — skipping client_gui (set CMAKE_PREFIX_PATH to Qt to enable)")
endif()

#Optional: link OpenSSL if you start using SHA-256 soon
find_package(OpenSSL)
if (OpenSSL_FOUND)
	target_link_libraries(dss_core PUBLIC OpenSSL::Crypto)
	target_link_libraries(peer PRIVATE OpenSSL::Crypto)
	target_link_libraries(client PRIVATE OpenSSL::Crypto)
	if (TARGET client_gui)
		target_link_libraries(client_gui PRIVATE OpenSSL::Crypto)
	endif()
endif()
