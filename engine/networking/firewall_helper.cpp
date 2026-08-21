#include "firewall_helper.h"
#include "engine/core/log.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#pragma comment(lib, "advapi32.lib")
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

#ifdef _WIN32

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
			CH_CORE_INFO("Firewall: IsElevated = {}", result);
			return result;
		}

		/// Runs a command via CreateProcess and captures stdout+stderr.
		static std::string RunCommand(const char* cmd)
		{
			std::string output;

			SECURITY_ATTRIBUTES sa{};
			sa.nLength = sizeof(sa);
			sa.bInheritHandle = TRUE;
			sa.lpSecurityDescriptor = nullptr;

			HANDLE hRead = nullptr, hWrite = nullptr;
			if (!CreatePipe(&hRead, &hWrite, &sa, 0))
			{
				return {};
			}
			SetHandleInformation(hRead, HANDLE_FLAG_INHERIT, 0);

			STARTUPINFOA si{};
			si.cb = sizeof(si);
			si.hStdOutput = hWrite;
			si.hStdError = hWrite;
			si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
			si.wShowWindow = SW_HIDE;

			char cmdBuf[1024];
			snprintf(cmdBuf, sizeof(cmdBuf), "cmd.exe /C %s", cmd);

			PROCESS_INFORMATION pi{};
			BOOL ok = CreateProcessA(nullptr, cmdBuf, nullptr, nullptr, TRUE, 0, nullptr, nullptr, &si, &pi);

			CloseHandle(hWrite);
			hWrite = nullptr;

			if (!ok)
			{
				CloseHandle(hRead);
				CH_CORE_WARN("Firewall: CreateProcess failed for: {}", cmd);
				return {};
			}

			WaitForSingleObject(pi.hProcess, 10000);

			char buf[512]{};
			DWORD bytesRead = 0;
			while (ReadFile(hRead, buf, sizeof(buf) - 1, &bytesRead, nullptr) && bytesRead > 0)
			{
				buf[bytesRead] = '\0';
				output += buf;
			}

			CloseHandle(hRead);
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);

			return output;
		}

		bool AddUDPRule(uint16_t port, const std::string& ruleName)
		{
			if (!IsElevated())
			{
				CH_CORE_WARN("Firewall: Not running as administrator — cannot add firewall rule. "
							 "Run as admin to allow inbound connections.");
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

			// netsh outputs the rule name on success, or "Ok." on some Windows versions
			if (output.find("Ok.") != std::string::npos || output.find(ruleName) != std::string::npos)
			{
				CH_CORE_INFO("Firewall: Added inbound UDP rule '{}' for port {}. Output: {}", ruleName, port, output);
				return true;
			}

			CH_CORE_WARN("Firewall: Failed to add rule '{}'. netsh output: [{}]", ruleName, output);
			return false;
		}

		void RemoveUDPRule(uint16_t port, const std::string& ruleName)
		{
			if (!IsElevated())
			{
				return;
			}

			char cmd[512]{};
			snprintf(cmd, sizeof(cmd), "netsh advfirewall firewall delete rule name=\"%s\" protocol=udp localport=%u",
					 ruleName.c_str(), static_cast<unsigned>(port));

			std::string output = RunCommand(cmd);

			if (output.find("No rules") != std::string::npos || output.find("Нет правил") != std::string::npos)
			{
				CH_CORE_INFO("Firewall: No rule to remove for UDP {}.", port);
			}
			else
			{
				CH_CORE_INFO("Firewall: Remove result for UDP {}: {}", port, output);
			}
		}

		bool RuleExists(uint16_t port, const std::string& ruleName)
		{
			char cmd[512]{};
			snprintf(cmd, sizeof(cmd),
					 "netsh advfirewall firewall show rule name=\"%s\" dir=in protocol=udp localport=%u",
					 ruleName.c_str(), static_cast<unsigned>(port));

			std::string output = RunCommand(cmd);
			bool found = output.find(ruleName) != std::string::npos;
			CH_CORE_INFO("Firewall: RuleExists('{}', {}) = {}", ruleName, port, found);
			return found;
		}

#else

		static bool IsElevated()
		{
			return getuid() == 0;
		}

		static std::string RunCommand(const char* cmd)
		{
			std::string result;
			std::array<char, 256> buf{};
			std::string fullCmd = std::string(cmd) + " 2>&1";
			std::shared_ptr<FILE> pipe(popen(fullCmd.c_str(), "r"), pclose);
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
			char cmd[512]{};
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
