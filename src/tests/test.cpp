#include "test.h"
#include "imgui.h"


namespace test{

    TestMenu::TestMenu(Test *&currentTestPointer)
        : m_current_test(currentTestPointer){
    }
    void TestMenu::onImGuiRender(){
        for (auto& test : m_tests){
            if (ImGui::Button(test.first.c_str())){
                m_current_test = test.second();
            }
        }
    }
}