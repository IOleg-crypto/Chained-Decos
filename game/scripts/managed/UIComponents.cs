using System;
using System.Runtime.InteropServices;

namespace ChainedEngine
{
    public class UIButton
    {
        private readonly uint _entityID;
        internal UIButton(uint entityID) { _entityID = entityID; }

        public string Text
        {
            get => Marshal.PtrToStringUni(InternalCalls.UIButton_GetText(_entityID));
            set => InternalCalls.UIButton_SetText(_entityID, Marshal.StringToCoTaskMemUni(value));
        }

        public bool IsClicked => InternalCalls.UIButton_IsClicked(_entityID);
    }

    public class UIText
    {
        private readonly uint _entityID;
        internal UIText(uint entityID) { _entityID = entityID; }

        public string Text
        {
            get => Marshal.PtrToStringUni(InternalCalls.UIText_GetText(_entityID));
            set => InternalCalls.UIText_SetText(_entityID, Marshal.StringToCoTaskMemUni(value));
        }

        public float FontSize
        {
            get => InternalCalls.UIText_GetFontSize(_entityID);
            set => InternalCalls.UIText_SetFontSize(_entityID, value);
        }
    }

    public partial class Entity
    {
        private UIButton _button;
        private UIText _text;

        public UIButton Button
        {
            get
            {
                if (_button == null && InternalCalls.Entity_HasComponent(_entityID, "UIButton"))
                    _button = new UIButton(_entityID);
                return _button;
            }
        }

        public UIText Text
        {
            get
            {
                if (_text == null && InternalCalls.Entity_HasComponent(_entityID, "UIText"))
                    _text = new UIText(_entityID);
                return _text;
            }
        }
    }
}
