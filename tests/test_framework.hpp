#pragma once

// =============================================================================
// IBM IMS Emulation Enterprise - Simple Test Framework
// Version: 3.6.2
// =============================================================================

#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <sstream>

namespace ims::test {

/**
 * @brief Simple test framework (no external dependencies)
 */
class TestRunner {
private:
    struct TestCase {
        std::string name;
        std::function<bool()> func;
    };
    
    std::vector<TestCase> tests_;
    std::string suite_name_;
    int passed_{0};
    int failed_{0};
    
public:
    explicit TestRunner(const std::string& suite_name) : suite_name_(suite_name) {}
    
    void add_test(const std::string& name, std::function<bool()> func) {
        tests_.push_back({name, func});
    }
    
    int run() {
        std::cout << "================================" << std::endl;
        std::cout << "Test Suite: " << suite_name_ << std::endl;
        std::cout << "================================" << std::endl;
        
        for (const auto& test : tests_) {
            std::cout << "  [RUN ] " << test.name << std::endl;
            try {
                bool result = test.func();
                if (result) {
                    std::cout << "  [PASS] " << test.name << std::endl;
                    ++passed_;
                } else {
                    std::cout << "  [FAIL] " << test.name << std::endl;
                    ++failed_;
                }
            } catch (const std::exception& e) {
                std::cout << "  [FAIL] " << test.name << " (Exception: " << e.what() << ")" << std::endl;
                ++failed_;
            } catch (...) {
                std::cout << "  [FAIL] " << test.name << " (Unknown exception)" << std::endl;
                ++failed_;
            }
        }
        
        std::cout << "--------------------------------" << std::endl;
        std::cout << "Results: " << passed_ << " passed, " << failed_ << " failed" << std::endl;
        std::cout << "================================" << std::endl;
        
        return failed_ > 0 ? 1 : 0;
    }
};

// Assertion macros
#define TEST_ASSERT(condition) \
    do { \
        if (!(condition)) { \
            std::cerr << "    Assertion failed: " #condition << std::endl; \
            std::cerr << "    At: " << __FILE__ << ":" << __LINE__ << std::endl; \
            return false; \
        } \
    } while(0)

#define TEST_ASSERT_EQ(expected, actual) \
    do { \
        if ((expected) != (actual)) { \
            std::cerr << "    Assertion failed: " #expected " == " #actual << std::endl; \
            std::cerr << "    Expected: " << (expected) << std::endl; \
            std::cerr << "    Actual: " << (actual) << std::endl; \
            std::cerr << "    At: " << __FILE__ << ":" << __LINE__ << std::endl; \
            return false; \
        } \
    } while(0)

#define TEST_ASSERT_NE(expected, actual) \
    do { \
        if ((expected) == (actual)) { \
            std::cerr << "    Assertion failed: " #expected " != " #actual << std::endl; \
            std::cerr << "    At: " << __FILE__ << ":" << __LINE__ << std::endl; \
            return false; \
        } \
    } while(0)

#define TEST_ASSERT_TRUE(condition) TEST_ASSERT(condition)
#define TEST_ASSERT_FALSE(condition) TEST_ASSERT(!(condition))

#define TEST_ASSERT_THROWS(expression, exception_type) \
    do { \
        bool caught = false; \
        try { \
            expression; \
        } catch (const exception_type&) { \
            caught = true; \
        } catch (...) { \
            std::cerr << "    Wrong exception type thrown" << std::endl; \
            return false; \
        } \
        if (!caught) { \
            std::cerr << "    Expected exception not thrown: " #exception_type << std::endl; \
            return false; \
        } \
    } while(0)

} // namespace ims::test
