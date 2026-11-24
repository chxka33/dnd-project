#include "core/SimulatorState.hpp"

#include "core/Application.hpp"
#include "entities/ItemPresets.hpp"
#include "gui/Widgets.hpp"
#include "gui/editor/EditorWidgets.hpp"
#include "scenario/CharacterSerializer.hpp"
#include "scenario/ChoiceNode.hpp"
#include "scenario/RandomNode.hpp"
#include "scenario/StoryNode.hpp"
#include "utils/ExecutablePath.hpp"

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Keyboard.hpp>

#include <algorithm>
#include <filesystem>
#include <system_error>
#include <vector>

namespace dnd::core
{
    using namespace dnd::scenario;
    namespace ed = dnd::gui::editor;

    namespace
    {
        const ImVec4 kMuted(0.65f, 0.65f, 0.65f, 1.f);
        const ImVec4 kAccent(0.95f, 0.80f, 0.35f, 1.f);
        const ImVec4 kGood(0.35f, 0.80f, 0.40f, 1.f);
        const ImVec4 kBad(0.90f, 0.30f, 0.30f, 1.f);

        std::string sceneLabel(const ScenarioGraph &graph, const NodeId &id)
        {
            const Node *node = graph.find(id);
            if (!node)
                return id.empty() ? std::string("(none)") : "(missing) " + id;
            return node->title.empty() ? id : node->title;
        }

        std::string lootText(const Loot &loot)
        {
            std::string text;
            if (loot.gold > 0)
                text = std::to_string(loot.gold) + " gp";
            for (const LootItem &item : loot.items)
            {
                if (item.quantity <= 0 || item.name.empty())
                    continue;
                if (!text.empty())
                    text += ", ";
                text += item.name;
                if (item.quantity > 1)
                    text += " x" + std::to_string(item.quantity);
            }
            return text;
        }

        std::vector<std::filesystem::path> listFiles(const char *extension)
        {
            namespace fs = std::filesystem;
            std::vector<fs::path> files;
            std::error_code ec;
            const fs::path dir = utils::executableDir();
            if (fs::is_directory(dir, ec))
            {
                for (const auto &entry : fs::directory_iterator(dir, ec))
                    if (entry.is_regular_file() && entry.path().extension() == extension)
                        files.push_back(entry.path());
                std::sort(files.begin(), files.end());
            }
            return files;
        }
    } // namespace

    // ------------------------------------------------------------------ жизненный цикл

    void SimulatorState::applyScale() const
    {
        ImGui::GetIO().FontGlobalScale = m_scale;
    }

    void SimulatorState::onEnter()
    {
        m_previousScale = ImGui::GetIO().FontGlobalScale;
        applyScale();
        entities::registerRosterItems(context().roster);
        m_canvas.setReadOnly(true);
        m_canvas.fitToContent();
        setStatus("Load a scenario (.dndsc) to start the session.", false);
        if (m_runner.hasScenario())
            setStatus("Session in progress.", false);
    }

    void SimulatorState::onExit()
    {
        ImGui::GetIO().FontGlobalScale = m_previousScale;
    }

    void SimulatorState::handleEvent(const sf::Event &event)
    {
        if (event.type != sf::Event::KeyPressed || event.key.code != sf::Keyboard::Escape)
            return;
        if (ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel))
            return;
        if (m_canvas.isInteracting())
        {
            m_canvas.cancelInteraction();
            return;
        }
        context().states.requestPop();
    }

    void SimulatorState::setStatus(std::string text, bool error)
    {
        m_status = std::move(text);
        m_statusIsError = error;
    }

    void SimulatorState::flushCombatEvents()
    {
        m_runner.appendEvents(m_combat.takeEvents());
    }

    // ------------------------------------------------------------------ интерфейс

    void SimulatorState::drawToolbar()
    {
        if (ImGui::Button("< Back"))
            context().states.requestPop();
        ImGui::SameLine();
        if (ImGui::Button("Load scenario"))
            m_openLoadScenario = true;
        ImGui::SameLine();
        if (ImGui::Button("Load party"))
            m_openLoadParty = true;
        if (context().roster.empty())
        {
            ImGui::SameLine();
            if (ImGui::Button("Demo party"))
            {
                context().roster.addDemoParty();
                setStatus("Demo party added.", false);
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Save session"))
            m_openSaveSession = true;
        ImGui::SameLine();
        if (ImGui::Button("Load session"))
            m_openLoadSession = true;
        ImGui::SameLine();
        ImGui::BeginDisabled(!m_runner.hasScenario());
        if (ImGui::Button("Restart"))
        {
            m_runner.restartScenario();
            setStatus("Scenario restarted.", false);
        }
        ImGui::SameLine();
        if (ImGui::Button("Fit map"))
            m_canvas.fitToContent();
        ImGui::SameLine();
        if (ImGui::Button("Focus party"))
        {
            if (const Node *node = m_runner.currentNode())
                m_canvas.focusOn(*node);
        }
        ImGui::EndDisabled();
        ImGui::SameLine();
        ImGui::TextUnformatted("UI scale");
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
        if (!m_status.empty())
        {
            ImGui::SameLine();
            ImGui::TextColored(m_statusIsError ? kBad : kGood, "%s", m_status.c_str());
        }
    }

    void SimulatorState::drawLog()
    {
        const auto &log = m_runner.eventLog();
        ImGui::TextUnformatted("Session log");
        ImGui::SameLine();
        if (ImGui::SmallButton("Clear log"))
            m_runner.clearLog();

        const float inputHeight = ImGui::GetFrameHeightWithSpacing();
        ImGui::BeginChild("session_log", ImVec2(0.f, -inputHeight), true);
        for (std::size_t i = 0; i < log.size(); ++i)
            ImGui::TextWrapped("%zu. %s", i + 1, log[i].c_str());
        if (log.size() != m_logShown)
        {
            ImGui::SetScrollHereY(1.f);
            m_logShown = log.size();
        }
        ImGui::EndChild();

        ImGui::SetNextItemWidth(-ImGui::CalcTextSize("Add note").x - ImGui::GetStyle().ItemSpacing.x * 3.f -
                                ImGui::GetStyle().FramePadding.x * 2.f);
        const bool submitted = ed::inputTextStd("##note", m_note, ImGuiInputTextFlags_EnterReturnsTrue);
        ImGui::SameLine();
        if (ImGui::Button("Add note") || submitted)
        {
            if (!m_note.empty())
                m_runner.logEvent("Note: " + m_note);
            m_note.clear();
        }
    }

    void SimulatorState::drawMapAndLog(const ImVec2 &size)
    {
        ImGui::BeginGroup();
        const float logHeight = std::max(size.y * 0.3f, ImGui::GetFontSize() * 9.f);
        const float mapHeight = std::max(size.y - logHeight - ImGui::GetStyle().ItemSpacing.y, 60.f);

        m_canvas.setActiveNode(m_runner.currentNodeId());
        static const std::vector<ValidationIssue> kNoIssues;
        if (m_runner.hasScenario())
        {
            m_canvas.draw(m_runner.graph(), kNoIssues, ImVec2(size.x, mapHeight));
        }
        else
        {
            ImGui::BeginChild("map_placeholder", ImVec2(size.x, mapHeight), true);
            ImGui::TextWrapped("No scenario loaded. Press \"Load scenario\" to pick a .dndsc file.");
            ImGui::EndChild();
        }

        ImGui::BeginChild("log_area", ImVec2(size.x, 0.f));
        drawLog();
        ImGui::EndChild();
        ImGui::EndGroup();
    }

    void SimulatorState::drawLootBlock(const Node &node)
    {
        if (!node.loot)
            return;
        const std::string summary = lootText(*node.loot);
        if (summary.empty())
            return;

        ImGui::SeparatorText("Loot");
        ImGui::TextColored(kAccent, "%s", summary.c_str());

        entities::Roster &roster = context().roster;
        std::vector<int> heroes;
        for (std::size_t i = 0; i < roster.size(); ++i)
            if (roster.at(i).side == entities::Side::Player)
                heroes.push_back(static_cast<int>(i));
        if (heroes.empty())
        {
            ImGui::TextColored(kMuted, "No heroes in the party.");
            return;
        }
        if (std::find(heroes.begin(), heroes.end(), m_lootTarget) == heroes.end())
            m_lootTarget = heroes.front();

        const std::string preview = gui::characterLabel(roster.at(static_cast<std::size_t>(m_lootTarget)).character);
        ImGui::SetNextItemWidth(ImGui::GetFontSize() * 14.f);
        if (ImGui::BeginCombo("##loot_hero", preview.c_str()))
        {
            for (int idx : heroes)
            {
                ImGui::PushID(idx);
                if (ImGui::Selectable(gui::characterLabel(roster.at(static_cast<std::size_t>(idx)).character).c_str(),
                                      idx == m_lootTarget))
                    m_lootTarget = idx;
                ImGui::PopID();
            }
            ImGui::EndCombo();
        }
        ImGui::SameLine();
        if (ImGui::Button("Give loot to hero"))
        {
            const entities::LootResult got = m_runner.giveCurrentNodeLoot(roster, static_cast<std::size_t>(m_lootTarget));
            setStatus("Given " + std::to_string(got.gold) + " gp and " + std::to_string(got.itemUnits) + " item(s).",
                      false);
        }
    }

    void SimulatorState::drawNavigation(const Node &node)
    {
        ImGui::SeparatorText("Next");
        const ScenarioGraph &graph = m_runner.graph();

        switch (node.type())
        {
        case NodeType::Story:
        {
            const auto &story = static_cast<const StoryNode &>(node);
            if (story.isEnding())
            {
                ImGui::TextColored(kMuted, "This is the end of the story branch.");
            }
            else
            {
                const std::string label = "Next scene -> " + sceneLabel(graph, story.next());
                if (ImGui::Button(label.c_str()))
                    m_runner.advanceStory();
            }
            break;
        }
        case NodeType::Choice:
        {
            const auto &choice = static_cast<const ChoiceNode &>(node);
            ImGui::TextColored(kMuted, "Players choose:");
            std::size_t picked = choice.choices().size();
            for (std::size_t i = 0; i < choice.choices().size(); ++i)
            {
                const Choice &c = choice.choices()[i];
                const std::string label = std::to_string(i + 1) + ". " + (c.label.empty() ? "(no text)" : c.label) +
                                          " -> " + sceneLabel(graph, c.target);
                ImGui::PushID(static_cast<int>(i));
                ImGui::BeginDisabled(c.target.empty() || !graph.contains(c.target));
                if (ImGui::Button(label.c_str()))
                    picked = i;
                ImGui::EndDisabled();
                ImGui::PopID();
            }
            if (picked < choice.choices().size())
                m_runner.chooseOption(picked);
            break;
        }
        case NodeType::Random:
        {
            const auto &random = static_cast<const RandomNode &>(node);
            ImGui::Text("Roll table (d%d):", random.diceSides());
            for (const RollTableEntry &e : random.entries())
            {
                if (e.minRoll == e.maxRoll)
                    ImGui::BulletText("%d: %s -> %s", e.minRoll, e.label.c_str(), sceneLabel(graph, e.target).c_str());
                else
                    ImGui::BulletText("%d-%d: %s -> %s", e.minRoll, e.maxRoll, e.label.c_str(),
                                      sceneLabel(graph, e.target).c_str());
            }
            if (ImGui::Button("Roll & go"))
                m_runner.rollRandomEncounter(m_rng, true);
            ImGui::SameLine();
            if (ImGui::Button("Roll only"))
                m_runner.rollRandomEncounter(m_rng, false);

            if (const auto &last = m_runner.lastRollResult())
            {
                ImGui::TextColored(kAccent, "Rolled %d (d%d): %s", last->roll, last->diceSides, last->label.c_str());
                if (!last->next.empty() && graph.contains(last->next))
                {
                    const std::string go = "Go to " + sceneLabel(graph, last->next);
                    if (ImGui::Button(go.c_str()))
                        m_runner.followLastRoll();
                }
            }
            break;
        }
        }
    }

    void SimulatorState::drawSceneTab()
    {
        if (!m_runner.hasScenario())
        {
            ImGui::TextWrapped("Load a scenario to begin.");
            return;
        }
        const ScenarioGraph &graph = m_runner.graph();

        // --- принудительный переход: выбранный на карте узел или любой из списка ---
        const NodeId &selected = m_canvas.selected();
        if (!selected.empty() && selected != m_runner.currentNodeId() && graph.contains(selected))
        {
            ImGui::TextColored(kAccent, "Selected on map: %s", sceneLabel(graph, selected).c_str());
            ImGui::SameLine();
            const std::string label = "Force jump to \"" + sceneLabel(graph, selected) + "\"";
            if (ImGui::SmallButton(label.c_str()))
                m_runner.moveToNode(selected, "DM override");
        }
        NodeId jump;
        if (ed::nodeCombo("Jump to any scene", graph, jump, m_runner.currentNodeId()) && !jump.empty())
            m_runner.moveToNode(jump, "DM override");

        const Node *node = m_runner.currentNode();
        if (!node)
        {
            ImGui::TextColored(kBad, "The party is not in any scene. Use \"Restart\" or jump to a scene.");
            return;
        }

        ImGui::SeparatorText("Current scene");
        ImGui::TextColored(kAccent, "%s", node->title.empty() ? node->id().c_str() : node->title.c_str());
        ImGui::SameLine();
        ImGui::TextColored(kMuted, "[%s]  %s", node->id().c_str(), ed::nodeTypeLabel(node->type()));

        if (!node->text.empty())
        {
            ImGui::TextUnformatted("Read aloud:");
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.92f, 0.88f, 0.75f, 1.f));
            ImGui::TextWrapped("%s", node->text.c_str());
            ImGui::PopStyleColor();
        }

        if (!node->dmNotes.empty())
        {
            ImGui::Checkbox("Show DM notes", &m_showDmNotes);
            if (m_showDmNotes)
            {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.75f, 0.85f, 0.95f, 1.f));
                ImGui::TextWrapped("%s", node->dmNotes.c_str());
                ImGui::PopStyleColor();
            }
        }

        if (node->check)
        {
            const CheckDefinition &c = *node->check;
            ImGui::SeparatorText("Challenge");
            ImGui::Text("%s %s%s%s, DC %d", c.type == CheckType::SavingThrow ? "Saving throw" : "Skill check",
                        toString(c.ability), c.skill.empty() ? "" : " / ", c.skill.c_str(), c.dc);
            ImGui::SameLine();
            if (ImGui::SmallButton("Open in Combat tab"))
                m_focusTab = 2;
        }

        drawLootBlock(*node);

        if (!node->enemyIds.empty())
        {
            ImGui::SeparatorText("Foes");
            for (const std::string &id : node->enemyIds)
            {
                const EnemyTemplate *tpl = graph.findEnemy(id);
                ImGui::BulletText("%s", tpl ? tpl->name.c_str() : (id + " (missing)").c_str());
            }
            if (ImGui::Button("Load foes into combat tracker"))
            {
                const std::size_t n = m_combat.tracker().loadNodeEnemies(context().roster, graph, *node, &m_rng, false);
                m_runner.logEvent("Loaded " + std::to_string(n) + " enemy(ies) from the scene.");
                m_focusTab = 2;
            }
        }

        drawNavigation(*node);
    }

    void SimulatorState::drawRightPanel(const ImVec2 &size)
    {
        ImGui::BeginChild("sim_side", size, true);
        entities::Roster &roster = context().roster;

        m_combat.setSceneContext(m_runner.hasScenario() ? &m_runner.graph() : nullptr, m_runner.currentNode());

        if (ImGui::BeginTabBar("sim_tabs"))
        {
            auto flags = [&](int index)
            {
                return m_focusTab == index ? ImGuiTabItemFlags_SetSelected : ImGuiTabItemFlags_None;
            };

            if (ImGui::BeginTabItem("Scene", nullptr, flags(0)))
            {
                drawSceneTab();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("DM Party Sheet", nullptr, flags(1)))
            {
                const Node *node = m_runner.currentNode();
                m_dmSheet.setLoot(node && node->loot ? &*node->loot : nullptr);
                const int open = m_dmSheet.draw(roster);
                const int recipient = m_dmSheet.takeLootRecipient();
                if (recipient >= 0 && static_cast<std::size_t>(recipient) < roster.size())
                    m_runner.logEvent("Loot given to " + roster.at(static_cast<std::size_t>(recipient)).character.name +
                                      ".");
                if (open >= 0)
                {
                    m_sheet.select(open);
                    m_focusTab = 3;
                }
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Combat & Checks", nullptr, flags(2)))
            {
                const int open = m_combat.draw(roster, m_rng);
                if (open >= 0)
                {
                    m_sheet.select(open);
                    m_focusTab = 3;
                }
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Character Sheet", nullptr, flags(3)))
            {
                m_sheet.draw(roster);
                ImGui::EndTabItem();
            }
            m_focusTab = -1;
            ImGui::EndTabBar();
        }
        flushCombatEvents();
        ImGui::EndChild();
    }

    void SimulatorState::drawModals()
    {
        namespace fs = std::filesystem;
        const fs::path saveDir = utils::executableDir();
        const ImVec2 center = ImGui::GetMainViewport()->GetCenter();

        auto open = [](bool &flag, const char *name)
        {
            if (flag)
            {
                ImGui::OpenPopup(name);
                flag = false;
            }
        };
        open(m_openLoadScenario, "Load scenario");
        open(m_openLoadParty, "Load party");
        open(m_openSaveSession, "Save session");
        open(m_openLoadSession, "Load session");

        // --- сценарий ---
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        if (ImGui::BeginPopupModal("Load scenario", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            const auto files = listFiles(".dndsc");
            if (files.empty())
                ImGui::TextUnformatted("No saved .dndsc files found.");
            for (const auto &p : files)
            {
                if (ImGui::Selectable(p.filename().string().c_str()))
                {
                    std::string err;
                    if (m_runner.loadScenario(p, &err))
                    {
                        m_canvas.clearSelection();
                        m_canvas.fitToContent();
                        m_combat.tracker().clear();
                        setStatus("Scenario loaded: " + p.filename().string(), false);
                    }
                    else
                        setStatus("Load failed: " + err, true);
                    ImGui::CloseCurrentPopup();
                }
            }
            if (ImGui::Button("Cancel"))
                ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }

        // --- герои ---
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        if (ImGui::BeginPopupModal("Load party", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            const auto files = listFiles(".dndch");
            if (files.empty())
                ImGui::TextUnformatted("No saved .dndch files found.");
            for (const auto &p : files)
            {
                if (ImGui::Selectable(p.filename().string().c_str()))
                {
                    entities::Roster loaded;
                    std::string err;
                    if (scenario::CharacterSerializer::loadRosterFromFile(p, loaded, &err))
                    {
                        context().roster = std::move(loaded);
                        m_combat.tracker().clear();
                        m_sheet.select(0);
                        setStatus("Party loaded: " + p.filename().string(), false);
                    }
                    else
                        setStatus("Load failed: " + err, true);
                    ImGui::CloseCurrentPopup();
                }
            }
            if (ImGui::Button("Cancel"))
                ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }

        // --- сохранение сессии ---
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        if (ImGui::BeginPopupModal("Save session", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::TextUnformatted("File (extension .dndsav):");
            ImGui::InputText("##sname", m_sessionName, sizeof(m_sessionName));
            if (ImGui::Button("Save"))
            {
                std::string name = m_sessionName;
                if (name.empty())
                    name = "session";
                if (fs::path(name).extension() != ".dndsav")
                    name += ".dndsav";
                const fs::path path = saveDir / fs::path(name).filename();
                std::string err;
                if (m_runner.saveSessionToFile(path, context().roster, m_combat.tracker(), &err))
                    setStatus("Session saved: " + path.string(), false);
                else
                    setStatus("Save failed: " + err, true);
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel"))
                ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }

        // --- загрузка сессии ---
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        if (ImGui::BeginPopupModal("Load session", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            const auto files = listFiles(".dndsav");
            if (files.empty())
                ImGui::TextUnformatted("No saved .dndsav files found.");
            for (const auto &p : files)
            {
                if (ImGui::Selectable(p.filename().string().c_str()))
                {
                    std::string err;
                    if (m_runner.loadSessionFromFile(p, context().roster, m_combat.tracker(), &err))
                    {
                        m_canvas.clearSelection();
                        m_canvas.fitToContent();
                        setStatus("Session loaded: " + p.filename().string(), false);
                    }
                    else
                        setStatus("Load failed: " + err, true);
                    ImGui::CloseCurrentPopup();
                }
            }
            if (ImGui::Button("Cancel"))
                ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }
    }

    void SimulatorState::drawUi()
    {
        const sf::Vector2u size = context().window.getSize();

        ImGui::SetNextWindowPos(ImVec2(0.f, 0.f), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(static_cast<float>(size.x), static_cast<float>(size.y)), ImGuiCond_Always);
        ImGui::Begin("Game Simulator", nullptr,
                     ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
                         ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoTitleBar);

        drawToolbar();
        ImGui::Separator();

        const ImVec2 avail = ImGui::GetContentRegionAvail();
        const float spacing = ImGui::GetStyle().ItemSpacing.x;
        const float leftWidth = std::max(avail.x * 0.45f, 200.f);
        const float rightWidth = std::max(avail.x - leftWidth - spacing, 200.f);

        drawMapAndLog(ImVec2(leftWidth, avail.y));
        ImGui::SameLine();
        drawRightPanel(ImVec2(rightWidth, avail.y));

        drawModals();
        ImGui::End();
    }

} // namespace dnd::core
