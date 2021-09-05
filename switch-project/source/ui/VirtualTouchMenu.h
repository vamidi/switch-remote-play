#ifndef __VIRTUALTOUCHMENU_H__
#define __VIRTUALTOUCHMENU_H__

#include "Menu.h"
#include "../touch/TouchOptions.h"
#include "../utils/ArrayCirculator.h"
#include <unordered_map>

class VirtualTouchMenu : public Menu
{
    public:
        VirtualTouchMenu();

        void ProcessInput(PadState const & pad) override;
        void Render(SDL_Renderer * const renderer, FC_Font * const font) override;

        touch::VirtualTouchConfig const Settings() const;

    private:
        Text helpText;
        std::unordered_map<touch::VirtualTouchParameters, Text> textElements;
        ArrayCirculator<touch::VirtualTouchParameters, std::vector> selected;
        int16_t deadzoneRadius;
        int16_t maxFingerCount;

        void PromptValueInput(touch::VirtualTouchParameters param);

        int16_t KeyboardValue(int16_t minValue, int16_t maxValue);

        void UpdateUI(touch::VirtualTouchParameters param);

        void SetupText();
};

#endif