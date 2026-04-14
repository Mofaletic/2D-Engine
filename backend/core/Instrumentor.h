#pragma once

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <utility>

#ifndef LANCELOT_PROFILE_ENABLED
#define LANCELOT_PROFILE_ENABLED 1
#endif

struct ProfileResult {
    std::string Name;
    long long Start = 0;
    long long ElapsedTime = 0;
    std::uint32_t ThreadID = 0;
};

class Instrumentor {
public:
    static Instrumentor& Get() {
        static Instrumentor instance;
        return instance;
    }

    void BeginSession(const std::string& name, const std::string& filepath = "profile-results.json") {
        std::lock_guard<std::mutex> lock(m_Mutex);

        if (m_CurrentSessionActive) {
            InternalEndSession();
        }

        const std::filesystem::path outputPath(filepath);
        if (outputPath.has_parent_path()) {
            std::filesystem::create_directories(outputPath.parent_path());
        }

        m_OutputStream.open(outputPath, std::ios::out | std::ios::trunc);
        if (!m_OutputStream.is_open()) {
            return;
        }

        m_CurrentSessionName = name;
        m_CurrentSessionActive = true;
        m_ProfileCount = 0;
        WriteHeader();
    }

    void EndSession() {
        std::lock_guard<std::mutex> lock(m_Mutex);
        InternalEndSession();
    }

    void WriteProfile(const ProfileResult& result) {
        std::lock_guard<std::mutex> lock(m_Mutex);
        if (!m_CurrentSessionActive || !m_OutputStream.is_open()) {
            return;
        }

        if (m_ProfileCount++ > 0) {
            m_OutputStream << ',';
        }

        std::string safeName = result.Name;
        std::replace(safeName.begin(), safeName.end(), '"', '\'');

        m_OutputStream
            << '{'
            << "\"cat\":\"function\","
            << "\"dur\":" << result.ElapsedTime << ','
            << "\"name\":\"" << safeName << "\","
            << "\"ph\":\"X\","
            << "\"pid\":0,"
            << "\"tid\":" << result.ThreadID << ','
            << "\"ts\":" << result.Start
            << '}';
        m_OutputStream.flush();
    }

private:
    void WriteHeader() {
        m_OutputStream << "{\"otherData\":{},\"displayTimeUnit\":\"ms\",\"traceEvents\":[";
        m_OutputStream.flush();
    }

    void WriteFooter() {
        m_OutputStream << "]}";
        m_OutputStream.flush();
    }

    void InternalEndSession() {
        if (!m_CurrentSessionActive) {
            return;
        }

        if (m_OutputStream.is_open()) {
            WriteFooter();
            m_OutputStream.close();
        }

        m_CurrentSessionName.clear();
        m_CurrentSessionActive = false;
        m_ProfileCount = 0;
    }

private:
    std::mutex m_Mutex;
    std::ofstream m_OutputStream;
    std::string m_CurrentSessionName;
    int m_ProfileCount = 0;
    bool m_CurrentSessionActive = false;
};

class InstrumentationTimer {
public:
    explicit InstrumentationTimer(std::string name)
        : m_Name(std::move(name)),
          m_StartTimepoint(std::chrono::steady_clock::now()) {
    }

    ~InstrumentationTimer() {
        Stop();
    }

    void Stop() {
        if (m_Stopped) {
            return;
        }

        const auto endTimepoint = std::chrono::steady_clock::now();
        const auto start = std::chrono::time_point_cast<std::chrono::microseconds>(m_StartTimepoint).time_since_epoch().count();
        const auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(endTimepoint - m_StartTimepoint).count();
        const std::uint32_t threadId = static_cast<std::uint32_t>(std::hash<std::thread::id>{}(std::this_thread::get_id()));

        Instrumentor::Get().WriteProfile({ m_Name, start, elapsed, threadId });
        m_Stopped = true;
    }

private:
    std::string m_Name;
    std::chrono::time_point<std::chrono::steady_clock> m_StartTimepoint;
    bool m_Stopped = false;
};

#if LANCELOT_PROFILE_ENABLED
    #if defined(_MSC_VER)
        #define LANCELOT_FUNC_SIG __FUNCSIG__
    #else
        #define LANCELOT_FUNC_SIG __PRETTY_FUNCTION__
    #endif

    #define PROFILE_BEGIN_SESSION(name, filepath) ::Instrumentor::Get().BeginSession(name, filepath)
    #define PROFILE_END_SESSION() ::Instrumentor::Get().EndSession()
    #define PROFILE_SCOPE(name) ::InstrumentationTimer timer##__LINE__(name)
    #define PROFILE_FUNCTION() PROFILE_SCOPE(LANCELOT_FUNC_SIG)
#else
    #define PROFILE_BEGIN_SESSION(name, filepath)
    #define PROFILE_END_SESSION()
    #define PROFILE_SCOPE(name)
    #define PROFILE_FUNCTION()
#endif
