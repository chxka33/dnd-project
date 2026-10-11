#pragma once

#include <cstdint>
#include <random>

namespace dnd::utils {

/// Обёртка над std::mt19937. Передаётся по ссылке туда, где нужна случайность
/// (вероятностные узлы, броски кубиков) — так её легко подменить сидом в тестах.
class Random {
public:
    Random() : m_engine(std::random_device{}()) {}
    explicit Random(std::uint32_t seed) : m_engine(seed) {}

    /// Целое число в [lo, hi] включительно.
    int range(int lo, int hi) {
        return std::uniform_int_distribution<int>(lo, hi)(m_engine);
    }

    /// Срабатывание с вероятностью percent% (P ∈ [1, 100]); <=0 — никогда, >=100 — всегда.
    bool chance(int percent) {
        if (percent <= 0)   return false;
        if (percent >= 100) return true;
        return range(1, 100) <= percent;
    }

    std::mt19937& engine() { return m_engine; }

private:
    std::mt19937 m_engine;
};

} // namespace dnd::utils
