#include "console_panel.h"
#include "engine/app/application.h"
#include "imgui.h"
#include <algorithm>
#include <cctype>

namespace Chained
{
	ConsolePanel::ConsolePanel()
	{
		m_Name = "Console";

		// Consume initially buffered messages before UI loop starts
		auto bufferedMessages = Log::ConsumeBufferedMessages();
		for (auto& entry : bufferedMessages)
		{
			m_Messages.push_back(std::move(entry));
		}

		m_ScrollToBottom = !m_Messages.empty();
	}

	ConsolePanel::~ConsolePanel() = default;

	void ConsolePanel::OnImGuiRender(bool readOnly)
	{
		if (!m_IsOpen)
		{
			return;
		}

		ImGui::SetNextWindowSize(ImVec2(520, 600), ImGuiCond_FirstUseEver);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 10));

		if (ImGui::Begin(m_Name.c_str(), &m_IsOpen))
		{
			bool filtersChanged = false;

			// --- 1. Control Panel ---
			ImGui::BeginDisabled(readOnly);

			if (ImGui::Button("Clear"))
			{
				Clear();
				filtersChanged = true;
			}
			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip("Clear all log messages");
			}
			ImGui::SameLine();

			ImGui::SetNextItemWidth(150);
			// If user types into filter, we flag that index rebuild is required
			if (ImGui::InputTextWithHint("##filter", "Filter...", m_FilterBuffer, sizeof(m_FilterBuffer)))
			{
				filtersChanged = true;
			}
			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip("Filter log messages by text");
			}
			ImGui::SameLine();

			const char* levels[] = {"TRACE", "INFO", "WARNING", "ERROR", "FATAL", "NONE"};
			ImGui::SetNextItemWidth(120);
			if (ImGui::Combo("Level", &m_LogLevel, levels, IM_ARRAYSIZE(levels)))
			{
				filtersChanged = true;
			}
			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip("Filter logs by minimum severity level");
			}

			ImGui::EndDisabled();
			ImGui::Separator();

			// --- 2. Ingest New Logs ---
			auto bufferedMessages = Log::ConsumeBufferedMessages();
			bool hasNewMessages = !bufferedMessages.empty();

			if (hasNewMessages || filtersChanged)
			{
				std::lock_guard<std::mutex> lock(m_LogMutex);

				if (hasNewMessages)
				{
					for (auto& entry : bufferedMessages)
					{
						m_Messages.push_back(std::move(entry));
					}

					while (m_Messages.size() > MAX_MESSAGES)
					{
						m_Messages.pop_front();
					}

					m_ScrollToBottom = true;
				}

				// --- 3. Optimized Rebuild of Visible Indices ---
				// We only rebuild when new logs arrive OR UI filter parameters change.
				m_VisibleIndices.clear();

				std::string filterStr = m_FilterBuffer;
				std::transform(filterStr.begin(), filterStr.end(), filterStr.begin(),
							   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });

				for (int i = 0; i < static_cast<int>(m_Messages.size()); ++i)
				{
					if (m_LogLevel != static_cast<int>(LogLevel::LogNone) &&
						static_cast<int>(m_Messages[i].level) < m_LogLevel)
					{
						continue;
					}

					if (!filterStr.empty())
					{
						// Case-insensitive substring match without heavy allocation
						auto it =
							std::search(m_Messages[i].message.begin(), m_Messages[i].message.end(), filterStr.begin(),
										filterStr.end(), [](unsigned char ch1, unsigned char ch2) {
											return std::tolower(ch1) == ch2; // filterStr is already lower
										});

						if (it == m_Messages[i].message.end())
						{
							continue;
						}
					}
					m_VisibleIndices.push_back(i);
				}
			}

			// --- 4. Content Scrolling Region ---
			const float footerHeight = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
			ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footerHeight), false, ImGuiWindowFlags_HorizontalScrollbar);

			{
				std::lock_guard<std::mutex> lock(m_LogMutex);

				ImGuiListClipper clipper;
				clipper.Begin(static_cast<int>(m_VisibleIndices.size()));

				while (clipper.Step())
				{
					for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i)
					{
						const int msgIdx = m_VisibleIndices[i];
						const auto& msg = m_Messages[msgIdx];

						ImVec4 color;
						switch (msg.level)
						{
						case LogLevel::LogTrace:
							color = ImVec4(0.7f, 0.7f, 0.7f, 1.0f);
							break;
						case LogLevel::LogInfo:
							color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
							break;
						case LogLevel::LogWarning:
							color = ImVec4(1.0f, 0.8f, 0.0f, 1.0f);
							break;
						case LogLevel::LogError:
							color = ImVec4(1.0f, 0.2f, 0.2f, 1.0f);
							break;
						case LogLevel::LogFatal:
							color = ImVec4(1.0f, 0.0f, 1.0f, 1.0f);
							break;
						default:
							color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
							break;
						}

						// Render Timestamp (Gray color)
						ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
						ImGui::TextUnformatted(msg.timestamp.c_str());
						ImGui::PopStyleColor();

						ImGui::SameLine();

						// Render Log Message
						ImGui::PushStyleColor(ImGuiCol_Text, color);
						ImGui::TextUnformatted(msg.message.c_str());
						ImGui::PopStyleColor();
					}
				}
			}

			// Stick to bottom if scrolled down or explicitly triggered
			if (m_ScrollToBottom || (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()))
			{
				ImGui::SetScrollHereY(1.0f);
			}
			m_ScrollToBottom = false;

			ImGui::EndChild();
		}
		ImGui::End();
		ImGui::PopStyleVar();
	}

	void ConsolePanel::Clear()
	{
		std::lock_guard<std::mutex> lock(m_LogMutex);
		m_Messages.clear();
		m_VisibleIndices.clear();
	}

} // namespace Chained