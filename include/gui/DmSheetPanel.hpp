#pragma once

#include "entities/Roster.hpp"
#include "scenario/Node.hpp"

#include <string>

namespace dnd::gui
{

    /// Панель Мастера (DM Sheet): сводка по группе и противникам в таблицах — HP, КД,
    /// пассивная внимательность, состояния, экипировка, ячейки заклинаний — и быстрые кнопки
    /// «-HP» / «+HP», управления состояниями и приёма лута. Рисуется внутри уже открытого окна ImGui.
    class DmSheetPanel
    {
    public:
        /// Возвращает индекс персонажа, чей лист хотят открыть (кнопка "Sheet"), или -1.
        int draw(entities::Roster &roster);

        /// Награда текущего узла сценария. Пока указатель не nullptr, у каждого героя появляется
        /// кнопка "Loot": золото уходит в кошелёк, предметы — в инвентарь. Указатель не копируется
        /// и не удаляется панелью — симулятор задаёт его каждый кадр (nullptr, если в узле нет награды).
        void setLoot(const scenario::Loot *loot) { m_loot = loot; }

        /// Индекс героя в Roster, которому только что выдали лут, или -1. Читает и сбрасывает
        /// значение — удобно для записи в журнал сессии.
        int takeLootRecipient();

    private:
        void drawSide(entities::Roster &roster, entities::Side side, int &openSheet);
        void drawLootBanner();

        int m_amount = 5;
        int m_removeIndex = -1;           ///< кого удаляем (после подтверждения)
        bool m_openRemovePopup = false;   ///< открыть окно подтверждения в этом кадре
        std::string m_removeName;

        const scenario::Loot *m_loot = nullptr;
        int m_lootRecipient = -1;
        std::string m_status;             ///< последнее сообщение («Thorin received 50 gp...»)
    };

} // namespace dnd::gui
