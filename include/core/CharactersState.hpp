#pragma once

#include "core/State.hpp"
#include "entities/Roster.hpp"
#include "gui/AddCharacterDialog.hpp"
#include "gui/CharacterSheetPanel.hpp"

#include <string>

namespace dnd::core
{

    /// Экран «Characters» — подготовка к игре: слева список игроков и противников,
    /// справа настройка выбранного персонажа (имя, уровень, характеристики, снаряжение).
    /// Здесь нет урона, лечения и отдыха — они относятся к самой игре (симулятор).
    /// Данные берутся из AppContext::roster.
    class CharactersState final : public State
    {
    public:
        void onEnter() override;
        void onExit() override;

        void handleEvent(const sf::Event &event) override;
        void update(sf::Time) override {}
        void drawUi() override;
        void render(sf::RenderWindow &) override {}

    private:
        void applyScale() const;
        void drawRosterList(entities::Roster &roster);
        void drawFileDialogs(entities::Roster &roster);

        /// Папка с сохранёнными персонажами (.dndch), как у сценариев.
        static constexpr const char *kSaveDir = "characters";

        char m_fileName[128] = "party";
        std::string m_status;
        bool m_openSave = false;
        bool m_openLoad = false;

        gui::CharacterSheetPanel m_sheet{gui::CharacterSheetPanel::Mode::Setup};
        gui::AddCharacterDialog m_addDialog;

        float m_scale = 2.0f;      ///< масштаб шрифта этого экрана
        float m_previousScale = 1.0f;
    };

} // namespace dnd::core
