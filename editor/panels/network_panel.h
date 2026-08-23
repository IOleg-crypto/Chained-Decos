#ifndef CH_NETWORK_PANEL_H
#define CH_NETWORK_PANEL_H

#include "panel.h"
#include <atomic>
#include <future>
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
		char m_HostPort[16] = "7777";
		int m_MaxClients = 8;

		// Connect settings
		char m_ConnectIP[128] = "127.0.0.1";
		char m_ConnectPort[16] = "7777";

		// Status
		std::string m_StatusMessage;
		bool m_StatusIsError = false;

		// Public IP fetch
		std::string m_PublicIP;	  ///< cached IPv4 result from api.ipify.org
		std::string m_PublicIPv6; ///< cached IPv6 result from ipv6.api.ipify.org
		std::future<std::string> m_IpFuture;
		std::future<std::string> m_IpFutureIPv6;
		bool m_FetchingIP = false;
		bool m_FetchingIPv6 = false;
	};

} // namespace Chained

#endif // CH_NETWORK_PANEL_H
