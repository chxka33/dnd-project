#include "core/EditorState.hpp"

#include "core/Application.hpp"
#include "gui/editor/EditorOps.hpp"
#include "gui/editor/EditorWidgets.hpp"
#include "scenario/ScenarioSerializer.hpp"

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Keyboard.hpp>

#include <imgui.h>

#include <algorithm>
#include <filesystem>
#include <system_error>
#include <utility>

namespace dnd::core
{
    using namespace dnd::scenario;
    namespace ed = dnd::gui::editor;

    // ------------------------------------------------------------------ жизненный цикл

    void EditorState::onEnter()
    {
        m_previousScale = ImGui::GetIO().FontGlobalScale;
        applyScale();
        resetScenario();
    }

    void EditorState::onExit()
    {
        ImGui::GetIO().FontGlobalScale = m_previousScale;
    }

    void EditorState::applyScale() const
    {
        ImGui::GetIO().FontGlobalScale = m_scale;
    }

    void EditorState::handleEvent(const sf::Event &event)
    {
        if (event.type != sf::Event::KeyPressed || event.key.code != sf::Keyboard::Escape)
            return;
        // Открыт список или диалог — Esc закрывает его, а не весь экран.
        if (ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel))
            return;
        if (m_canvas.isInteracting())
        {
            m_canvas.cancelInteraction();
            return;
        }
        requestAction(Pending::Exit);
    }

    // ------------------------------------------------------------------ сценарий: новый / сохранить / загрузить

    std::string EditorState::snapshot() const
    {
        return ScenarioSerializer::toJson(m_graph).dump();
    }

    bool EditorState::isDirty() const
    {
        return snapshot() != m_savedSnapshot;
    }

    void EditorState::setStatus(std::string text, bool error)
    {
        m_status = std::move(text);
        m_statusIsError = error;
    }

    void EditorState::resetScenario()
    {
        m_graph = ScenarioGraph{};
        m_graph.title = "Untitled scenario";
        m_canvas.clearSelection();
        m_canvas.fitToContent();
        m_bestiary.reset();
        m_documentPath.clear();
        m_savedSnapshot = snapshot();
    }

    namespace
    {
        bool fileExists(const std::string &path)
        {
            std::error_code ec;
            return std::filesystem::exists(path, ec);
        }
    } // namespace

    bool EditorState::saveTo(const std::string &path)
    {
        std::string error;
        if (!ScenarioSerializer::saveToFile(m_graph, path, &error))
        {
            setStatus("Save failed: " + error, true);
            return false;
        }
        m_documentPath = path;
        m_path = path;
        m_savedSnapshot = snapshot();
        setStatus("Saved to " + path, false);
        return true;
    }

    /// Кнопка «Save»: если в поле указан другой, уже существующий файл — сначала спрашиваем.
    void EditorState::requestSave()
    {
        if (m_path != m_documentPath && fileExists(m_path))
            m_openOverwritePopup = true;
        else
            saveTo(m_path);
    }

    /// «Save» в окне несохранённых изменений доступен, только если он не затрёт чужой файл.
    bool EditorState::canQuickSave() const
    {
        return !m_documentPath.empty() || !fileExists(m_path);
    }

    void EditorState::loadScenario()
    {
        std::string error;
        std::unique_ptr<ScenarioGraph> loaded = ScenarioSerializer::loadFromFile(m_path, &error);
        if (!loaded)
        {
            setStatus("Load failed: " + error, true);
            return;
        }
        m_graph = std::move(*loaded);
        m_canvas.clearSelection();
        m_canvas.fitToContent();
        m_bestiary.reset();
        m_documentPath = m_path;
        m_savedSnapshot = snapshot();
        setStatus("Loaded " + m_path, false);
    }

    void EditorState::requestAction(Pending action)
    {
        if (isDirty())
        {
            m_pending = action;
            m_openUnsavedPopup = true;
            return;
        }
        performAction(action);
    }

    void EditorState::performAction(Pending action)
    {
        switch (action)
        {
        case Pending::Exit:
            context().states.requestPop();
            break;
        case Pending::New:
            resetScenario();
            setStatus("New scenario", false);
            break;
        case Pending::Load:
            loadScenario();
            break;
        case Pending::None:
            break;
        }
        m_pending = Pending::None;
    }

    // ------------------------------------------------------------------ интерфейс

    void EditorState::drawToolbar()
    {
        // --- строка 1: файл ---
        if (ImGui::Button("< Back"))
            requestAction(Pending::Exit);
        ImGui::SameLine();
        if (ImGui::Button("New"))
            requestAction(Pending::New);
        ImGui::SameLine();
        ImGui::TextUnformatted("File:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(ImGui::GetFontSize() * 24.f);
        ed::inputTextStd("##path", m_path);
        ImGui::SameLine();
        if (ImGui::Button("Save"))
            requestSave();
        ImGui::SameLine();
        if (ImGui::Button("Load"))
            requestAction(Pending::Load);

        // --- строка 2: добавление узлов и вид ---
        ImGui::TextUnformatted("Add:");
        ImGui::SameLine();
        if (ImGui::Button("Story scene"))
            m_canvas.addNodeAtViewCenter(m_graph, NodeType::Story);
        ImGui::SameLine();
        if (ImGui::Button("Fork"))
            m_canvas.addNodeAtViewCenter(m_graph, NodeType::Choice);
        ImGui::SameLine();
        if (ImGui::Button("Random table"))
            m_canvas.addNodeAtViewCenter(m_graph, NodeType::Random);
        ImGui::SameLine();
        ImGui::TextUnformatted(" | ");
        ImGui::SameLine();
        if (ImGui::Button("Fit view"))
            m_canvas.fitToContent();
        ImGui::SameLine();
        if (ImGui::Button("Auto layout"))
        {
            ed::autoLayout(m_graph);
            m_canvas.fitToContent();
        }
        ImGui::SameLine();
        ImGui::Text("Zoom %d%%", static_cast<int>(m_canvas.zoom() * 100.f + 0.5f));
        ImGui::SameLine();
        ImGui::TextUnformatted(" | UI scale");
        ImGui::SameLine();
        if (ImGui::Button("-"))
        {
            m_scale = std::max(1.0f, m_scale - 0.25f);
            applyScale();
        }
        ImGui::SameLine();
        if (ImGui::Button("+"))
        {
            m_scale = std::min(3.0f, m_scale + 0.25f);
            applyScale();
        }
    }

    void EditorState::drawSceneTab()
    {
        Node *node = m_canvas.selected().empty() ? nullptr : m_graph.find(m_canvas.selected());
        if (!node)
        {
            ImGui::TextWrapped("Select a scene on the map to edit it.\n\n"
                               "Right-click the map to add scenes. Drag from the round port on the right "
                               "side of a scene onto another scene to link them.");
            return;
        }
        if (m_inspector.draw(m_graph, *node))
        {
            m_deleteTarget = node->id();
            m_openDeletePopup = true;
        }
    }

    void EditorState::drawScenarioTab()
    {
        ImGui::TextUnformatted("Scenario title");
        ImGui::SetNextItemWidth(-FLT_MIN);
        ed::inputTextStd("##scenario_title", m_graph.title);

        ImGui::TextUnformatted("Description");
        ed::inputMultilineStd("##scenario_description", m_graph.description, 5);

        ImGui::TextUnformatted("Start scene");
        NodeId start = m_graph.startId();
        if (ed::nodeCombo("##start", m_graph, start, NodeId{}) && !start.empty())
            m_graph.setStart(start);

        ImGui::SeparatorText("Validation");
        if (m_issues.empty())
        {
            ImGui::TextColored(ImVec4(0.35f, 0.85f, 0.45f, 1.f), "No problems found. The scenario is playable.");
            return;
        }

        const bool playable = ScenarioGraph::isPlayable(m_issues);
        if (playable)
            ImGui::TextColored(ImVec4(0.95f, 0.75f, 0.3f, 1.f), "Playable, but with %d warning(s).",
                               static_cast<int>(m_issues.size()));
        else
            ImGui::TextColored(ImVec4(0.9f, 0.3f, 0.3f, 1.f), "Not playable: %d problem(s).",
                               static_cast<int>(m_issues.size()));

        for (std::size_t i = 0; i < m_issues.size(); ++i)
        {
            const ValidationIssue &issue = m_issues[i];
            const bool isError = issue.severity == ValidationIssue::Severity::Error;
            ImGui::PushID(static_cast<int>(i));
            ImGui::PushStyleColor(ImGuiCol_Text, isError ? ImVec4(0.95f, 0.45f, 0.4f, 1.f)
                                                         : ImVec4(0.95f, 0.8f, 0.4f, 1.f));
            const std::string text = (isError ? "[!] " : "[?] ") + issue.message;
            if (ImGui::Selectable(text.c_str()) && !issue.node.empty())
            {
                if (const Node *node = m_graph.find(issue.node))
                {
                    m_canvas.select(issue.node);
                    m_canvas.focusOn(*node);
                    m_focusSceneTab = true;
                }
            }
            ImGui::PopStyleColor();
            ImGui::PopID();
        }
    }

    void EditorState::drawSidePanel(const ImVec2 &size)
    {
        ImGui::BeginChild("editor_side", size, true);
        if (ImGui::BeginTabBar("editor_tabs"))
        {
            const ImGuiTabItemFlags sceneFlags = m_focusSceneTab ? ImGuiTabItemFlags_SetSelected : 0;
            m_focusSceneTab = false;

            if (ImGui::BeginTabItem("Scene", nullptr, sceneFlags))
            {
                drawSceneTab();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Bestiary"))
            {
                if (const auto focus = m_bestiary.draw(m_graph, m_canvas.selected()))
                {
                    if (const Node *node = m_graph.find(*focus))
                    {
                        m_canvas.select(*focus);
                        m_canvas.focusOn(*node);
                    }
                }
                ImGui::EndTabItem();
            }
            const std::string scenarioLabel =
                m_issues.empty() ? std::string("Scenario") : "Scenario (" + std::to_string(m_issues.size()) + ")";
            if (ImGui::BeginTabItem((scenarioLabel + "###scenario_tab").c_str()))
            {
                drawScenarioTab();
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
        ImGui::EndChild();
    }

    void EditorState::drawModals()
    {
        const ImVec2 center = ImGui::GetMainViewport()->GetCenter();

        // --- несохранённые изменения ---
        if (m_openUnsavedPopup)
        {
            ImGui::OpenPopup("Unsaved changes");
            m_openUnsavedPopup = false;
        }
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        if (ImGui::BeginPopupModal("Unsaved changes", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::TextUnformatted("The scenario has unsaved changes.");

            // Загрузка заменяет сценарий, а поле «File» в этот момент указывает на загружаемый файл:
            // сохранять «заодно» нельзя, иначе текущий сценарий затрёт чужой файл.
            const bool offerSave = m_pending != Pending::Load && canQuickSave();
            const std::string &target = m_documentPath.empty() ? m_path : m_documentPath;
            if (offerSave)
                ImGui::TextDisabled("Save will write to: %s", target.c_str());
            else if (m_pending == Pending::Load)
                ImGui::TextDisabled("Cancel, press Save, then Load - or discard the changes.");
            else
                ImGui::TextDisabled("Cancel and press Save with a new file name first.");
            ImGui::Separator();

            if (offerSave)
            {
                if (ImGui::Button("Save"))
                {
                    const Pending action = m_pending;
                    ImGui::CloseCurrentPopup();
                    if (saveTo(target))
                        performAction(action);
                    else
                        m_pending = Pending::None; // причина ошибки видна в строке статуса
                }
                ImGui::SameLine();
            }
            if (ImGui::Button("Discard"))
            {
                ImGui::CloseCurrentPopup();
                performAction(m_pending);
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel"))
            {
                ImGui::CloseCurrentPopup();
                m_pending = Pending::None;
            }
            ImGui::EndPopup();
        }

        // --- перезапись существующего файла ---
        if (m_openOverwritePopup)
        {
            ImGui::OpenPopup("Overwrite file");
            m_openOverwritePopup = false;
        }
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        if (ImGui::BeginPopupModal("Overwrite file", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("\"%s\" already exists.", m_path.c_str());
            ImGui::TextUnformatted("Replace it with the current scenario?");
            ImGui::Separator();
            if (ImGui::Button("Overwrite"))
            {
                ImGui::CloseCurrentPopup();
                saveTo(m_path);
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel"))
                ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }

        // --- удаление сцены ---
        if (m_openDeletePopup)
        {
            ImGui::OpenPopup("Delete scene");
            m_openDeletePopup = false;
        }
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        if (ImGui::BeginPopupModal("Delete scene", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            const Node *node = m_graph.find(m_deleteTarget);
            if (!node)
            {
                ImGui::CloseCurrentPopup();
            }
            else
            {
                ImGui::Text("Delete scene \"%s\"?", node->title.empty() ? node->id().c_str() : node->title.c_str());
                ImGui::TextUnformatted("Links leading to it will be cleared.");
                ImGui::Separator();
                if (ImGui::Button("Delete"))
                {
                    ed::removeNode(m_graph, m_deleteTarget);
                    m_canvas.clearSelection();
                    m_deleteTarget.clear();
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button("Cancel"))
                    ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }

    void EditorState::drawUi()
    {
        const sf::Vector2u size = context().window.getSize();

        ImGui::SetNextWindowPos(ImVec2(0.f, 0.f), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(static_cast<float>(size.x), static_cast<float>(size.y)), ImGuiCond_Always);
        ImGui::Begin("Scenario Editor", nullptr,
                     ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
                         ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoTitleBar);

        drawToolbar();
        ImGui::Separator();

        m_issues = m_graph.validate();

        const ImVec2 avail = ImGui::GetContentRegionAvail();
        const float statusHeight = ImGui::GetFrameHeightWithSpacing();
        const float spacing = ImGui::GetStyle().ItemSpacing.x;
        const float sideWidth = std::max(avail.x * 0.34f, ImGui::GetFontSize() * 22.f);
        const float bodyHeight = std::max(avail.y - statusHeight, 50.f);

        m_canvas.draw(m_graph, m_issues, ImVec2(std::max(avail.x - sideWidth - spacing, 100.f), bodyHeight));

        NodeId deleteRequest;
        if (m_canvas.takeDeleteRequest(deleteRequest))
        {
            m_deleteTarget = deleteRequest;
            m_openDeletePopup = true;
        }

        ImGui::SameLine();
        drawSidePanel(ImVec2(sideWidth, bodyHeight));

        // --- строка состояния ---
        ImGui::Text("%d scene(s)", static_cast<int>(m_graph.size()));
        ImGui::SameLine();
        if (!m_status.empty())
            ImGui::TextColored(m_statusIsError ? ImVec4(0.95f, 0.4f, 0.35f, 1.f) : ImVec4(0.6f, 0.85f, 0.6f, 1.f),
                               "  %s", m_status.c_str());

        drawModals();
        ImGui::End();
    }

} // namespace dnd::core
