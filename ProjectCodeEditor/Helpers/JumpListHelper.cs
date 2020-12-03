﻿using System;
using System.Threading.Tasks;
using Windows.UI.StartScreen;

namespace ProjectCodeEditor.Helpers
{
    public static class JumpListHelper
    {
        public static JumpList AppJumpList;

        public static async Task InitializeAsync()
        {
            if (JumpList.IsSupported())
            {
                AppJumpList = await JumpList.LoadCurrentAsync();
                AppJumpList.SystemGroupKind = JumpListSystemGroupKind.None;
                await AppJumpList.SaveAsync();
            }
        }
    }
}