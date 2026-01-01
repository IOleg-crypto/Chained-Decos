using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace ChainedEngine
{
    public enum UIAnchor : byte
    {
        TopLeft,
        TopCenter,
        TopRight,
        MiddleLeft,
        MiddleCenter,
        MiddleRight,
        BottomLeft,
        BottomCenter,
        BottomRight
    }

    public class RectTransform
    {
        private readonly uint _entityID;

        internal RectTransform(uint entityID)
        {
            _entityID = entityID;
        }

        public Vector2 Position
        {
            get
            {
                InternalCalls.RectTransform_GetPosition(_entityID, out Vector2 result);
                return result;
            }
            set => InternalCalls.RectTransform_SetPosition(_entityID, ref value);
        }

        public Vector2 Size
        {
            get
            {
                InternalCalls.RectTransform_GetSize(_entityID, out Vector2 result);
                return result;
            }
            set => InternalCalls.RectTransform_SetSize(_entityID, ref value);
        }

        public UIAnchor Anchor
        {
            get => (UIAnchor)InternalCalls.RectTransform_GetAnchor(_entityID);
            set => InternalCalls.RectTransform_SetAnchor(_entityID, (byte)value);
        }

        public bool Active
        {
            get => InternalCalls.RectTransform_IsActive(_entityID);
            set => InternalCalls.RectTransform_SetActive(_entityID, value);
        }
    }

    public partial class Entity
    {
        private RectTransform _rectTransform;

        public RectTransform RectTransform
        {
            get
            {
                if (_rectTransform == null)
                    _rectTransform = new RectTransform(ID);
                return _rectTransform;
            }
        }
    }
}
