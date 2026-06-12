using System;
using Chained;

namespace ChainedDecos.Scripts
{
    public class ServerManager : Script
    {
        public ushort Port = 27015;
        public bool AutoHost = false;

        public override void OnCreate()
        {
            Log.Info("[ServerManager] Initialized.");
            if (AutoHost)
            {
                HostServer();
            }
        }

        public void HostServer()
        {
            Log.Info($"[ServerManager] Attempting to host server on port {Port}...");
            if (Network.Host(Port))
            {
                Log.Info("[ServerManager] Server successfully started and listening!");
            }
            else
            {
                Log.Error("[ServerManager] Failed to start server. Check engine logs for details.");
            }
        }

        public override void OnUpdate(float deltaTime)
        {
            // Network polling is handled by the engine's NetworkService::OnUpdate
            
            // Example: Press H to host manually
            if (Input.IsKeyPressed(Key.H))
            {
                HostServer();
            }

            // Example: Press C to connect to local
            if (Input.IsKeyPressed(Key.C))
            {
                Log.Info("[ServerManager] Attempting to connect to 127.0.0.1:27015...");
                if (Network.Connect("127.0.0.1:27015"))
                {
                    Log.Info("[ServerManager] Connection attempt started...");
                }
            }
        }
    }
}
