#include "core/StateManager.hpp"
#include "core/Application.hpp" // AppContext (не нужен, если контекст = nullptr)

#include <stdexcept>

namespace dnd::core {

void StateManager::registerState(StateId id, Factory factory) {
    m_factories[id] = std::move(factory);
}

void StateManager::requestSwitch(StateId id) { m_pending.push_back({Op::Switch, id}); }
void StateManager::requestPush(StateId id)   { m_pending.push_back({Op::Push, id}); }
void StateManager::requestPop()              { m_pending.push_back({Op::Pop, StateId::MainMenu}); }

std::unique_ptr<State> StateManager::create(StateId id) const {
    const auto it = m_factories.find(id);
    if (it == m_factories.end()) {
        throw std::logic_error("StateManager: state is not registered");
    }
    return it->second();
}

void StateManager::pushNow(StateId id) {
    auto state = create(id);
    state->m_context = m_context;
    m_stack.push_back(std::move(state));
    m_stack.back()->onEnter();
}

void StateManager::popNow() {
    if (m_stack.empty()) return;
    m_stack.back()->onExit();
    m_stack.pop_back();
}

void StateManager::applyPending() {
    // Забираем очередь, чтобы onEnter()/onExit() могли безопасно добавлять новые запросы.
    auto pending = std::move(m_pending);
    m_pending.clear();

    for (const Pending& p : pending) {
        switch (p.op) {
            case Op::Push:   pushNow(p.id); break;
            case Op::Pop:    popNow();      break;
            case Op::Switch:
                while (!m_stack.empty()) popNow();
                pushNow(p.id);
                break;
        }
    }
}

void StateManager::handleEvent(const sf::Event& e) { if (auto* s = current()) s->handleEvent(e); }
void StateManager::update(sf::Time dt)             { if (auto* s = current()) s->update(dt); }
void StateManager::drawUi()                        { if (auto* s = current()) s->drawUi(); }
void StateManager::render(sf::RenderWindow& w)     { if (auto* s = current()) s->render(w); }

} // namespace dnd::core
