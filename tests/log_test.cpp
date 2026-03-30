#include <gtest/gtest.h>
#include "rix/util/log.hpp"

using namespace rix;

TEST(LogTest, NullLineDoesNotCrash) {
    Log::set_log_level(Log::FATAL);
    // All of these are below FATAL, so they should return inactive Lines
    Log::debugv << "should be discarded";
    Log::debug  << "should be discarded";
    Log::info   << "should be discarded";
    Log::warn   << "should be discarded";
    Log::error  << "should be discarded";
}

TEST(LogTest, ActiveLineDoesNotCrash) {
    Log::set_log_level(Log::DEBUGV);
    Log::debugv << "debugv message";
    Log::debug  << "debug message";
    Log::info   << "info message";
    Log::warn   << "warn message";
    Log::error  << "error message";
    Log::fatal  << "fatal message";
}

TEST(LogTest, ChainingOperators) {
    Log::set_log_level(Log::INFO);
    // Active: chain multiple values
    Log::info << "count=" << 42 << " flag=" << true;
    // Inactive: chain should still compile and not crash
    Log::debug << "count=" << 42 << " flag=" << true;
}

TEST(LogTest, StreamManipulators) {
    Log::set_log_level(Log::INFO);
    Log::info << "with endl" << std::endl;
    Log::debug << "suppressed endl" << std::endl;
}

TEST(LogTest, InitSetsName) {
    Log::init("test_logger");
    Log::set_log_level(Log::INFO);
    Log::info << "after init";
}

TEST(LogTest, SetLogLevelFilters) {
    // At WARN level, INFO should be inactive, WARN should be active
    Log::set_log_level(Log::WARN);
    Log::info << "should be inactive";
    Log::warn << "should be active";
    Log::error << "should be active";
    Log::fatal << "should be active";
}
