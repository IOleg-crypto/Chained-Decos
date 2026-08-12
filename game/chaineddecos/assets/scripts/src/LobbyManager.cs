using System;
using System.Collections.Generic;
using Chained;

namespace ChainedDecos.Scripts
{
    /// <summary>
    /// Manages the 3D lobby: positions player avatars in slots, handles chat, and transitions to game.
    /// Uses pre-placed avatar entities in the scene (avatar_0 .. avatar_3).
    /// </summary>
    public class LobbyManager : Script
    {
        private const int MaxAvatars = 4;
        private const string AvatarTagPrefix = "avatar_";

        public string PlayerPrefabPath = "prefab/player.chprefab";
        public string GameScene = "scenes/rpg_strategy_scene_mp.chscene";

        // Shared state from previous scenes
        public static ushort SelectedPort = 7777;
        public static int SelectedSkinIndex = 0;
        public static string SelectedMap = "scenes/rpg_strategy_scene_mp.chscene";

        private float m_RefreshTimer = 0.0f;

        // Maps networkId -> slot index (0..3)
        private Dictionary<ulong, int> m_SlotByNetId = new Dictionary<ulong, int>();
        // Maps slot index -> networkId (0 = empty)
        private ulong[] m_SlotNetIds = new ulong[MaxAvatars];

        public override void OnCreate()
        {
            Log.Info("LobbyManager: Initialized");

            Network.SetPlayerPrefab(PlayerPrefabPath);

            // Hide all avatar slots initially
            for (int i = 0; i < MaxAvatars; i++)
            {
                SetSlotHidden(i);
            }

            if (Network.IsHost)
            {
                string hostName = PlayerSettings.Nickname;
                Network.SetLocalPlayerInfo(hostName, (byte)SelectedSkinIndex);
                Log.Info($"LobbyManager: Host ready (name='{hostName}', skin={SelectedSkinIndex})");
            }
        }

        public override void OnUpdate(float deltaTime)
        {
            m_RefreshTimer += deltaTime;

            if (m_RefreshTimer >= 0.5f)
            {
                m_RefreshTimer = 0.0f;
                RefreshAvatars();
            }

            // Check for scene change from host (client side)
            if (Network.IsClient && Network.HasPendingSceneChange)
            {
                string path = Network.GetPendingSceneChange();
                Network.ClearPendingSceneChange();
                Log.Info($"LobbyManager: Received scene change -> {path}");
                Scene.LoadScene(path);
                return;
            }
        }

        private void RefreshAvatars()
        {
            if (!Network.IsHost && !Network.IsClient)
                return;

            string json = Network.GetPlayerListJSON();
            if (string.IsNullOrEmpty(json) || json == "[]")
                return;

            // Parse player list JSON
            var players = ParsePlayerList(json);

            // Build set of current networkIds
            var currentIds = new HashSet<ulong>();
            foreach (var p in players)
                currentIds.Add(p.NetworkID);

            // Release slots for players that left
            for (int i = 0; i < MaxAvatars; i++)
            {
                if (m_SlotNetIds[i] != 0 && !currentIds.Contains(m_SlotNetIds[i]))
                {
                    ulong leftId = m_SlotNetIds[i];
                    m_SlotByNetId.Remove(leftId);
                    m_SlotNetIds[i] = 0;
                    SetSlotHidden(i);
                    Log.Info($"LobbyManager: Freed slot {i} (netID={leftId} left)");
                }
            }

            // Assign slots for new players
            foreach (var p in players)
            {
                if (m_SlotByNetId.ContainsKey(p.NetworkID))
                {
                    // Already has a slot — just update color
                    int slot = m_SlotByNetId[p.NetworkID];
                    SetSlotColor(slot, p.SkinIndex);
                    continue;
                }

                // Find free slot
                int freeSlot = -1;
                for (int i = 0; i < MaxAvatars; i++)
                {
                    if (m_SlotNetIds[i] == 0)
                    {
                        freeSlot = i;
                        break;
                    }
                }

                if (freeSlot < 0)
                {
                    Log.Warn("LobbyManager: No free avatar slots!");
                    continue;
                }

                m_SlotNetIds[freeSlot] = p.NetworkID;
                m_SlotByNetId[p.NetworkID] = freeSlot;
                SetSlotPosition(freeSlot, players.IndexOf(p));
                SetSlotColor(freeSlot, p.SkinIndex);
                Log.Info($"LobbyManager: Assigned netID={p.NetworkID} to slot {freeSlot} (skin={p.SkinIndex})");
            }
        }

        private void SetSlotPosition(int slot, int index)
        {
            Entity? avatar = Scene.FindEntityByTag(AvatarTagPrefix + slot);
            if (avatar == null || !avatar.IsValid)
                return;

            TransformComponent? t = avatar.GetComponent<TransformComponent>();
            if (t != null)
            {
                t.Translation = new Vector3(index * 2.0f, 0.5f, 0.0f);
            }
        }

        private void SetSlotColor(int slot, int skinIndex)
        {
            Entity? avatar = Scene.FindEntityByTag(AvatarTagPrefix + slot);
            if (avatar == null || !avatar.IsValid)
                return;

            PrimitiveComponent? prim = avatar.GetComponent<PrimitiveComponent>();
            if (prim != null)
            {
                prim.AlbedoColor = SkinDatabase.GetColor(skinIndex);
            }
        }

        private void SetSlotHidden(int slot)
        {
            Entity? avatar = Scene.FindEntityByTag(AvatarTagPrefix + slot);
            if (avatar == null || !avatar.IsValid)
                return;

            TransformComponent? t = avatar.GetComponent<TransformComponent>();
            if (t != null)
            {
                t.Translation = new Vector3(0, -100, 0);
            }
        }

        private struct PlayerEntry
        {
            public ulong NetworkID;
            public string Name;
            public int SkinIndex;
        }

        private List<PlayerEntry> ParsePlayerList(string json)
        {
            var result = new List<PlayerEntry>();
            int startIndex = 0;

            while (startIndex < json.Length)
            {
                int idStart = json.IndexOf("\"id\":", startIndex);
                if (idStart < 0) break;
                idStart += 5;
                while (idStart < json.Length && (json[idStart] == ' ' || json[idStart] == ':')) idStart++;

                int idEnd = json.IndexOf(',', idStart);
                if (idEnd < 0) idEnd = json.IndexOf('}', idStart);
                if (idEnd < 0) break;

                ulong networkId = 0;
                ulong.TryParse(json.Substring(idStart, idEnd - idStart), out networkId);
                if (networkId == 0) { startIndex = idEnd; continue; }

                int nameStart = json.IndexOf("\"name\":\"", idEnd);
                string name = "Player";
                if (nameStart >= 0)
                {
                    nameStart += 8;
                    int nameEnd = json.IndexOf('"', nameStart);
                    if (nameEnd > nameStart)
                        name = json.Substring(nameStart, nameEnd - nameStart);
                }

                int skinStart = json.IndexOf("\"skin\":", idEnd);
                int skinValue = 0;
                if (skinStart > 0 && skinStart < json.IndexOf('}', idEnd))
                {
                    skinStart += 7;
                    while (skinStart < json.Length && (json[skinStart] == ' ' || json[skinStart] == ':')) skinStart++;
                    int skinEnd = json.IndexOf(',', skinStart);
                    if (skinEnd < 0) skinEnd = json.IndexOf('}', skinStart);
                    if (skinEnd > skinStart)
                        int.TryParse(json.Substring(skinStart, skinEnd - skinStart), out skinValue);
                }

                result.Add(new PlayerEntry { NetworkID = networkId, Name = name, SkinIndex = skinValue });
                startIndex = idEnd + 1;
            }

            return result;
        }

    }
}
