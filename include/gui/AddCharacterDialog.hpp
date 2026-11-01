#pragma once

#include "entities/Roster.hpp"

#include <array>
#include <string>

namespace dnd::gui
{

    /// Модальное окно «Добавить персонажа»: имя, класс, уровень, сторона и характеристики.
    class AddCharacterDialog
    {
    public:
        /// Открывает окно (вызывается по нажатию кнопки).
        void open() { m_openRequested = true; }

        /// Рисует окно, если оно открыто. Возвращает индекс добавленного персонажа
        /// в roster или -1, если ничего не добавлено в этом кадре.
        int draw(entities::Roster &roster);

    private:
        void reset();

        static constexpr const char *kPopupId = "Add character";

        bool m_openRequested = false;
        std::string m_name = "New Hero";
        int m_classIndex = 4; // Fighter
        int m_level = 1;
        int m_sideIndex = 0; // Player
        std::array<int, entities::kAbilityCount> m_scores{10, 10, 10, 10, 10, 10};
    };

} // namespace dnd::gui
