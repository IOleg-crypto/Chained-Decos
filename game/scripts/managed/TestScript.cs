using System;

namespace ChainedEngine
{
    public class TestScript : Entity
    {
        public TestScript(uint id) : base(id)
        {
        }

        public override void OnCreate()
        {
            Log.Info($"TestScript.OnCreate for entity {ID}");
            Transform.Position = new Vector3(0, 10, 0);
        }

        private float _timer = 0;

        public override void OnUpdate(float deltaTime)
        {
            _timer += deltaTime;
            if (_timer >= 1.0f)
            {
                _timer = 0;
                Vector3 pos = Transform.Position;
                Log.Info($"TestScript.OnUpdate - Entity {ID} Position: {pos.X}, {pos.Y}, {pos.Z}");
            }
        }
    }
}
