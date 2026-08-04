#include "network_panel.h"
#include "engine/networking/network_service.h"
#include "engine/core/service_locator.h"
#include "imgui.h"
#include <cstdint>
#include <cstdlib>
#include <cstring>

namespace Chained
{

	NetworkPanel::NetworkPanel()
	{
		m_Name = "Network";
	}

	NetworkPanel::~NetworkPanel() = default;

	void NetworkPanel::OnImGuiRender(bool /*readOnly*/)
	{
		if (!m_IsOpen)
		{
			return;
		}

		ImGui::SetNextWindowSize(ImVec2(420, 380), ImGuiCond_FirstUseEver);

		if (ImGui::Begin(m_Name.c_str(), &m_IsOpen))
		{
			auto* net = ServiceLocator::TryGet<Network>();

			// --- Status bar ---
			if (net && net->IsConnected())
			{
				const char* roleStr = net->IsHost() ? "HOST" : "CLIENT";
				ImGui::Text("Status: %s", roleStr);
				ImGui::SameLine();
				ImGui::TextDisabled("(%zu connected)", net->GetClientCount());
			}
			else
			{
				ImGui::TextDisabled("Status: Offline");
			}

			ImGui::Separator();

			// --- Tab Bar ---
			if (ImGui::BeginTabBar("##NetworkTabs"))
			{
				if (ImGui::BeginTabItem("Host"))
				{
					DrawHostTab();
					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem("Connect"))
				{
					DrawConnectTab();
					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem("Players"))
				{
					DrawPlayersTab();
					ImGui::EndTabItem();
				}

				ImGui::EndTabBar();
			}

			// --- Status message ---
			if (!m_StatusMessage.empty())
			{
				ImGui::Separator();
				if (m_StatusIsError)
				{
					ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%s", m_StatusMessage.c_str());
				}
				else
				{
					ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "%s", m_StatusMessage.c_str());
				}
			}
		}

		ImGui::End();
	}

	void NetworkPanel::DrawHostTab()
	{
		auto* net = ServiceLocator::TryGet<Network>();

		if (net && net->IsHost())
		{
			ImGui::Text("Server is running.");
			ImGui::Separator();

			ImGui::Text("Port: %s", m_HostPort);
			ImGui::Text("Max clients: %d", m_MaxClients);
			ImGui::Text("Connected: %zu", net->GetClientCount());

			ImGui::Separator();

			ImGui::BeginDisabled();
			ImGui::Button("Stop Server");
			ImGui::EndDisabled();
		}
		else
		{
			ImGui::Text("Start a server to host a game.");
			ImGui::Separator();

			ImGui::SetNextItemWidth(120);
			ImGui::InputText("Port", m_HostPort, sizeof(m_HostPort), ImGuiInputTextFlags_CharsDecimal);

			ImGui::SetNextItemWidth(120);
			ImGui::InputInt("Max Clients", &m_MaxClients, 1, 10);
			if (m_MaxClients < 1)
			{
				m_MaxClients = 1;
			}
			if (m_MaxClients > 32)
			{
				m_MaxClients = 32;
			}

			ImGui::Separator();

			if (ImGui::Button("Start Server", ImVec2(ImGui::GetContentRegionAvail().x, 0)))
			{
				uint16_t port = static_cast<uint16_t>(std::atoi(m_HostPort));
				if (port == 0)
				{
					port = 27886;
				}

				if (net)
				{
					net->HostGame(port, m_MaxClients);
					m_StatusMessage = "Server started on port " + std::string(m_HostPort);
					m_StatusIsError = false;
				}
				else
				{
					m_StatusMessage = "Network service not available!";
					m_StatusIsError = true;
				}
			}
		}
	}

	void NetworkPanel::DrawConnectTab()
	{
		auto* net = ServiceLocator::TryGet<Network>();

		if (net && net->IsClient())
		{
			ImGui::Text("Connected to server.");
			ImGui::Separator();

			ImGui::Text("Server: %s:%s", m_ConnectIP, m_ConnectPort);

			ImGui::Separator();

			if (ImGui::Button("Disconnect", ImVec2(ImGui::GetContentRegionAvail().x, 0)))
			{
				net->Disconnect();
				m_StatusMessage = "Disconnected.";
				m_StatusIsError = false;
			}
		}
		else
		{
			ImGui::Text("Connect to an existing server.");
			ImGui::Separator();

			ImGui::SetNextItemWidth(200);
			ImGui::InputText("IP", m_ConnectIP, sizeof(m_ConnectIP));

			ImGui::SetNextItemWidth(120);
			ImGui::InputText("Port", m_ConnectPort, sizeof(m_ConnectPort), ImGuiInputTextFlags_CharsDecimal);

			ImGui::Separator();

			if (ImGui::Button("Connect", ImVec2(ImGui::GetContentRegionAvail().x, 0)))
			{
				uint16_t port = static_cast<uint16_t>(std::atoi(m_ConnectPort));
				if (port == 0)
				{
					port = 27886;
				}

				if (net)
				{
					net->ConnectTo(m_ConnectIP, port);
					m_StatusMessage =
						"Connecting to " + std::string(m_ConnectIP) + ":" + std::string(m_ConnectPort) + "...";
					m_StatusIsError = false;
				}
				else
				{
					m_StatusMessage = "Network service not available!";
					m_StatusIsError = true;
				}
			}
		}
	}

	void NetworkPanel::DrawPlayersTab()
	{
		auto* net = ServiceLocator::TryGet<Network>();

		if (!net || net->GetRole() == Role::Offline)
		{
			ImGui::TextDisabled("Not connected.");
			return;
		}

		ImGui::Text("Connected players:");
		ImGui::Separator();

		// Table header
		ImGui::Columns(3, "##PlayerColumns", true);
		ImGui::SetColumnWidth(0, 60);
		ImGui::SetColumnWidth(1, 180);
		ImGui::SetColumnWidth(2, 100);

		ImGui::Text("ID");
		ImGui::NextColumn();
		ImGui::Text("Role");
		ImGui::NextColumn();
		ImGui::Text("Status");
		ImGui::NextColumn();
		ImGui::Separator();

		// Host entry (self)
		if (net->IsHost())
		{
			ImGui::Text("0");
			ImGui::NextColumn();
			ImGui::Text("Host (You)");
			ImGui::NextColumn();
			ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "Playing");
			ImGui::NextColumn();
		}

		// Client entries
		size_t clientCount = net->GetClientCount();
		for (size_t i = 0; i < clientCount; ++i)
		{
			ImGui::Text("%zu", i + 1);
			ImGui::NextColumn();
			ImGui::Text("Client");
			ImGui::NextColumn();
			ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "Connected");
			ImGui::NextColumn();
		}

		ImGui::Columns(1);

		if (clientCount == 0 && net->IsHost())
		{
			ImGui::TextDisabled("No clients connected yet.");
		}
	}

} // namespace Chained
