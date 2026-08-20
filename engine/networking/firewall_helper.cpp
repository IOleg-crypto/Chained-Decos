#include "firewall_helper.h"
#include "engine/core/log.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#pragma comment(lib, "shell32.lib")
#else
#include <unistd.h>
#include <sys/types.h>
#endif

#include <cstdio>
#include <array>
#include <memory>

namespace Chained
{
	namespace Firewall
	{

		/// Runs a shell command and returns the combined stdout+stderr output.
		static std::string RunCommand(const char* cmd)
		{
			std::string result;
			std::array<char, 256> buf{};

#ifdef _WIN32
			std::string fullCmd = std::string(cmd) + " 2>&1";
			std::shared_ptr<FILE> pipe(_popen(fullCmd.c_str(), "r"), _pclose);
#else
			std::string fullCmd = std::string(cmd) + " 2>&1";
			std::shared_ptr<FILE> pipe(popen(fullCmd.c_str(), "r"), pclose);
#endif
			if (!pipe)
			{
				return {};
			}

			while (fgets(buf.data(), static_cast<int>(buf.size()), pipe.get()) != nullptr)
			{
				result += buf.data();
			}

			return result;
		}

#ifdef _WIN32

		/// Checks whether the process is running with administrator privileges.
		static bool IsElevated()
		{
			HANDLE hToken = nullptr;
			if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
			{
				return false;
			}

			DWORD isElevated = 0;
			DWORD size = sizeof(isElevated);
			bool result =
				GetTokenInformation(hToken, TokenElevation, &isElevated, sizeof(isElevated), &size) && isElevated != 0;
			CloseHandle(hToken);
			return result;
		}

		bool AddUDPRule(uint16_t port, const std::string& ruleName)
		{
			if (!IsElevated())
			{
				CH_CORE_WARN("Firewall: Not running as administrator — cannot add firewall rule. "
							 "Run as admin to enable automatic port forwarding.");
				return false;
			}

			if (RuleExists(port, ruleName))
			{
				CH_CORE_INFO("Firewall: Rule '{}' for UDP {} already exists.", ruleName, port);
				return true;
			}

			char cmd[512]{};
			snprintf(cmd, sizeof(cmd),
					 "netsh advfirewall firewall add rule "
					 "name=\"%s\" dir=in action=allow protocol=udp localport=%u "
					 "enable=yes profile=any",
					 ruleName.c_str(), static_cast<unsigned>(port));

			std::string output = RunCommand(cmd);

			if (output.find("Ok.") != std::string::npos)
			{
				CH_CORE_INFO("Firewall: Added inbound UDP rule '{}' for port {}.", ruleName, port);
				return true;
			}

			CH_CORE_WARN("Firewall: Failed to add rule '{}'. Output: {}", ruleName, output);
			return false;
		}

		void RemoveUDPRule(uint16_t port, const std::string& ruleName)
		{
			if (!IsElevated())
			{
				return;
			}

			char cmd[256]{};
			snprintf(cmd, sizeof(cmd), "netsh advfirewall firewall delete rule name=\"%s\" protocol=udp localport=%u",
					 ruleName.c_str(), static_cast<unsigned>(port));

			std::string output = RunCommand(cmd);

			if (output.find("Ok.") != std::string::npos || output.find("No rules") != std::string::npos)
			{
				CH_CORE_INFO("Firewall: Removed rule '{}' for UDP {}.", ruleName, port);
			}
			else
			{
				CH_CORE_WARN("Firewall: Failed to remove rule '{}'. Output: {}", ruleName, output);
			}
		}

		bool RuleExists(uint16_t port, const std::string& ruleName)
		{
			char cmd[256]{};
			snprintf(cmd, sizeof(cmd), "netsh advfirewall firewall show rule name=\"%s\" protocol=udp localport=%u",
					 ruleName.c_str(), static_cast<unsigned>(port));

			std::string output = RunCommand(cmd);
			return output.find(ruleName) != std::string::npos;
		}

#else

		/// Checks whether the process is running as root (UID 0).
		static bool IsElevated()
		{
			return getuid() == 0;
		}

		bool AddUDPRule(uint16_t port, const std::string& ruleName)
		{
			if (!IsElevated())
			{
				CH_CORE_WARN("Firewall: Not running as root — cannot add iptables rule. "
							 "Run with sudo to enable automatic port forwarding.");
				return false;
			}

			if (RuleExists(port, ruleName))
			{
				CH_CORE_INFO("Firewall: iptables rule '{}' for UDP {} already exists.", ruleName, port);
				return true;
			}

			// Check for nftables first, fall back to iptables
			char cmd[512]{};
			int ret = system("command -v nft >/dev/null 2>&1");
			if (ret == 0)
			{
				snprintf(cmd, sizeof(cmd), "nft add rule inet filter input udp dport %u accept comment '%s'",
						 static_cast<unsigned>(port), ruleName.c_str());
			}
			else
			{
				snprintf(cmd, sizeof(cmd), "iptables -A INPUT -p udp --dport %u -j ACCEPT -m comment --comment '%s'",
						 static_cast<unsigned>(port), ruleName.c_str());
			}

			std::string output = RunCommand(cmd);

			// iptables/nft silently succeed (no output) on success
			if (output.empty() || output.find("error") == std::string::npos)
			{
				CH_CORE_INFO("Firewall: Added inbound UDP rule '{}' for port {}.", ruleName, port);
				return true;
			}

			CH_CORE_WARN("Firewall: Failed to add rule '{}'. Output: {}", ruleName, output);
			return false;
		}

		void RemoveUDPRule(uint16_t port, const std::string& ruleName)
		{
			if (!IsElevated())
			{
				return;
			}

			char cmd[512]{};
			int ret = system("command -v nft >/dev/null 2>&1");
			if (ret == 0)
			{
				snprintf(cmd, sizeof(cmd),
						 "nft delete rule inet filter input handle $(nft -a list chain inet filter input | "
						 "grep 'comment \"%s\"' | awk '{print $NF}') 2>/dev/null; true",
						 ruleName.c_str());
			}
			else
			{
				snprintf(cmd, sizeof(cmd),
						 "iptables -D INPUT -p udp --dport %u -j ACCEPT -m comment --comment '%s' 2>/dev/null; true",
						 static_cast<unsigned>(port), ruleName.c_str());
			}

			RunCommand(cmd);
			CH_CORE_INFO("Firewall: Removed iptables rule '{}' for UDP {}.", ruleName, port);
		}

		bool RuleExists(uint16_t port, const std::string& ruleName)
		{
			char cmd[256]{};
			int ret = system("command -v nft >/dev/null 2>&1");
			if (ret == 0)
			{
				snprintf(cmd, sizeof(cmd), "nft list chain inet filter input 2>/dev/null | grep 'comment \"%s\"'",
						 ruleName.c_str());
			}
			else
			{
				snprintf(cmd, sizeof(cmd), "iptables -L INPUT -n 2>/dev/null | grep 'udp.*dpt:%u' | grep '%s'",
						 static_cast<unsigned>(port), ruleName.c_str());
			}

			std::string output = RunCommand(cmd);
			return !output.empty();
		}

#endif

	} // namespace Firewall
} // namespace Chained
