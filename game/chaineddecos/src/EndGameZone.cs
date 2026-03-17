using System;
using CHEngine;

namespace ChainedDecos.Scripts
{
    public class EndGameZone : Script
    {
        public override void OnCollisionEnter(ulong otherID)
        {
            Entity other = new Entity(otherID);

            // Check if the colliding entity is the player
            TagComponent? tagComp = other.GetComponent<TagComponent>();
            if (other.HasComponent<PlayerComponent>() || (tagComp != null && tagComp.Tag == "Player"))
            {
                Log.Info("Player collided with EndGameZone! Loading main menu...");
                Scene.LoadScene("scenes/main_menu.chscene");
            }
        }
    }
}
