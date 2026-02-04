cmake_minimum_required(VERSION 3.16)
project(elastisim)

set(CMAKE_CXX_STANDARD 17)

find_package(cppzmq REQUIRED)
find_package(PkgConfig)
pkg_check_modules(simgrid REQUIRED IMPORTED_TARGET simgrid)

add_executable(elastisim
    main.cpp
    src/system/SimulationEngine.cpp
    src/software/Job.cpp
    src/system/Scheduler.cpp
    src/system/Node.cpp
    src/util/Utility.cpp
    src/ElastiSim.cpp
    src/system/PeriodicInvoker.cpp
    src/software/Workload.cpp
    src/system/PlatformManager.cpp
    src/tasks/Task.cpp
    src/tasks/BusyWaitTask.cpp
    src/tasks/CombinedTask.cpp
    src/tasks/PfsReadTask.cpp
    src/tasks/BurstBufferWriteTask.cpp
    src/tasks/PfsWriteTask.cpp
    src/system/Sensing.cpp
    src/system/JobSubmitter.cpp
    src/software/Application.cpp
    src/system/WalltimeMonitor.cpp
    src/tasks/IoTask.cpp
    src/tasks/BurstBufferReadTask.cpp
    src/tasks/SequenceTask.cpp
    src/software/Phase.cpp
    src/interface/SchedulingInterface.cpp
    src/system/messages/SimMsg.cpp
    src/system/messages/SchedMsg.cpp
    src/util/Configuration.cpp
    src/tasks/CombinedGpuTask.cpp
    src/system/Gpu.cpp
    src/tasks/IdleTask.cpp
    src/tasks/DelayTask.cpp
    src/tasks/CombinedCpuTask.cpp
    src/tasks/AsyncSleep.cpp
)

target_include_directories(elastisim PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/src/interface
    ${PROJECT_SOURCE_DIR}/src/software
    ${PROJECT_SOURCE_DIR}/src/system
    ${PROJECT_SOURCE_DIR}/src/system/messages
    ${PROJECT_SOURCE_DIR}/src/tasks
    ${PROJECT_SOURCE_DIR}/src/util
    ${PROJECT_SOURCE_DIR}/third-party/exprtk
    ${PROJECT_SOURCE_DIR}/third-party/nlohmann_json
    ${PROJECT_SOURCE_DIR}/third-party/indicators
)

target_link_libraries(elastisim PRIVATE cppzmq)
target_link_libraries(elastisim PRIVATE PkgConfig::simgrid)
