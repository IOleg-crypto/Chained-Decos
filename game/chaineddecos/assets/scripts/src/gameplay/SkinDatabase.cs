using System;
using Chained;

namespace ChainedDecos.Scripts
{
    /// <summary>
    /// Database of available player skins (colors) for the lobby.
    /// </summary>
    public static class SkinDatabase
    {
        public static readonly Vector4[] Colors = new Vector4[]
        {
            new Vector4(1.0f, 1.0f, 1.0f, 1.0f),   // White
            new Vector4(1.0f, 0.2f, 0.2f, 1.0f),   // Red
            new Vector4(0.2f, 0.8f, 0.2f, 1.0f),   // Green
            new Vector4(0.2f, 0.4f, 1.0f, 1.0f),   // Blue
            new Vector4(1.0f, 0.8f, 0.0f, 1.0f),   // Yellow
            new Vector4(0.8f, 0.2f, 1.0f, 1.0f),   // Purple
            new Vector4(0.0f, 0.8f, 0.8f, 1.0f),   // Cyan
            new Vector4(1.0f, 0.5f, 0.0f, 1.0f),   // Orange
        };

        public static readonly string[] Names = new string[]
        {
            "White", "Red", "Green", "Blue", "Yellow", "Purple", "Cyan", "Orange"
        };

        public static int SkinCount => Colors.Length;

        public static Vector4 GetColor(int index)
        {
            if (index < 0 || index >= Colors.Length)
                return Colors[0];
            return Colors[index];
        }

        public static string GetName(int index)
        {
            if (index < 0 || index >= Names.Length)
                return Names[0];
            return Names[index];
        }
    }
}
