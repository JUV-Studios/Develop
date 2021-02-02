﻿#pragma once

#include "AppSettings.g.h"
#include <winrt\DevelopManaged.h>

namespace winrt::Develop::implementation
{
    struct AppSettings : AppSettingsT<AppSettings>
    {
        AppSettings() = delete;
        static Windows::Foundation::Collections::IVectorView<hstring> SupportedFileTypes();
        static Windows::Foundation::IAsyncAction Initialize();
        static DevelopManaged::SettingsViewModel Preferences();
    };
}

namespace winrt::Develop::factory_implementation
{
    struct AppSettings : AppSettingsT<AppSettings, implementation::AppSettings>
    {
    };
}