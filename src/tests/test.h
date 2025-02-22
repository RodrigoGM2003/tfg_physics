#ifndef TEST_H
#define TEST_H

#pragma once

#include <functional>
#include <string>
#include <iostream>

namespace test {
    
    class Test {
    public:
        /**
         * @brief Construct a new Test object
         */
        Test() {};

        /**
         * @brief Destroy the Test object
         */
        virtual ~Test() {};

        /**
         * @brief Update the test (called once per frame)
         * @param deltaTime 
         */
        virtual void onUpdate(float deltaTime) {}

        /**
         * @brief Render the test
         */
        virtual void onRender() {}
        
        /**
         * @brief Render the ImGui window
         */
        virtual void onImGuiRender() {}
    };

    class TestMenu : public Test {
    private:
        Test*& m_current_test; // Reference to the current test
        std::vector<std::pair<std::string, std::function<Test*()>>> m_tests; // List of tests

    public:
        /**
         * @brief Construct a new Test Menu object
         * @param currentTestPointer
         */
        TestMenu(Test*& currentTestPointer);

        /**
         * @brief Destroy the Test Menu object
         */
        void onImGuiRender() override;

        /**
         * @brief Register a new test
         * @tparam T
         * @param name
         */
        template<typename T>
        void registerTest(const std::string& name) {
            std::cout << "Registering test " << name << std::endl;
            m_tests.push_back(std::make_pair(name, []() { return new T(); }));
        }
    };
}




#endif // TEST_H
