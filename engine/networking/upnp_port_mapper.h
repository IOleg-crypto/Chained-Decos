#ifndef CH_UPNP_PORT_MAPPER_H
#define CH_UPNP_PORT_MAPPER_H

#include <cstdint>
#include <string>

namespace Chained
{

	class UpnpPortMapper
	{
	public:
		UpnpPortMapper() = default;
		~UpnpPortMapper();

		UpnpPortMapper(const UpnpPortMapper&) = delete;
		UpnpPortMapper& operator=(const UpnpPortMapper&) = delete;

		bool Initialize();
		void Shutdown();

		bool AddMapping(uint16_t port, const char* protocol = "UDP", const char* description = "Chained Engine");
		bool RemoveMapping(uint16_t port, const char* protocol = "UDP");
		std::string GetPublicIP();
		bool IsAvailable() const
		{
			return m_Available;
		}

	private:
		void CleanupDiscovery();

		void* m_DevList = nullptr;
		void* m_URLs = nullptr;
		void* m_IgdData = nullptr;
		void* m_ControlURL = nullptr;
		void* m_ServiceType = nullptr;
		bool m_Available = false;
		char m_LanAddress[64] = {};
	};

} // namespace Chained

#endif // CH_UPNP_PORT_MAPPER_H
