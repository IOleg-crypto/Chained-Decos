using Chained;

namespace ChainedDecos.Scripts
{
    /// <summary>
    /// Attach directly to the "Send" chat button. Reads text from the input field,
    /// sends it, and clears the field. Enter-key sending in LobbyUI still works in parallel.
    /// </summary>
    public class SendChatButton : Script
    {
        public string ChatInputTag = "chat_input";

        public override void OnUpdate(float deltaTime)
        {
            ButtonControl? btn = Entity.GetComponent<ButtonControl>();
            if (btn == null || !btn.IsClicked)
                return;

            Entity? inputEntity = Scene.FindEntityByTag(ChatInputTag);
            if (inputEntity == null)
                return;

            InputTextControl? input = inputEntity.GetComponent<InputTextControl>();
            if (input == null || string.IsNullOrWhiteSpace(input.Text))
                return;

            Network.SendChatMessage(input.Text.Trim());
            input.Text = "";
        }
    }
}
