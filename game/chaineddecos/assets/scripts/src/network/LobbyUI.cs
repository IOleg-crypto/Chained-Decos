using System;
using System.Collections.Generic;
using Chained;

namespace ChainedDecos.Scripts
{
    /// <summary>
    /// Lobby UI: manages the native chat widget, skin selector and start/disconnect buttons.
    /// Required scene entities:
    ///   chat_messages  - LabelControl  (scrolling message history)
    ///   chat_input     - InputTextControl (text entry field)
    ///   btn_send_chat  - ButtonControl (Send button)
    ///   btn_skin_N     - ButtonControl (skin colour pickers, N = 0..7)
    /// Optional ICE entities:
    ///   label_ice_token  - LabelControl  (shows truncated ICE SDP token)
    ///   btn_copy_token   - ButtonControl (copies full ICE SDP token to clipboard)
    /// </summary>
    public class LobbyUI : Script
    {
        private const int MaxChatLines = 30;

        private readonly List<string> m_ChatLines = new List<string>();
        private string m_LastDisplayed = "";
        private string m_LastDisplayedInfo = "";
        private int m_DisconnectGraceFrames = 0;
        private const int DisconnectGraceLimit = 15;
        private bool m_WasEnterDown = false;
        private string m_CachedIceToken = "";
        private string m_LastIceTokenLabel = "";

        public override void OnCreate()
        {
            Log.Info("LobbyUI: Initialized");
            m_DisconnectGraceFrames = 0;
        }

        public override void OnUpdate(float deltaTime)
        {
            // 0. Detect host disconnect (client side)
            if (Network.IsClient)
            {
                if (!Network.IsConnected)
                {
                    m_DisconnectGraceFrames++;
                    if (m_DisconnectGraceFrames > DisconnectGraceLimit)
                    {
                        Log.Info("[LobbyUI] Host disconnected — returning to menu.");
                        Scene.LoadScene("scenes/start_menu.chscene");
                        return;
                    }
                }
                else
                {
                    m_DisconnectGraceFrames = 0;
                }
            }

            // 1. Scene change from host (client side)
            if (Network.IsClient && Network.HasPendingSceneChange)
            {
                string path = Network.GetPendingSceneChange();
                Network.ClearPendingSceneChange();
                Log.Info($"LobbyUI: Scene change -> {path}");
                Scene.LoadScene(path);
                return;
            }

            // 2. Incoming network chat
            if (Network.HasPendingChat)
            {
                string json = Network.GetPendingChatJSON();
                Network.ClearPendingChat();
                ParseAndAppendChat(json);
                UpdateChatLabel();
            }

            // 2b. Server Info Label (IP & Port)
            UpdateServerInfo();

            // 2c. ICE Token display (host only)
            UpdateIceTokenDisplay();

            // 3. Skin buttons - highlight selected skin
            for (int i = 0; i < SkinDatabase.SkinCount; i++)
            {
                Entity? btn = Scene.FindEntityByTag($"btn_skin_{i}");
                if (btn == null || !btn.IsValid) continue;
                ButtonControl? bc = btn.GetComponent<ButtonControl>();
                if (bc == null) continue;
                bool selected = (LobbyManager.SelectedSkinIndex == i);
                bc.Label = selected ? $">> {SkinDatabase.GetName(i)} <<" : SkinDatabase.GetName(i);
            }

            // 4. Chat input + Send button
            Entity? sendEntity  = Scene.FindEntityByTag("btn_send_chat");
            Entity? inputEntity = Scene.FindEntityByTag("chat_input");

            if (sendEntity  != null && sendEntity.IsValid &&
                inputEntity != null && inputEntity.IsValid)
            {
                ButtonControl?    send  = sendEntity.GetComponent<ButtonControl>();
                InputTextControl? input = inputEntity.GetComponent<InputTextControl>();

                if (send != null && input != null)
                {
                    bool enterDown   = Input.IsKeyDown(Key.Enter);
                    bool doSend      = send.IsClicked || (enterDown && !m_WasEnterDown);
                    m_WasEnterDown   = enterDown;

                    string msg = (input.Text ?? "").Trim();
                    if (doSend && msg.Length > 0)
                    {
                        Network.SendChatMessage(msg);
                        input.Text = "";
                    }
                }
            }
            else
            {
                m_WasEnterDown = false;
            }
        }

        private void ParseAndAppendChat(string json)
        {
            if (string.IsNullOrEmpty(json) || json == "[]") return;

            int pos = 0;
            while (pos < json.Length)
            {
                int sStart = json.IndexOf("\"sender\":\"", pos);
                if (sStart < 0) break;
                sStart += 10;
                int sEnd = json.IndexOf('"', sStart);
                if (sEnd < 0) break;
                string sender = json.Substring(sStart, sEnd - sStart);

                int mStart = json.IndexOf("\"message\":\"", sEnd);
                if (mStart < 0) break;
                mStart += 11;
                int mEnd = json.IndexOf('"', mStart);
                if (mEnd < 0) break;
                string message = json.Substring(mStart, mEnd - mStart);

                AppendLine($"[{sender}]: {message}");
                pos = mEnd + 1;
            }
        }

        private void AppendLine(string line)
        {
            m_ChatLines.Add(line);
            while (m_ChatLines.Count > MaxChatLines)
                m_ChatLines.RemoveAt(0);
        }

        private void UpdateChatLabel()
        {
            string text = string.Join("\n", m_ChatLines);
            if (text == m_LastDisplayed) return;
            m_LastDisplayed = text;

            Entity? labelEntity = Scene.FindEntityByTag("chat_messages");
            if (labelEntity == null || !labelEntity.IsValid) return;
            LabelControl? lc = labelEntity.GetComponent<LabelControl>();
            if (lc != null) lc.Text = text;
        }

        private void UpdateServerInfo()
        {
            int count = Network.PlayerCount;
            if (count <= 0) count = 1;
            int max = LobbyManager.MaxClients > 0 ? LobbyManager.MaxClients : 4;
            string players = $"Players: {count}/{max}";

            Entity? playersLabelEntity = Scene.FindEntityByTag("label_players_count");
            if (playersLabelEntity != null && playersLabelEntity.IsValid)
            {
                LabelControl? plc = playersLabelEntity.GetComponent<LabelControl>();
                if (plc != null)
                {
                    plc.Text = $"PLAYERS: {count} / {max}";
                }
            }

            Entity? labelEntity = Scene.FindEntityByTag("label_server_info");
            if (labelEntity == null || !labelEntity.IsValid) return;
            LabelControl? lc = labelEntity.GetComponent<LabelControl>();
            if (lc == null) return;

            string text;
            if (Network.IsHost)
            {
                string pub = Network.GetPublicAddress();
                bool fetching = pub == "Fetching..." || string.IsNullOrEmpty(pub);
                string local = Network.GetListenAddress();
                if (string.IsNullOrEmpty(local)) local = $"127.0.0.1:{LobbyManager.SelectedPort}";
                string upnp = Network.IsUpnpAvailable ? "UPnP: OK" : "UPnP: Off";
                string ice = Network.IsIceActive ? " | ICE: Active" : "";
                string nat = Network.HasStunResult ? " | STUN: Ready" : (fetching ? "" : " | STUN: Direct");
                text = $"LAN: {local} | WAN: {pub} | {players} | {upnp}{nat}{ice}";
                if (!fetching && text == m_LastDisplayedInfo) return;
            }
            else if (Network.IsClient)
            {
                text = $"[CLIENT] Connected | {players} | Port: {LobbyManager.SelectedPort}";
            }
            else
            {
                text = "[OFFLINE]";
            }

            if (text != m_LastDisplayedInfo)
            {
                m_LastDisplayedInfo = text;
                lc.Text = text;
            }
        }

        private void UpdateIceTokenDisplay()
        {
            if (!Network.IsHost || !Network.IsIceActive) return;

            Entity? tokenEntity = Scene.FindEntityByTag("label_ice_token");
            Entity? copyBtnEntity = Scene.FindEntityByTag("btn_copy_token");

            // Cache the token
            string token = Network.GetIceSessionToken();
            if (!string.IsNullOrEmpty(token))
            {
                m_CachedIceToken = token;
            }

            // Update label (show truncated token)
            if (tokenEntity != null && tokenEntity.IsValid)
            {
                LabelControl? lc = tokenEntity.GetComponent<LabelControl>();
                if (lc != null)
                {
                    string display = m_CachedIceToken.Length > 80
                        ? m_CachedIceToken.Substring(0, 80) + "..."
                        : m_CachedIceToken;
                    string label = string.IsNullOrEmpty(m_CachedIceToken)
                        ? "ICE Token: Gathering..."
                        : $"ICE Token: {display}";
                    if (label != m_LastIceTokenLabel)
                    {
                        m_LastIceTokenLabel = label;
                        lc.Text = label;
                    }
                }
            }

            // Handle copy button
            if (copyBtnEntity != null && copyBtnEntity.IsValid)
            {
                ButtonControl? bc = copyBtnEntity.GetComponent<ButtonControl>();
                if (bc != null)
                {
                    if (bc.IsClicked && !string.IsNullOrEmpty(m_CachedIceToken))
                    {
                        Clipboard.SetText(m_CachedIceToken);
                        bc.Label = "Copied!";
                        Log.Info("[LobbyUI] ICE session token copied to clipboard.");
                    }
                }
            }
        }
    }
}
