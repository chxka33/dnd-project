#pragma once

#include "core/State.hpp"

#include <cstddef>
#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

namespace dnd::core
{

    struct AppContext;

    /// Идентификаторы всех экранов приложения.
    enum class StateId
    {
        MainMenu,
        Editor,    // редактор сценариев (Назаров)
        Simulator, // симулятор игры (Ярошук)
        Options
    };

    /// Стек состояний. Смена состояний откладывается до applyPending(),
    /// поэтому состояние безопасно может запросить переход изнутри своих методов.
    class StateManager
    {
    public:
        using Factory = std::function<std::unique_ptr<State>()>;

        /// Контекст может быть nullptr (например, в юнит-тестах).
        void setContext(AppContext *context) { m_context = context; }

        void registerState(StateId id, Factory factory);

        void requestSwitch(StateId id); ///< очистить стек и сделать id единственным состоянием
        void requestPush(StateId id);   ///< положить поверх текущего
        void requestPop();              ///< снять верхнее состояние
        void applyPending();            ///< применить накопленные запросы

        bool empty() const { return m_stack.empty(); }
        std::size_t size() const { return m_stack.size(); }
        State *current() { return m_stack.empty() ? nullptr : m_stack.back().get(); }

        // Делегирование верхнему состоянию.
        void handleEvent(const sf::Event &event);
        void update(sf::Time dt);
        void drawUi();
        void render(sf::RenderWindow &window);

    private:
        enum class Op
        {
            Switch,
            Push,
            Pop
        };
        struct Pending
        {
            Op op;
            StateId id;
        };

        std::unique_ptr<State> create(StateId id) const;
        void pushNow(StateId id);
        void popNow();

        AppContext *m_context = nullptr;
        std::unordered_map<StateId, Factory> m_factories;
        std::vector<std::unique_ptr<State>> m_stack;
        std::vector<Pending> m_pending;
    };

} // namespace dnd::core
