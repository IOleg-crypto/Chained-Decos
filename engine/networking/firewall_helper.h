#ifndef CH_FIREWALL_HELPER_H
#define CH_FIREWALL_HELPER_H

#include <cstdint>
#include <string>

namespace Chained
{
	namespace Firewall
	{
		/// Adds an inbound UDP rule to Windows Firewall for the given port.
		/// Returns true on success or if the rule already exists.
		/// Requires administrator privileges; returns false if elevation fails.
		bool AddUDPRule(uint16_t port, const std::string& ruleName = "ChainedDecos Game");

		/// Removes the inbound UDP rule for the given port.
		void RemoveUDPRule(uint16_t port, const std::string& ruleName = "ChainedDecos Game");

		/// Checks whether the rule already exists.
		bool RuleExists(uint16_t port, const std::string& ruleName = "ChainedDecos Game");
	} // namespace Firewall
} // namespace Chained

#endif // CH_FIREWALL_HELPER_H
