#pragma once

#include "core/State.hpp"

namespace dnd::core {

/// Главное меню: переход в редактор / симулятор / выход.
class MainMenuState final : public State {
public:
    void handleEvent(const sf::Event&) override {}
    void update(sf::Time) override {}
    void drawUi() override;
    void render(sf::RenderWindow&) override {}
};

/// Временная заглушка для экранов, которые ещё не написаны
/// (редактор — Назаров, симулятор — Ярошук). Заменяется настоящими состояниями.
class PlaceholderState final : public State {
public:
    explicit PlaceholderState(const char* title) : m_title(title) {}

    void handleEvent(const sf::Event&) override {}
    void update(sf::Time) override {}
    void drawUi() override;
    void render(sf::RenderWindow&) override {}

private:
    const char* m_title;
};

} // namespace dnd::core
