using Chained;
using System;
using System.Text;

namespace ChainedDecos.Scripts
{
    public class NetworkingDemo : Script
    {
        public int Port = 27015;
        public string ServerAddress = "127.0.0.1";

        public override void OnCreate()
        {
            Log.Info("NetworkingDemo: Press H to Host, C to Connect, S to Send Message");
        }

        public override void OnUpdate(float ts)
        {
            if (Input.IsKeyPressed(Key.H))
            {
                if (Network.Host((ushort)Port))
                    Log.Info($"Hosting on port {Port}");
                else
                    Log.Error("Failed to host!");
            }

            if (Input.IsKeyPressed(Key.C))
            {
                if (Network.Connect(ServerAddress))
                    Log.Info($"Connecting to {ServerAddress}...");
                else
                    Log.Error("Failed to connect!");
            }

            if (Input.IsKeyPressed(Key.S))
            {
                string msg = "Hello from " + (Network.IsServer() ? "Server" : "Client");
                byte[] data = Encoding.UTF8.GetBytes(msg);
                if (Network.SendData(data))
                    Log.Info("Sent: " + msg);
            }

            // Simple message receiver
            if (Network.HasMessages())
            {
                byte[] data = Network.GetNextMessage();
                if (data != null && data.Length > 0)
                {
                    string received = Encoding.UTF8.GetString(data);
                    Log.Info($"Received: {received}");
                }
            }
        }
    }
}
