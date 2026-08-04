#ifndef CH_NETWORK_PANEL_H
#define CH_NETWORK_PANEL_H

#include "panel.h"
#include <string>

namespace Chained
{

	class NetworkPanel : public Panel
	{
	public:
		NetworkPanel();
		~NetworkPanel() override;

		void OnImGuiRender(bool readOnly = false) override;

	private:
		void DrawHostTab();
		void DrawConnectTab();
		void DrawPlayersTab();

		// Host settings
		char m_HostPort[16] = "27886";
		int m_MaxClients = 8;

		// Connect settings
		char m_ConnectIP[128] = "127.0.0.1";
		char m_ConnectPort[16] = "27886";

		// Status
		std::string m_StatusMessage;
		bool m_StatusIsError = false;
	};

} // namespace Chained

#endif // CH_NETWORK_PANEL_H
