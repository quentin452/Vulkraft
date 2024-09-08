#pragma once
#include <algorithm>
#include <chrono>
#include <iostream>
#include <mutex>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

#include <ThreadedLoggerForCPP/LoggerThread.hpp>

#include <ThreadedLoggerForCPP/LoggerFileSystem.hpp>
#include <ThreadedLoggerForCPP/LoggerGlobals.hpp>

// COPYRIGHT MIT : created by https://github.com/quentin452
class GamePerformanceProfiler {
  public:
    GamePerformanceProfiler() {
#ifdef PERFORMANCE_MONITOR
        realTimeStart = std::chrono::steady_clock::now();
#endif
    }

  private:
#ifdef PERFORMANCE_MONITOR
    std::unordered_map<std::string, std::chrono::steady_clock::time_point> startTimes;
    std::unordered_map<std::string, int64_t> profilingData;
    std::unordered_map<std::string, int> callCounts;
    std::unordered_map<std::string, int64_t> maxTimes;
    std::mutex mtx;
    std::chrono::steady_clock::time_point realTimeStart;
    std::vector<std::string> warningsIssued;
#endif
  public:
#ifdef PERFORMANCE_MONITOR
    bool stress_test_enabled = true;
#endif
    void start(const std::string &name, const std::string &file, int line, const std::string &customName) {
#ifdef PERFORMANCE_MONITOR
        auto now = std::chrono::steady_clock::now();
        std::string key = customName.empty() ? (name + ":" + file + ":" + std::to_string(line)) : customName;

        {
            std::lock_guard<std::mutex> lock(mtx);
            startTimes[key] = now;
            callCounts[key]++;
        }
#endif
    }

    void stop(const std::string &name, const std::string &file, int line, const std::string &customName) {
#ifdef PERFORMANCE_MONITOR
        auto now = std::chrono::steady_clock::now();
        std::string key = customName.empty() ? (name + ":" + file + ":" + std::to_string(line)) : customName;

        {
            std::lock_guard<std::mutex> lock(mtx);
            auto it = startTimes.find(key);
            if (it != startTimes.end()) {
                auto duration = std::chrono::duration_cast<std::chrono::microseconds>(now - it->second).count();
                profilingData[key] += duration;
                maxTimes[key] = std::max(maxTimes[key], duration);
                startTimes.erase(it);
            } else {
                std::string warningMessage = "Warning: Profiling stopped for a method that was not started: " + key;
                if (std::find(warningsIssued.begin(), warningsIssued.end(), warningMessage) == warningsIssued.end()) {
                    warningsIssued.push_back(warningMessage);
                }
            }
        }
#endif
    }

    void printWithoutLogMessageAsync(const std::string &filePath) {
#ifdef PERFORMANCE_MONITOR
        try {
            if (profilingData.empty()) {
                std::ofstream file(filePath, std::ios::app);
                if (file.is_open()) {
                    file << "Warning: profilingData is empty." << std::endl;
                    file.close();
                }
                return;
            }

            double totalProfilingTimeSec = 0;
            std::vector<std::tuple<std::string, double, double, double, double>> sortedData;

            for (const auto &entry : profilingData) {
                std::string key = entry.first;
                double totalTimeSec = entry.second / 1000000.0;
                double avgTimeSec = totalTimeSec / callCounts.at(key);
                double maxTimeSec = maxTimes.at(key) / 1000000.0;
                double combinedAvg = (avgTimeSec + maxTimeSec) / 2.0;
                sortedData.push_back(std::make_tuple(key, totalTimeSec, avgTimeSec, maxTimeSec, combinedAvg));

                totalProfilingTimeSec += totalTimeSec;
            }

            std::sort(sortedData.begin(), sortedData.end(), [](const auto &a, const auto &b) {
                return std::get<1>(b) < std::get<1>(a);
            });

            auto now = std::chrono::steady_clock::now();
            auto realTimeElapsed = std::chrono::duration_cast<std::chrono::seconds>(now - realTimeStart).count();

            std::ofstream file(filePath, std::ios::app);
            if (file.is_open()) {
                file << "Total Profiling Time: " + std::to_string(totalProfilingTimeSec) + " s, Real Time Elapsed: " + std::to_string(realTimeElapsed) + " s" << std::endl;

                for (const auto &entry : sortedData) {
                    std::string key = std::get<0>(entry);
                    double totalTimeSec = std::get<1>(entry);
                    double avgTimeSec = std::get<2>(entry);
                    double maxTimeSec = std::get<3>(entry);
                    std::string logMessage = key + ": " + std::to_string(totalTimeSec) + " s (Total Time), " + std::to_string(avgTimeSec) + " s (Average Time For one call), " + std::to_string(maxTimeSec) + " s (Max Time For one call)" + " With a Total Calls: " + std::to_string(callCounts.at(key));
                    file << logMessage << std::endl;
                }

                for (const std::string &warningMessage : warningsIssued) {
                    file << "Warning: " << warningMessage << std::endl;
                }
                warningsIssued.clear();

                file.close();
            }
        } catch (const std::exception &e) {
            std::ofstream file(filePath, std::ios::app);
            if (file.is_open()) {
                file << "Error: " << e.what() << std::endl;
                file.close();
            }
        }
#endif
    }

    void printWithLogMessageAsync() {
#ifdef PERFORMANCE_MONITOR
        try {
            if (profilingData.empty()) {
                LoggerThread::GetLoggerThread().logMessageAsync(
                    LogLevel::WARNING, __FILE__, __LINE__, "Warning: profilingData is empty.");
                return;
            }

            double totalProfilingTimeSec = 0;
            std::vector<std::tuple<std::string, double, double, double, double>> sortedData;

            for (const auto &entry : profilingData) {
                std::string key = entry.first;
                double totalTimeSec = entry.second / 1000000.0;
                double avgTimeSec = totalTimeSec / callCounts.at(key);
                double maxTimeSec = maxTimes.at(key) / 1000000.0;
                double combinedAvg = (avgTimeSec + maxTimeSec) / 2.0;
                sortedData.push_back(std::make_tuple(key, totalTimeSec, avgTimeSec, maxTimeSec, combinedAvg));

                totalProfilingTimeSec += totalTimeSec;
            }

            std::sort(sortedData.begin(), sortedData.end(), [](const auto &a, const auto &b) {
                return std::get<1>(b) < std::get<1>(a);
            });

            auto now = std::chrono::steady_clock::now();
            auto realTimeElapsed = std::chrono::duration_cast<std::chrono::seconds>(now - realTimeStart).count();

            std::string totalLogMessage = "Total Profiling Time: " + std::to_string(totalProfilingTimeSec) + " s, Real Time Elapsed: " + std::to_string(realTimeElapsed) + " s";
            LoggerThread::GetLoggerThread().logMessageAsync(LogLevel::INFO, __FILE__, __LINE__, totalLogMessage);

            for (const auto &entry : sortedData) {
                std::string key = std::get<0>(entry);
                double totalTimeSec = std::get<1>(entry);
                double avgTimeSec = std::get<2>(entry);
                double maxTimeSec = std::get<3>(entry);
                std::string logMessage = key + ": " + std::to_string(totalTimeSec) + " s (Total Time), " + std::to_string(avgTimeSec) + " s (Average Time For one call), " + std::to_string(maxTimeSec) + " s (Max Time For one call)" + " With a Total Calls: " + std::to_string(callCounts.at(key));
                LoggerThread::GetLoggerThread().logMessageAsync(LogLevel::INFO, __FILE__, __LINE__, logMessage);
            }
            for (const std::string &warningMessage : warningsIssued) {
                LoggerThread::GetLoggerThread().logMessageAsync(LogLevel::WARNING, __FILE__, __LINE__, warningMessage);
            }
            warningsIssued.clear();
        } catch (const std::exception &e) {
            LoggerThread::GetLoggerThread().logMessageAsync(LogLevel::ERRORING, __FILE__, __LINE__, e.what());
        }
#endif
    }

    void addData(const std::string &key, int64_t value) {
#ifdef PERFORMANCE_MONITOR
        std::lock_guard<std::mutex> lock(mtx);
        profilingData[key] += value;
#endif
    }
};

inline GamePerformanceProfiler gamePerformanceProfiler;

#ifdef PERFORMANCE_MONITOR
#define PROFILE_START(customName) gamePerformanceProfiler.start(__FUNCTION__, __FILE__, __LINE__, customName);

#define PROFILE_STOP(customName) gamePerformanceProfiler.stop(__FUNCTION__, __FILE__, __LINE__, customName);

class ScopedProfiler {
  public:
    ScopedProfiler(const std::string &name) : name(name) {
        PROFILE_START(name);
    }
    ~ScopedProfiler() {
        PROFILE_STOP(name);
    }

  private:
    std::string name;
};

#define PROFILE_SCOPED(customName) ScopedProfiler profiler(customName);
#else
#define PROFILE_START(customName)
#define PROFILE_STOP(customName)
#define PROFILE_SCOPED(customName)
#endif
