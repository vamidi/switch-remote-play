#include "MouseOptions.h"

namespace mouse
{
    std::unordered_map<Parameters, std::string> const MouseParamsToStr(MouseConfig const config)
    {
        auto values = std::unordered_map<Parameters, std::string>{};

        values[Parameters::LeftMouseButton] = controller::SwitchButtonToString(config.leftClickButton);
        values[Parameters::RightMouseButton] = controller::SwitchButtonToString(config.rightClickButton);
        values[Parameters::MiddleMouseButton] = controller::SwitchButtonToString(config.middleClickButton);
        values[Parameters::MouseWheelAnalog] = controller::AnalogStickToString(config.mouseWheelAnalog);
        values[Parameters::MouseSensitivity] = std::to_string(config.mouseSensitivity);
        values[Parameters::MouseOnConnect] = config.mouseOnConnect ? "yes" : "no";

        return values;
    }

    MouseConfig const MouseParamsFromStr(std::unordered_map<Parameters, std::string> const & map)
    {
        auto config = MouseConfig{};

        auto parseMouseButton = [&](auto mouseBtn, auto & member, auto defVal)
        {
            auto entry = map.find(mouseBtn);
            if(entry != map.end())
            {
                auto btnStr = entry->second;
                auto btn = controller::ParseSwitchButtonString(btnStr);
                member = btn;
            }
            else
                member = defVal;
        };

        parseMouseButton(Parameters::LeftMouseButton, config.leftClickButton, DefaultLeftClickButton);
        parseMouseButton(Parameters::RightMouseButton, config.rightClickButton, DefaultRightClickButton);
        parseMouseButton(Parameters::MiddleMouseButton, config.middleClickButton, DefaultMiddleClickButton);

        {
            auto entry = map.find(Parameters::MouseOnConnect);
            if(entry != map.end())
            {
                auto str = entry->second;
                config.mouseOnConnect = str == "yes";
            }
            else
                config.mouseOnConnect = false;
        }

        
        {
            auto entry = map.find(Parameters::MouseSensitivity);
            if(entry != map.end())
            {
                auto str = entry->second;
                auto val = std::atoi(str.c_str());
                auto sens = std::min(val, (int)MaxMouseSensitivity);
                sens = std::max(sens, (int)MinMouseSensitivity);
                config.mouseSensitivity = (int16_t)sens;
            }
            else
                config.mouseSensitivity = DefaultMouseSensitivity;
        }

        {
            auto entry = map.find(Parameters::MouseWheelAnalog);
            if(entry != map.end())
            {
                auto str = entry->second;
                auto analog = controller::ParseAnalogStickString(str);
                config.mouseWheelAnalog = analog;
            }
            else
                config.mouseWheelAnalog = DefaultMouseWheelAnalog;
        }

        return config;
    }

}