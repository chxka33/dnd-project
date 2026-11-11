#include "gui/editor/ScenarioCanvas.hpp"

#include "gui/editor/EditorWidgets.hpp"

#include <algorithm>
#include <cmath>
#include <string>
#include <unordered_map>

namespace dnd::gui::editor
{
    using namespace dnd::scenario;

    namespace
    {
        constexpr float kMinZoom = 0.25f;
        constexpr float kMaxZoom = 3.0f;
        constexpr float kGridStep = 40.f;
        constexpr float kPortRadius = 7.f;
        constexpr float kTitleHeight = 28.f;
        constexpr float kLinkHitDistance = 7.f;

        ImVec2 vadd(const ImVec2 &a, const ImVec2 &b) { return ImVec2(a.x + b.x, a.y + b.y); }
        ImVec2 vsub(const ImVec2 &a, const ImVec2 &b) { return ImVec2(a.x - b.x, a.y - b.y); }
        ImVec2 vmul(const ImVec2 &a, float s) { return ImVec2(a.x * s, a.y * s); }

        bool inside(const ImVec2 &p, const ImVec2 &min, const ImVec2 &max)
        {
            return p.x >= min.x && p.x <= max.x && p.y >= min.y && p.y <= max.y;
        }

        float distanceToSegment(const ImVec2 &p, const ImVec2 &a, const ImVec2 &b)
        {
            const ImVec2 ab = vsub(b, a);
            const float len2 = ab.x * ab.x + ab.y * ab.y;
            float t = 0.f;
            if (len2 > 0.f)
                t = std::clamp(((p.x - a.x) * ab.x + (p.y - a.y) * ab.y) / len2, 0.f, 1.f);
            const ImVec2 proj(a.x + ab.x * t, a.y + ab.y * t);
            const ImVec2 d = vsub(p, proj);
            return std::sqrt(d.x * d.x + d.y * d.y);
        }

        ImVec2 bezierPoint(const ImVec2 &p0, const ImVec2 &p1, const ImVec2 &p2, const ImVec2 &p3, float t)
        {
            const float u = 1.f - t;
            const float w0 = u * u * u;
            const float w1 = 3.f * u * u * t;
            const float w2 = 3.f * u * t * t;
            const float w3 = t * t * t;
            return ImVec2(w0 * p0.x + w1 * p1.x + w2 * p2.x + w3 * p3.x,
                          w0 * p0.y + w1 * p1.y + w2 * p2.y + w3 * p3.y);
        }

        ImU32 linkColor(Link::Kind kind)
        {
            switch (kind)
            {
            case Link::Kind::Story:
                return IM_COL32(170, 190, 220, 255);
            case Link::Kind::Choice:
                return IM_COL32(230, 180, 70, 255);
            case Link::Kind::Random:
                return IM_COL32(190, 130, 230, 255);
            case Link::Kind::RandomFallback:
                return IM_COL32(150, 150, 150, 255);
            }
            return IM_COL32(200, 200, 200, 255);
        }

        void drawArrowHead(ImDrawList *dl, const ImVec2 &tip, const ImVec2 &from, float size, ImU32 color)
        {
            ImVec2 dir = vsub(tip, from);
            const float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
            dir = len > 0.001f ? vmul(dir, 1.f / len) : ImVec2(1.f, 0.f);
            const ImVec2 perp(-dir.y, dir.x);
            const ImVec2 base = vsub(tip, vmul(dir, size));
            dl->AddTriangleFilled(tip, vadd(base, vmul(perp, size * 0.55f)),
                                  vsub(base, vmul(perp, size * 0.55f)), color);
        }
    } // namespace

    // ------------------------------------------------------------------ координаты

    ImVec2 ScenarioCanvas::toScreen(float wx, float wy) const
    {
        return ImVec2(m_origin.x + m_pan.x + wx * m_zoom, m_origin.y + m_pan.y + wy * m_zoom);
    }

    ImVec2 ScenarioCanvas::toWorld(const ImVec2 &screen) const
    {
        return ImVec2((screen.x - m_origin.x - m_pan.x) / m_zoom, (screen.y - m_origin.y - m_pan.y) / m_zoom);
    }

    void ScenarioCanvas::nodeRect(const Node &node, ImVec2 &min, ImVec2 &max) const
    {
        min = toScreen(node.position.x, node.position.y);
        max = ImVec2(min.x + kNodeWidth * m_zoom, min.y + kNodeHeight * m_zoom);
    }

    ImVec2 ScenarioCanvas::outputPort(const Node &node) const
    {
        ImVec2 min, max;
        nodeRect(node, min, max);
        return ImVec2(max.x, (min.y + max.y) * 0.5f);
    }

    ImVec2 ScenarioCanvas::inputPort(const Node &node) const
    {
        ImVec2 min, max;
        nodeRect(node, min, max);
        return ImVec2(min.x, (min.y + max.y) * 0.5f);
    }

    // ------------------------------------------------------------------ выделение и вид

    void ScenarioCanvas::select(const NodeId &id)
    {
        m_selected = id;
        m_selectedLink.reset();
    }

    void ScenarioCanvas::clearSelection()
    {
        m_selected.clear();
        m_selectedLink.reset();
        m_drag = Drag::None;
    }

    void ScenarioCanvas::resetView()
    {
        m_pan = ImVec2(60.f, 60.f);
        m_zoom = 1.f;
    }

    void ScenarioCanvas::fitToContent()
    {
        m_pendingFit = true;
    }

    void ScenarioCanvas::focusOn(const Node &node)
    {
        m_pendingFocus = node.id();
    }

    Node &ScenarioCanvas::addNodeAtViewCenter(ScenarioGraph &graph, NodeType type)
    {
        const ImVec2 center = toWorld(ImVec2(m_origin.x + m_size.x * 0.5f, m_origin.y + m_size.y * 0.5f));
        const float cascade = static_cast<float>(m_spawnCount++ % 6) * 28.f; // чтобы новые узлы не слипались
        Node &node = createNode(graph, type,
                                {center.x - kNodeWidth * 0.5f + cascade, center.y - kNodeHeight * 0.5f + cascade});
        select(node.id());
        return node;
    }

    bool ScenarioCanvas::takeDeleteRequest(NodeId &out)
    {
        if (!m_deleteRequest)
            return false;
        out = *m_deleteRequest;
        m_deleteRequest.reset();
        return true;
    }

    void ScenarioCanvas::zoomAt(const ImVec2 &screenPoint, float wheel)
    {
        const ImVec2 before = toWorld(screenPoint);
        m_zoom = std::clamp(m_zoom * std::pow(1.1f, wheel), kMinZoom, kMaxZoom);
        // Мировая точка под курсором остаётся на месте.
        m_pan = ImVec2(screenPoint.x - m_origin.x - before.x * m_zoom,
                       screenPoint.y - m_origin.y - before.y * m_zoom);
    }

    // ------------------------------------------------------------------ геометрия и поиск

    void ScenarioCanvas::buildGeometry(const ScenarioGraph &graph)
    {
        m_geometry.clear();
        for (Link &link : collectLinks(graph))
        {
            const Node *from = graph.find(link.from);
            const Node *to = graph.find(link.to);
            if (!from || !to)
                continue; // битая ссылка: покажет валидация
            const ImVec2 p0 = outputPort(*from);
            const ImVec2 p3 = inputPort(*to);
            const float dx = std::max(60.f * m_zoom, std::abs(p3.x - p0.x) * 0.5f);
            m_geometry.push_back({std::move(link), p0, ImVec2(p0.x + dx, p0.y), ImVec2(p3.x - dx, p3.y), p3});
        }
    }

    std::vector<const Node *> ScenarioCanvas::drawOrder(const ScenarioGraph &graph) const
    {
        std::vector<const Node *> order;
        order.reserve(graph.size());
        const Node *selected = nullptr;
        for (const NodeId &id : graph.order())
        {
            const Node *node = graph.find(id);
            if (!node)
                continue;
            if (id == m_selected)
                selected = node;
            else
                order.push_back(node);
        }
        if (selected)
            order.push_back(selected); // выбранный узел рисуется поверх остальных
        return order;
    }

    const Node *ScenarioCanvas::hitNode(const std::vector<const Node *> &order, const ImVec2 &p) const
    {
        for (auto it = order.rbegin(); it != order.rend(); ++it)
        {
            ImVec2 min, max;
            nodeRect(**it, min, max);
            if (inside(p, min, max))
                return *it;
        }
        return nullptr;
    }

    const Node *ScenarioCanvas::hitPort(const std::vector<const Node *> &order, const ImVec2 &p) const
    {
        const float radius = kPortRadius * std::max(m_zoom, 0.8f) + 3.f;
        for (auto it = order.rbegin(); it != order.rend(); ++it)
        {
            const ImVec2 port = outputPort(**it);
            const ImVec2 d = vsub(p, port);
            if (d.x * d.x + d.y * d.y <= radius * radius)
                return *it;
        }
        return nullptr;
    }

    const ScenarioCanvas::LinkGeometry *ScenarioCanvas::hitLink(const ImVec2 &p) const
    {
        constexpr int kSegments = 24;
        const LinkGeometry *best = nullptr;
        float bestDistance = kLinkHitDistance;
        for (const LinkGeometry &g : m_geometry)
        {
            ImVec2 prev = g.p0;
            for (int i = 1; i <= kSegments; ++i)
            {
                const ImVec2 cur = bezierPoint(g.p0, g.p1, g.p2, g.p3, static_cast<float>(i) / kSegments);
                const float d = distanceToSegment(p, prev, cur);
                if (d < bestDistance)
                {
                    bestDistance = d;
                    best = &g;
                }
                prev = cur;
            }
        }
        return best;
    }

    // ------------------------------------------------------------------ ввод

    void ScenarioCanvas::handleInput(ScenarioGraph &graph, bool hovered)
    {
        ImGuiIO &io = ImGui::GetIO();
        const ImVec2 mouse = io.MousePos;
        const std::vector<const Node *> order = drawOrder(graph);

        if (hovered && m_drag == Drag::None && io.MouseWheel != 0.f)
            zoomAt(mouse, io.MouseWheel);

        // --- начало действий ---
        if (hovered && m_drag == Drag::None)
        {
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Middle))
            {
                m_drag = Drag::Pan;
                m_dragButton = ImGuiMouseButton_Middle;
            }
            else if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            {
                m_dragButton = ImGuiMouseButton_Left;
                if (const Node *port = hitPort(order, mouse))
                {
                    select(port->id());
                    m_drag = Drag::NewLink;
                    m_dragNode = port->id();
                }
                else if (const Node *node = hitNode(order, mouse))
                {
                    select(node->id());
                    m_drag = Drag::MoveNode;
                    m_dragNode = node->id();
                }
                else if (const LinkGeometry *link = hitLink(mouse))
                {
                    m_selected.clear();
                    m_selectedLink = link->link;
                }
                else
                {
                    clearSelection();
                    m_drag = Drag::Pan;
                }
            }
            else if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
            {
                m_ctxNode.clear();
                m_ctxLink.reset();
                m_ctxWorld = toWorld(mouse);
                if (const Node *node = hitNode(order, mouse))
                {
                    m_ctxNode = node->id();
                    select(node->id());
                }
                else if (const LinkGeometry *link = hitLink(mouse))
                {
                    m_ctxLink = link->link;
                    m_selected.clear();
                    m_selectedLink = link->link;
                }
                ImGui::OpenPopup("canvas_ctx");
            }
        }

        // --- продолжение / завершение перетаскивания ---
        if (m_drag != Drag::None)
        {
            if (ImGui::IsMouseDown(m_dragButton))
            {
                switch (m_drag)
                {
                case Drag::Pan:
                    m_pan = vadd(m_pan, io.MouseDelta);
                    break;
                case Drag::MoveNode:
                    if (Node *node = graph.find(m_dragNode))
                    {
                        node->position.x += io.MouseDelta.x / m_zoom;
                        node->position.y += io.MouseDelta.y / m_zoom;
                    }
                    break;
                case Drag::NewLink:
                case Drag::None:
                    break;
                }
            }
            else
            {
                if (m_drag == Drag::NewLink)
                {
                    const Node *target = hitNode(order, mouse);
                    if (target && target->id() != m_dragNode)
                        connect(graph, m_dragNode, target->id());
                }
                m_drag = Drag::None;
            }
        }

        // --- клавиатура ---
        if (!io.WantTextInput && (hovered || ImGui::IsWindowFocused()) && m_drag == Drag::None &&
            (ImGui::IsKeyPressed(ImGuiKey_Delete, false) || ImGui::IsKeyPressed(ImGuiKey_Backspace, false)))
        {
            if (m_selectedLink)
            {
                disconnect(graph, *m_selectedLink);
                m_selectedLink.reset();
            }
            else if (!m_selected.empty())
            {
                m_deleteRequest = m_selected;
            }
        }
    }

    // ------------------------------------------------------------------ отрисовка

    void ScenarioCanvas::drawGrid(ImDrawList *dl) const
    {
        const float step = kGridStep * m_zoom;
        if (step < 10.f)
            return;

        const ImU32 minor = IM_COL32(48, 42, 38, 255);
        const ImU32 major = IM_COL32(70, 62, 54, 255);
        const ImVec2 end = vadd(m_origin, m_size);

        float ox = std::fmod(m_pan.x, step);
        if (ox < 0.f)
            ox += step;
        float oy = std::fmod(m_pan.y, step);
        if (oy < 0.f)
            oy += step;

        for (float x = m_origin.x + ox; x < end.x; x += step)
        {
            const int index = static_cast<int>(std::lround((x - m_origin.x - m_pan.x) / step));
            dl->AddLine(ImVec2(x, m_origin.y), ImVec2(x, end.y), index % 5 == 0 ? major : minor);
        }
        for (float y = m_origin.y + oy; y < end.y; y += step)
        {
            const int index = static_cast<int>(std::lround((y - m_origin.y - m_pan.y) / step));
            dl->AddLine(ImVec2(m_origin.x, y), ImVec2(end.x, y), index % 5 == 0 ? major : minor);
        }
    }

    void ScenarioCanvas::drawLinks(ImDrawList *dl) const
    {
        ImFont *font = ImGui::GetFont();
        const ImVec2 mouse = ImGui::GetIO().MousePos;
        const LinkGeometry *hovered = (m_drag == Drag::None) ? hitLink(mouse) : nullptr;

        for (const LinkGeometry &g : m_geometry)
        {
            const bool selected = m_selectedLink && m_selectedLink->sameAs(g.link);
            const bool hot = hovered == &g;
            const bool fallback = g.link.kind == Link::Kind::RandomFallback;

            ImU32 color = linkColor(g.link.kind);
            float thickness = std::clamp((fallback ? 1.5f : 2.2f) * m_zoom, 1.2f, 5.f);
            if (selected)
            {
                color = IM_COL32(255, 220, 90, 255);
                thickness += 1.5f;
            }
            else if (hot)
            {
                thickness += 1.f;
            }

            dl->AddBezierCubic(g.p0, g.p1, g.p2, g.p3, color, thickness);
            drawArrowHead(dl, g.p3, g.p2, std::clamp(11.f * m_zoom, 6.f, 20.f), color);

            // Подпись посередине стрелки: текст варианта / диапазон кубика.
            if (m_zoom >= 0.6f && !g.link.label.empty())
            {
                const std::string text = truncateUtf8(g.link.label, 22);
                const float fontSize = 12.f * m_zoom;
                const ImVec2 mid = bezierPoint(g.p0, g.p1, g.p2, g.p3, 0.5f);
                const ImVec2 ts = font->CalcTextSizeA(fontSize, FLT_MAX, 0.f, text.c_str());
                const ImVec2 pad(4.f * m_zoom, 2.f * m_zoom);
                const ImVec2 tl(mid.x - ts.x * 0.5f - pad.x, mid.y - ts.y * 0.5f - pad.y);
                const ImVec2 br(mid.x + ts.x * 0.5f + pad.x, mid.y + ts.y * 0.5f + pad.y);
                dl->AddRectFilled(tl, br, IM_COL32(28, 24, 22, 230), 3.f * m_zoom);
                dl->AddRect(tl, br, color, 3.f * m_zoom);
                dl->AddText(font, fontSize, ImVec2(tl.x + pad.x, tl.y + pad.y), IM_COL32(235, 225, 205, 255),
                            text.c_str());
            }
        }
    }

    void ScenarioCanvas::drawNodes(ImDrawList *dl, const ScenarioGraph &graph,
                                   const std::vector<ValidationIssue> &issues) const
    {
        ImFont *font = ImGui::GetFont();

        // Худшая проблема на узел: Error > Warning.
        std::unordered_map<NodeId, ValidationIssue::Severity> worst;
        for (const ValidationIssue &issue : issues)
        {
            if (issue.node.empty())
                continue;
            auto it = worst.find(issue.node);
            if (it == worst.end())
                worst[issue.node] = issue.severity;
            else if (issue.severity == ValidationIssue::Severity::Error)
                it->second = ValidationIssue::Severity::Error;
        }

        const bool showText = m_zoom >= 0.45f;
        const bool showDetails = m_zoom >= 0.6f;
        const float rounding = 6.f * m_zoom;
        const float titleH = kTitleHeight * m_zoom;
        const ImVec2 mouse = ImGui::GetIO().MousePos;

        for (const Node *node : drawOrder(graph))
        {
            ImVec2 min, max;
            nodeRect(*node, min, max);
            // Быстрая отсечка узлов за пределами холста.
            if (max.x < m_origin.x || min.x > m_origin.x + m_size.x ||
                max.y < m_origin.y || min.y > m_origin.y + m_size.y)
                continue;

            const bool selected = node->id() == m_selected;
            const bool isDragTarget = m_drag == Drag::NewLink && node->id() != m_dragNode && inside(mouse, min, max);

            // Тень и корпус.
            dl->AddRectFilled(ImVec2(min.x + 4.f * m_zoom, min.y + 5.f * m_zoom),
                              ImVec2(max.x + 4.f * m_zoom, max.y + 5.f * m_zoom),
                              IM_COL32(0, 0, 0, 90), rounding);
            dl->AddRectFilled(min, max, IM_COL32(44, 37, 32, 255), rounding);
            dl->AddRectFilled(min, ImVec2(max.x, min.y + titleH), nodeTypeColor(node->type()), rounding,
                              ImDrawFlags_RoundCornersTop);

            // Рамка: выбор > цель связи > проблемы > обычная.
            ImU32 border = IM_COL32(95, 80, 64, 255);
            float borderThickness = 1.5f;
            const auto issue = worst.find(node->id());
            if (issue != worst.end())
            {
                border = issue->second == ValidationIssue::Severity::Error ? IM_COL32(210, 60, 55, 255)
                                                                           : IM_COL32(215, 170, 50, 255);
                borderThickness = 2.f;
            }
            if (isDragTarget)
            {
                border = IM_COL32(90, 220, 110, 255);
                borderThickness = 3.f;
            }
            if (selected)
            {
                border = IM_COL32(255, 220, 90, 255);
                borderThickness = 3.f;
            }
            dl->AddRect(min, max, border, rounding, 0, borderThickness * std::max(m_zoom, 0.7f));

            if (showText)
            {
                const float pad = 8.f * m_zoom;

                // Заголовок.
                const std::string title = node->title.empty() ? node->id() : node->title;
                const ImVec4 titleClip(min.x + pad, min.y, max.x - pad, min.y + titleH);
                dl->AddText(font, 15.f * m_zoom, ImVec2(min.x + pad, min.y + 6.f * m_zoom),
                            IM_COL32(250, 245, 232, 255), title.c_str(), nullptr, 0.f, &titleClip);

                if (showDetails)
                {
                    // Саммари сцены (с переносом, обрезается по корпусу).
                    const ImVec4 bodyClip(min.x + pad, min.y + titleH + 4.f * m_zoom, max.x - pad,
                                          max.y - 22.f * m_zoom);
                    if (!node->summary.empty())
                        dl->AddText(font, 13.f * m_zoom, ImVec2(bodyClip.x, bodyClip.y),
                                    IM_COL32(215, 205, 185, 255), node->summary.c_str(), nullptr,
                                    bodyClip.z - bodyClip.x, &bodyClip);

                    // Нижняя строка: тип и значки.
                    std::string footer;
                    switch (node->type())
                    {
                    case NodeType::Story:
                        footer = "Scene";
                        break;
                    case NodeType::Choice:
                        footer = "Fork x" + std::to_string(static_cast<const ChoiceNode *>(node)->choices().size());
                        break;
                    case NodeType::Random:
                        footer = "Roll d" + std::to_string(static_cast<const RandomNode *>(node)->diceSides());
                        break;
                    }
                    if (node->check)
                        footer += std::string("  |  ") +
                                  (node->check->type == CheckType::SavingThrow ? "Save " : "Check ") +
                                  "DC " + std::to_string(node->check->dc);
                    if (node->loot)
                        footer += "  |  Loot";
                    if (!node->enemyIds.empty())
                        footer += "  |  Foes " + std::to_string(node->enemyIds.size());
                    const ImVec4 footClip(min.x + pad, max.y - 20.f * m_zoom, max.x - pad, max.y);
                    dl->AddText(font, 12.f * m_zoom, ImVec2(footClip.x, footClip.y + 3.f * m_zoom),
                                IM_COL32(150, 140, 125, 255), footer.c_str(), nullptr, 0.f, &footClip);
                }
            }

            // Метка старта.
            if (node->id() == graph.startId())
            {
                const float fs = 12.f * std::max(m_zoom, 0.7f);
                const ImVec2 ts = font->CalcTextSizeA(fs, FLT_MAX, 0.f, "START");
                const ImVec2 tl(min.x, min.y - ts.y - 6.f * m_zoom);
                dl->AddRectFilled(tl, ImVec2(tl.x + ts.x + 10.f * m_zoom, min.y - 1.f), IM_COL32(60, 150, 70, 255),
                                  3.f * m_zoom);
                dl->AddText(font, fs, ImVec2(tl.x + 5.f * m_zoom, tl.y + 2.f * m_zoom),
                            IM_COL32(255, 255, 255, 255), "START");
            }

            // Порты: вход слева, выход справа (за него тянут новую связь).
            const float portR = kPortRadius * std::max(m_zoom, 0.6f);
            const ImVec2 in = inputPort(*node);
            const ImVec2 out = outputPort(*node);
            dl->AddCircleFilled(in, portR * 0.75f, IM_COL32(60, 52, 46, 255));
            dl->AddCircle(in, portR * 0.75f, IM_COL32(190, 175, 150, 255), 0, 1.5f);
            const ImVec2 d = vsub(mouse, out);
            const bool portHot = (d.x * d.x + d.y * d.y) <= (portR + 3.f) * (portR + 3.f);
            dl->AddCircleFilled(out, portR, portHot ? IM_COL32(255, 220, 90, 255) : nodeTypeColor(node->type()));
            dl->AddCircle(out, portR, IM_COL32(240, 230, 210, 255), 0, 1.5f);
        }
    }

    void ScenarioCanvas::drawLinkPreview(ImDrawList *dl, const ScenarioGraph &graph) const
    {
        if (m_drag != Drag::NewLink)
            return;
        const Node *from = graph.find(m_dragNode);
        if (!from)
            return;

        const ImVec2 mouse = ImGui::GetIO().MousePos;
        const ImVec2 p0 = outputPort(*from);
        const float dx = std::max(60.f * m_zoom, std::abs(mouse.x - p0.x) * 0.5f);
        dl->AddBezierCubic(p0, ImVec2(p0.x + dx, p0.y), ImVec2(mouse.x - dx, mouse.y), mouse,
                           IM_COL32(255, 220, 90, 220), std::clamp(2.5f * m_zoom, 1.5f, 5.f));
        drawArrowHead(dl, mouse, ImVec2(mouse.x - dx, mouse.y), std::clamp(11.f * m_zoom, 6.f, 20.f),
                      IM_COL32(255, 220, 90, 220));
    }

    void ScenarioCanvas::drawContextMenu(ScenarioGraph &graph)
    {
        if (!ImGui::BeginPopup("canvas_ctx"))
            return;

        if (!m_ctxNode.empty() && graph.contains(m_ctxNode))
        {
            Node &node = graph.at(m_ctxNode);
            ImGui::TextDisabled("%s", node.title.empty() ? node.id().c_str() : node.title.c_str());
            ImGui::Separator();
            if (ImGui::MenuItem("Set as start scene", nullptr, false, graph.startId() != node.id()))
                graph.setStart(node.id());
            if (ImGui::MenuItem("Delete scene..."))
                m_deleteRequest = node.id();
        }
        else if (m_ctxLink)
        {
            ImGui::TextDisabled("Link");
            ImGui::Separator();
            if (ImGui::MenuItem("Delete link"))
            {
                disconnect(graph, *m_ctxLink);
                m_selectedLink.reset();
            }
        }
        else
        {
            ImGui::TextDisabled("Add scene here");
            ImGui::Separator();
            const EditorPosition pos{m_ctxWorld.x - kNodeWidth * 0.5f, m_ctxWorld.y - kNodeHeight * 0.5f};
            if (ImGui::MenuItem("Story scene"))
                select(createNode(graph, NodeType::Story, pos).id());
            if (ImGui::MenuItem("Fork (choice)"))
                select(createNode(graph, NodeType::Choice, pos).id());
            if (ImGui::MenuItem("Random table"))
                select(createNode(graph, NodeType::Random, pos).id());
        }
        ImGui::EndPopup();
    }

    // ------------------------------------------------------------------ главный метод

    void ScenarioCanvas::draw(ScenarioGraph &graph, const std::vector<ValidationIssue> &issues, const ImVec2 &size)
    {
        ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(24, 20, 18, 255));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
        ImGui::BeginChild("scenario_canvas", size, true,
                          ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
                              ImGuiWindowFlags_NoMove);
        ImGui::PopStyleVar();

        m_origin = ImGui::GetCursorScreenPos();
        m_size = ImGui::GetContentRegionAvail();

        if (m_size.x >= 1.f && m_size.y >= 1.f)
        {
            ImGui::InvisibleButton("surface", m_size,
                                   ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight |
                                       ImGuiButtonFlags_MouseButtonMiddle);
            const bool hovered = ImGui::IsItemHovered();

            // Выделение могло устареть (узел удалён, сценарий загружен заново).
            if (!m_selected.empty() && !graph.contains(m_selected))
                m_selected.clear();
            if (m_drag != Drag::None && !graph.contains(m_dragNode))
                m_drag = Drag::None;

            // Отложенные команды вида (нужны размеры холста, известные только здесь).
            if (m_pendingFit)
            {
                m_pendingFit = false;
                if (graph.empty())
                {
                    resetView();
                }
                else
                {
                    float minX = 1e9f, minY = 1e9f, maxX = -1e9f, maxY = -1e9f;
                    for (const NodeId &id : graph.order())
                    {
                        const Node &n = graph.at(id);
                        minX = std::min(minX, n.position.x);
                        minY = std::min(minY, n.position.y);
                        maxX = std::max(maxX, n.position.x + kNodeWidth);
                        maxY = std::max(maxY, n.position.y + kNodeHeight);
                    }
                    const float w = std::max(maxX - minX, 1.f) + 120.f;
                    const float h = std::max(maxY - minY, 1.f) + 120.f;
                    m_zoom = std::clamp(std::min(m_size.x / w, m_size.y / h), kMinZoom, 1.25f);
                    m_pan = ImVec2(m_size.x * 0.5f - (minX + maxX) * 0.5f * m_zoom,
                                   m_size.y * 0.5f - (minY + maxY) * 0.5f * m_zoom);
                }
            }
            if (m_pendingFocus)
            {
                if (const Node *n = graph.find(*m_pendingFocus))
                    m_pan = ImVec2(m_size.x * 0.5f - (n->position.x + kNodeWidth * 0.5f) * m_zoom,
                                   m_size.y * 0.5f - (n->position.y + kNodeHeight * 0.5f) * m_zoom);
                m_pendingFocus.reset();
            }

            buildGeometry(graph);
            handleInput(graph, hovered);
            buildGeometry(graph); // после перетаскивания узлов связи должны следовать за ними в этом же кадре

            if (m_selectedLink)
            {
                const bool alive = std::any_of(m_geometry.begin(), m_geometry.end(), [&](const LinkGeometry &g)
                                               { return g.link.sameAs(*m_selectedLink); });
                if (!alive)
                    m_selectedLink.reset();
            }

            ImDrawList *dl = ImGui::GetWindowDrawList();
            dl->PushClipRect(m_origin, vadd(m_origin, m_size), true);
            drawGrid(dl);
            drawLinks(dl);
            drawNodes(dl, graph, issues);
            drawLinkPreview(dl, graph);
            if (graph.empty())
            {
                const char *hint = "Right-click to add the first scene";
                const ImVec2 ts = ImGui::CalcTextSize(hint);
                dl->AddText(ImVec2(m_origin.x + (m_size.x - ts.x) * 0.5f, m_origin.y + (m_size.y - ts.y) * 0.5f),
                            IM_COL32(150, 140, 125, 255), hint);
            }
            dl->PopClipRect();

            drawContextMenu(graph);
        }

        ImGui::EndChild();
        ImGui::PopStyleColor();
    }

} // namespace dnd::gui::editor
