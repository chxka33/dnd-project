#pragma once

#include <SFML/System/Time.hpp>
#include <SFML/Window/Event.hpp>

#include <cassert>

namespace sf { class RenderWindow; }

namespace dnd::core {

struct AppContext;

/// Базовый класс экрана приложения (меню, редактор, симулятор, ...).
///
/// Порядок вызовов за кадр:
///   handleEvent() -> update() (фикс. шаг) -> drawUi() (ImGui) -> render() (SFML)
class State {
public:
    virtual ~State() = default;

    virtual void onEnter() {}   ///< состояние стало активным
    virtual void onExit() {}    ///< состояние снимается со стека

    virtual void handleEvent(const sf::Event& event) = 0;
    virtual void update(sf::Time dt) = 0;
    virtual void drawUi() {}                           ///< вызовы ImGui::*
    virtual void render(sf::RenderWindow& window) = 0; ///< 2D-отрисовка SFML

protected:
    /// Доступ к окну, ресурсам и менеджеру состояний.
    AppContext& context() {
        assert(m_context && "State is not attached to a StateManager");
        return *m_context;
    }

private:
    friend class StateManager;
    AppContext* m_context = nullptr;
};

} // namespace dnd::core
