cmake_minimum_required(VERSION 3.16)
project(zerodrive)

set(CMAKE_CXX_STANDARD 17)

include_directories(.)
include_directories(/usr/local/include/fuse3)

add_executable(zerodrive
        DriveAgent.cpp
        DriveAgent.h
        DriveClientAgent.cpp
        DriveClientAgent.h
        DriveServerAgent.cpp
        DriveServerAgent.h
        op.cpp
        op.h
        zerodrive.cpp
        zerodrive.h
        Protocol.h
        Protocol.cpp
        NetworkAgent.cpp
        FileOperation.h
        FileOperation.cpp
        zerodrive_common.h zerodrive_common.cpp SharedQueue.h)
