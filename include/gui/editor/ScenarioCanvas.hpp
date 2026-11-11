#pragma once

#include "gui/editor/EditorOps.hpp"
#include "scenario/ScenarioGraph.hpp"

#include <imgui.h>

#include <optional>
#include <vector>

namespace dnd::gui::editor
{

    /// Интерактивная карта нелинейного сюжета (ImGui + ImDrawList).
    ///
    /// Управление:
    ///   * колесо мыши — масштаб (к курсору); ЛКМ по пустому месту или СКМ — панорама;
    ///   * ЛКМ по узлу — выбор и перетаскивание;
    ///   * тянуть за порт справа на узле и отпустить над другим узлом — создать связь;
    ///   * ЛКМ по стрелке — выбрать связь; Delete/Backspace — удалить связь (или запросить удаление узла);
    ///   * ПКМ — контекстное меню (добавить сцену, сделать стартовой, удалить).
    ///
    /// Холст меняет ScenarioGraph напрямую (позиции, связи, новые узлы); удаление узлов только
    /// запрашивает (takeDeleteRequest) — подтверждение показывает владелец холста.
    class ScenarioCanvas
    {
    public:
        ScenarioCanvas() = default;

        /// Рисует холст в текущем окне ImGui (дочернее окно заданного размера).
        void draw(scenario::ScenarioGraph &graph,
                  const std::vector<scenario::ValidationIssue> &issues,
                  const ImVec2 &size);

        // --- выделение ---
        const scenario::NodeId &selected() const { return m_selected; }
        void select(const scenario::NodeId &id);
        void clearSelection();

        // --- вид ---
        void resetView();
        void fitToContent();                         ///< выполнится при ближайшей отрисовке
        void focusOn(const scenario::Node &node);    ///< центрирует вид на узле
        float zoom() const { return m_zoom; }

        /// Создаёт узел в центре видимой области и выделяет его.
        scenario::Node &addNodeAtViewCenter(scenario::ScenarioGraph &graph, scenario::NodeType type);

        /// Запрос на удаление узла (Delete / контекстное меню). true один раз на запрос.
        bool takeDeleteRequest(scenario::NodeId &out);

        // --- текущее перетаскивание ---
        bool isInteracting() const { return m_drag != Drag::None; }
        void cancelInteraction() { m_drag = Drag::None; }

    private:
        enum class Drag
        {
            None,
            Pan,
            MoveNode,
            NewLink
        };

        struct LinkGeometry
        {
            Link link;
            ImVec2 p0, p1, p2, p3; ///< контрольные точки кубической кривой Безье (экранные)
        };

        // координаты
        ImVec2 toScreen(float wx, float wy) const;
        ImVec2 toWorld(const ImVec2 &screen) const;
        void nodeRect(const scenario::Node &node, ImVec2 &min, ImVec2 &max) const;
        ImVec2 outputPort(const scenario::Node &node) const;
        ImVec2 inputPort(const scenario::Node &node) const;

        // ввод
        void buildGeometry(const scenario::ScenarioGraph &graph);
        std::vector<const scenario::Node *> drawOrder(const scenario::ScenarioGraph &graph) const;
        const scenario::Node *hitNode(const std::vector<const scenario::Node *> &order, const ImVec2 &p) const;
        const scenario::Node *hitPort(const std::vector<const scenario::Node *> &order, const ImVec2 &p) const;
        const LinkGeometry *hitLink(const ImVec2 &p) const;
        void zoomAt(const ImVec2 &screenPoint, float wheel);
        void handleInput(scenario::ScenarioGraph &graph, bool hovered);

        // отрисовка
        void drawGrid(ImDrawList *dl) const;
        void drawLinks(ImDrawList *dl) const;
        void drawNodes(ImDrawList *dl, const scenario::ScenarioGraph &graph,
                       const std::vector<scenario::ValidationIssue> &issues) const;
        void drawLinkPreview(ImDrawList *dl, const scenario::ScenarioGraph &graph) const;
        void drawContextMenu(scenario::ScenarioGraph &graph);

        // вид
        ImVec2 m_pan{60.f, 60.f}; ///< смещение мирового начала координат от левого верхнего угла холста (px)
        float m_zoom = 1.f;
        ImVec2 m_origin{0.f, 0.f}; ///< левый верхний угол холста на экране
        ImVec2 m_size{0.f, 0.f};
        bool m_pendingFit = true;
        std::optional<scenario::NodeId> m_pendingFocus;

        // выделение и перетаскивание
        scenario::NodeId m_selected;
        std::optional<Link> m_selectedLink;
        Drag m_drag = Drag::None;
        ImGuiMouseButton m_dragButton = ImGuiMouseButton_Left;
        scenario::NodeId m_dragNode;

        // контекстное меню
        scenario::NodeId m_ctxNode;
        std::optional<Link> m_ctxLink;
        ImVec2 m_ctxWorld{0.f, 0.f};

        std::optional<scenario::NodeId> m_deleteRequest;
        std::vector<LinkGeometry> m_geometry;
        int m_spawnCount = 0;
    };

} // namespace dnd::gui::editor
