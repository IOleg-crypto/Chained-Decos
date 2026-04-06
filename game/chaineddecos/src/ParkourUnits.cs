using System;
using CHEngine;

namespace ChainedDecos.Scripts
{
    /// <summary>
    /// Prototype pattern for parkour entities.
    /// Used for mass cloning of similar objects (e.g. traps, collectibles).
    /// </summary>
    public abstract class ParkourUnit
    {
        public string Name { get; set; } = string.Empty;
        public float Health { get; set; } = 100.0f;
        public float Weight { get; set; } = 1.0f;

        public abstract ParkourUnit Clone();
    }

    /// <summary>
    /// A simple hazard unit (e.g., a rotating fan or a sliding block).
    /// </summary>
    public class HazardUnit : ParkourUnit
    {
        public HazardUnit(string name, float health, float weight)
        {
            Name = name;
            Health = health;
            Weight = weight;
        }

        public override ParkourUnit Clone()
        {
            Log.Info($"Cloning Hazard: {Name}");
            // Shallow clone for simple data structures, but could be deep if we had nested components.
            return (HazardUnit)this.MemberwiseClone();
        }

        public override string ToString() => $"Hazard: {Name}, HP: {Health}, Weight: {Weight}";
    }

    /// <summary>
    /// A script to manage hazards using Prototype.
    /// One prototype is configured and the others are spawned via cloning.
    /// </summary>
    public class HazardManager : Script
    {
        private HazardUnit? prototype;

        public override void OnCreate()
        {
            // Initialize prototype
            prototype = new HazardUnit("DeathTrap_Prototype", 500, 10.0f);
            Log.Info("Prototype 'HazardUnit' created.");
        }

        public override void OnUpdate(float deltaTime)
        {
            if (Input.IsKeyPressed(Key.P)) // Press 'P' to spawn hazard via cloning
            {
                if (prototype != null)
                {
                    HazardUnit clone = (HazardUnit)prototype.Clone();
                    Log.Info($"Clonned new unit: {clone.ToString()}");
                    // In a more complete implementation, we'd also instantiate a new Entity here.
                }
            }
        }
    }
}
