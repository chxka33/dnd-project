#pragma once

#include "entities/Roster.hpp"

#include <string>

namespace dnd::gui
{

    /// Панель Мастера (DM Sheet): сводка по всей группе и противникам в таблицах,
    /// быстрые кнопки урона/лечения. Рисуется внутри уже открытого окна ImGui.
    class DmSheetPanel
    {
    public:
        /// Возвращает индекс персонажа, чей лист хотят открыть (кнопка "Sheet"), или -1.
        int draw(entities::Roster &roster);

    private:
        void drawSide(entities::Roster &roster, entities::Side side, int &openSheet);

        int m_amount = 5;
        int m_removeIndex = -1;           ///< кого удаляем (после подтверждения)
        bool m_openRemovePopup = false;   ///< открыть окно подтверждения в этом кадре
        std::string m_removeName;
    };

} // namespace dnd::gui
