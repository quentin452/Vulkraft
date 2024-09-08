#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <functional>
#include <iostream>
#include <limits>
#include <mutex>
#include <optional>
#include <queue>
#include <set>
#include <stdexcept>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "camera/camera.hpp"
#include "chunk/chunk.hpp"
#include "chunk/chunkDrawing.h"

#include "VulkraftApplication.h"

#include "player/player.hpp"
#include "raycast/trace-ray.hpp"

#include <ThreadedLoggerForCPP/LoggerThread.hpp>

#include <ThreadedLoggerForCPP/LoggerFileSystem.hpp>
#include <ThreadedLoggerForCPP/LoggerGlobals.hpp>

#include <game_performance_profiler.hpp>

#include "globals.h"

void InitThreadedLoggerForCPP(const std::string &ProjectDirectory,
                              const std::string &LogFileName,
                              const std::string &GameSaveFolder) {
  PROFILE_SCOPED(std::string("Vulkraft:") + ":" + __FUNCTION__)
#pragma warning(push)
#pragma warning(disable : 4996) // Disable warning for getenv
#ifdef _WIN32
  LoggerGlobals::UsernameDirectory = std::getenv("USERNAME");
#else
  LoggerGlobals::UsernameDirectory = std::getenv("USER");
#endif
#pragma warning(pop)
  LoggerGlobals::SrcProjectDirectory = ProjectDirectory;
  LoggerGlobals::LogFolderPath = "C:\\Users\\" +
                                 LoggerGlobals::UsernameDirectory + "\\." +
                                 GameSaveFolder + "\\logging\\";
  LoggerGlobals::LogFilePath =
      "C:\\Users\\" + LoggerGlobals::UsernameDirectory + "\\." +
      GameSaveFolder + "\\logging\\" + LogFileName + ".log";
  LoggerGlobals::LogFolderBackupPath =
      "C:\\Users\\" + LoggerGlobals::UsernameDirectory + "\\." +
      GameSaveFolder + "\\logging\\LogBackup";
  LoggerGlobals::LogFileBackupPath =
      "C:\\Users\\" + LoggerGlobals::UsernameDirectory + "\\." +
      GameSaveFolder + "\\logging\\LogBackup\\" + LogFileName + "-";
  LoggerThread::GetLoggerThread().StartLoggerThread(
      LoggerGlobals::LogFolderPath, LoggerGlobals::LogFilePath,
      LoggerGlobals::LogFolderBackupPath, LoggerGlobals::LogFileBackupPath);
}

int main() {
  PROFILE_SCOPED(std::string("Vulkraft:") + ":" + __FUNCTION__)
  InitThreadedLoggerForCPP(Globals::GAME_TITLE, Globals::GAME_TITLE,
                           Globals::GAME_TITLE);
#ifdef DEBUG
  LOGGER_THREAD(LogLevel::INFO, "Starting Catz Voxel Engine in Debug Mode...");
#else
  LOGGER_THREAD(LogLevel::INFO,
                "Starting Catz Voxel Engine in Release Mode...");
#endif
  srand(time(NULL));

  try {
    Globals::vulkraftApp.run();
  } catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }
  gamePerformanceProfiler.printWithLogMessageAsync();
  LOGGER_THREAD(LogLevel::INFO, "Exited correctly Catz Voxel Engine")
  LoggerThread::GetLoggerThread().ExitLoggerThread();
  return EXIT_SUCCESS;
}