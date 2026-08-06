#include <gtest/gtest.h>

#include "engine/networking/net_packet.h"
#include "engine/networking/network_service.h"

#include <chrono>
#include <cstring>
#include <thread>

using namespace Chained;

namespace
{
	constexpr uint16_t kTestPort = 27599;

	// Host and client share one process, which exercises Network's refcounting of the
	// process-wide GNS library.
	class NetworkLoopbackTest : public ::testing::Test
	{
	protected:
		void SetUp() override
		{
			m_Host.Initialize();
			m_Client.Initialize();
			ASSERT_TRUE(m_Host.IsEnabled()) << "GameNetworkingSockets failed to initialize";
			ASSERT_TRUE(m_Client.IsEnabled()) << "GameNetworkingSockets failed to initialize";
		}

		void TearDown() override
		{
			m_Client.Shutdown();
			m_Host.Shutdown();
		}

		// GNS handshakes asynchronously, so pump both peers until the predicate holds.
		template <typename Predicate> bool PumpUntil(Predicate pred, std::chrono::milliseconds timeout)
		{
			const auto deadline = std::chrono::steady_clock::now() + timeout;
			while (std::chrono::steady_clock::now() < deadline)
			{
				m_Host.Update(0.0f);
				m_Client.Update(0.0f);
				if (pred())
				{
					return true;
				}
				std::this_thread::sleep_for(std::chrono::milliseconds(5));
			}
			return false;
		}

		Network m_Host;
		Network m_Client;
	};
} // namespace

TEST_F(NetworkLoopbackTest, InitializeSucceeds)
{
	EXPECT_EQ(m_Host.GetRole(), Role::Offline);
	EXPECT_FALSE(m_Host.IsConnected());
}

TEST_F(NetworkLoopbackTest, HostGameOpensListenSocket)
{
	m_Host.HostGame(kTestPort, 4);

	EXPECT_TRUE(m_Host.IsEnabled()) << "HostGame disabled the service — listen socket failed";
	EXPECT_EQ(m_Host.GetRole(), Role::Host);
	EXPECT_EQ(m_Host.GetPort(), kTestPort);
}

TEST_F(NetworkLoopbackTest, ClientConnectsToHost)
{
	m_Host.HostGame(kTestPort, 4);
	ASSERT_TRUE(m_Host.IsEnabled());

	m_Client.ConnectTo("127.0.0.1", kTestPort);
	ASSERT_TRUE(m_Client.IsEnabled());
	EXPECT_EQ(m_Client.GetRole(), Role::Client);

	const bool connected = PumpUntil([this] { return m_Host.GetClientCount() == 1; }, std::chrono::seconds(5));
	EXPECT_TRUE(connected) << "Host never accepted the client connection";
}

TEST_F(NetworkLoopbackTest, ClientToHostPacketRoundTrip)
{
	m_Host.HostGame(kTestPort, 4);
	ASSERT_TRUE(m_Host.IsEnabled());

	m_Client.ConnectTo("127.0.0.1", kTestPort);
	ASSERT_TRUE(m_Client.IsEnabled());
	ASSERT_TRUE(PumpUntil([this] { return m_Host.GetClientCount() == 1; }, std::chrono::seconds(5)));

	InputStatePacket received{};
	bool gotPacket = false;
	m_Host.SetPacketCallback([&](PacketType type, const uint8_t* data, size_t size, HSteamNetConnection) {
		if (type == PacketType::InputState && size == InputStatePacket::WireSize())
		{
			received = InputStatePacket::Deserialize(data);
			gotPacket = true;
		}
	});

	InputStatePacket sent{};
	sent.Tick = 4242;
	sent.MoveX = 1.5f;
	sent.MoveZ = -2.5f;
	sent.ActionFlags = InputAction_Jump | InputAction_Sprint;

	uint8_t buffer[InputStatePacket::WireSize()];
	sent.Serialize(buffer);
	m_Client.SendToServer(PacketType::InputState, buffer, sizeof(buffer), true);

	ASSERT_TRUE(PumpUntil([&] { return gotPacket; }, std::chrono::seconds(5)))
		<< "Host never received the client's input packet";

	EXPECT_EQ(received.Tick, sent.Tick);
	EXPECT_FLOAT_EQ(received.MoveX, sent.MoveX);
	EXPECT_FLOAT_EQ(received.MoveZ, sent.MoveZ);
	EXPECT_EQ(received.ActionFlags, sent.ActionFlags);

	m_Host.ClearPacketCallback();
}

TEST_F(NetworkLoopbackTest, HostBroadcastReachesClient)
{
	m_Host.HostGame(kTestPort, 4);
	ASSERT_TRUE(m_Host.IsEnabled());

	m_Client.ConnectTo("127.0.0.1", kTestPort);
	ASSERT_TRUE(m_Client.IsEnabled());
	ASSERT_TRUE(PumpUntil([this] { return m_Host.GetClientCount() == 1; }, std::chrono::seconds(5)));

	std::string receivedPath;
	m_Client.SetPacketCallback([&](PacketType type, const uint8_t* data, size_t size, HSteamNetConnection) {
		if (type == PacketType::SceneChange && size == SceneChangePacket::WireSize())
		{
			receivedPath = SceneChangePacket::Deserialize(data).ScenePath;
		}
	});

	m_Host.BroadcastSceneChange("assets/scenes/level_01.chscene");

	ASSERT_TRUE(PumpUntil([&] { return !receivedPath.empty(); }, std::chrono::seconds(5)))
		<< "Client never received the broadcast scene change";
	EXPECT_EQ(receivedPath, "assets/scenes/level_01.chscene");

	m_Client.ClearPacketCallback();
}

TEST_F(NetworkLoopbackTest, HostAppearsInItsOwnPlayerList)
{
	m_Host.SetLocalPlayerInfo("Alice", 3);
	m_Host.HostGame(kTestPort, 4);
	ASSERT_TRUE(m_Host.IsEnabled());

	const auto& players = m_Host.GetPlayerList();
	ASSERT_EQ(players.size(), 1u) << "Host is missing from its own lobby";
	EXPECT_EQ(players[0].NetworkID, 1u);
	EXPECT_EQ(players[0].IsHost, 1);
	EXPECT_STREQ(players[0].Name, "Alice");
	EXPECT_EQ(players[0].SkinIndex, 3);
	EXPECT_EQ(m_Host.GetLocalNetworkID(), 1u);
}

TEST_F(NetworkLoopbackTest, PeerGetsIdentityDistinctFromHost)
{
	m_Host.HostGame(kTestPort, 4);
	ASSERT_TRUE(m_Host.IsEnabled());

	m_Client.ConnectTo("127.0.0.1", kTestPort);
	ASSERT_TRUE(m_Client.IsEnabled());
	ASSERT_TRUE(PumpUntil([this] { return m_Host.GetClientCount() == 1; }, std::chrono::seconds(5)));

	const HSteamNetConnection peer = m_Host.GetClients().front();
	const uint64_t peerID = m_Host.GetNetworkIDForConnection(peer);
	EXPECT_NE(peerID, 0u) << "Peer identity was not bound at accept time";
	EXPECT_NE(peerID, 1u) << "NetworkID 1 is reserved for the host";

	// The host must still be in the list, and only it may be flagged as host.
	const auto& players = m_Host.GetPlayerList();
	ASSERT_EQ(players.size(), 2u);
	int hostFlags = 0;
	for (const auto& p : players)
	{
		hostFlags += p.IsHost ? 1 : 0;
	}
	EXPECT_EQ(hostFlags, 1);

	EXPECT_EQ(m_Host.GetNetworkIDForConnection(k_HSteamNetConnection_Invalid), 0u);
}

TEST(NetworkPacketTest, EntitySpawnRoundTrip)
{
	EntitySpawnPacket sent;
	sent.NetworkID = 0x00000000DEADBEEFull;
	std::strncpy(sent.PrefabPath, "prefab/player.chprefab", sizeof(sent.PrefabPath) - 1);

	uint8_t buffer[EntitySpawnPacket::WireSize()];
	sent.Serialize(buffer);
	const EntitySpawnPacket received = EntitySpawnPacket::Deserialize(buffer);

	EXPECT_EQ(received.NetworkID, sent.NetworkID);
	EXPECT_STREQ(received.PrefabPath, "prefab/player.chprefab");
}

TEST(NetworkPacketTest, EntityDestroyAndPlayerAssignRoundTrip)
{
	EntityDestroyPacket destroy;
	destroy.NetworkID = 77;
	uint8_t destroyBuf[EntityDestroyPacket::WireSize()];
	destroy.Serialize(destroyBuf);
	EXPECT_EQ(EntityDestroyPacket::Deserialize(destroyBuf).NetworkID, 77u);

	PlayerAssignPacket assign;
	assign.NetworkID = 5;
	uint8_t assignBuf[PlayerAssignPacket::WireSize()];
	assign.Serialize(assignBuf);
	EXPECT_EQ(PlayerAssignPacket::Deserialize(assignBuf).NetworkID, 5u);
}
