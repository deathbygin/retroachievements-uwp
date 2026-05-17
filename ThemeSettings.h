// ThemeSettings.h
// Shared helper for reading and applying user-customizable theme settings.
// Settings are persisted in LocalSettings so they survive app restarts.

#pragma once

#include <string>
#include <cmath>
#include <ppltasks.h>
#include <collection.h>
#include <Windows.UI.ViewManagement.h>

namespace RetroAchievements_UWP
{
    class ThemeSettings
    {
    public:
        // Returns the saved accent color or the default Purple.
        static Platform::String^ GetAccentColor()
        {
            int theme = GetAppTheme();
            if (theme == 4) return "#316B9D"; // Steam Blue
            if (theme == 6) return "#E50914"; // Netflix Red
            if (theme == 7) return "#BBD08F"; // Retro Boy Light Green Accent
            if (theme == 8) return "#ffcc00"; // Hedgehog Gold
            if (theme == 10) return "#52B043"; // Blade Xbox Lime Green

            auto localSettings = Windows::Storage::ApplicationData::Current->LocalSettings;
            if (localSettings->Values->HasKey("AccentColor"))
            {
                return safe_cast<Platform::String^>(localSettings->Values->Lookup("AccentColor"));
            }
            return "#7B2FF7";
        }

        // Saves the chosen accent color.
        static void SetAccentColor(Platform::String^ hex)
        {
            auto localSettings = Windows::Storage::ApplicationData::Current->LocalSettings;
            localSettings->Values->Insert("AccentColor", hex);
        }

        // Returns the list of colors from the Xbox palette image.
        static Windows::Foundation::Collections::IVector<Platform::String^>^ GetAvailableAccentColors()
        {
            auto colors = ref new Platform::Collections::Vector<Platform::String^>();
            // Row 1
            colors->Append("#78BA00"); // Lime
            colors->Append("#107C10"); // Green
            colors->Append("#00AB94"); // Teal
            colors->Append("#006D60"); // Dark Teal
            // Row 2
            colors->Append("#316B9D"); // Steam Blue
            colors->Append("#0078D7"); // Blue
            colors->Append("#153E7E"); // Navy
            colors->Append("#7B2FF7"); // Purple
            // Row 3
            colors->Append("#D80073"); // Magenta
            colors->Append("#E671B8"); // Pink
            colors->Append("#E1175E"); // Deep Pink
            colors->Append("#9E1030"); // Maroon
            // Row 4
            colors->Append("#E51400"); // Red
            colors->Append("#F09609"); // Orange
            colors->Append("#FF8F00"); // Gold
            colors->Append("#694121"); // Brown
            // Row 5
            colors->Append("#A2A2A2"); // Light Grey
            colors->Append("#728570"); // Sage
            colors->Append("#647687"); // Steel
            colors->Append("#8E8E8E"); // Silver
            // Row 6
            colors->Append("#333333"); // Charcoal
            return colors;
        }

        // Reads the saved background theme from LocalSettings.
        // 0 = Default, 1 = Light, 2 = Dark (Gray), 3 = AMOLED (Black), 4 = Steam, 5 = Arcade Hero
        static int GetAppTheme()
        {
            auto localSettings = Windows::Storage::ApplicationData::Current->LocalSettings;
            if (localSettings->Values->HasKey("AppTheme"))
            {
                int theme = safe_cast<int>(localSettings->Values->Lookup("AppTheme"));
                if (theme > 11) return 0; // Safety check: fallback to default if theme index is no longer valid
                return theme;
            }
            // Migrate old setting if it exists
            if (localSettings->Values->HasKey("IsLightTheme"))
            {
                bool isLight = safe_cast<bool>(localSettings->Values->Lookup("IsLightTheme"));
                return isLight ? 1 : 2;
            }
            return 0; // Default to System Default
        }

        // Returns true if the effective theme is light.
        static bool GetIsLightTheme()
        {
            auto theme = GetElementTheme();
            if (theme == Windows::UI::Xaml::ElementTheme::Light) return true;
            if (theme == Windows::UI::Xaml::ElementTheme::Dark) return false;

            // If Default, check system color
            auto uiSettings = ref new Windows::UI::ViewManagement::UISettings();
            auto color = uiSettings->GetColorValue(Windows::UI::ViewManagement::UIColorType::Background);
            return (color.R + color.G + color.B) > (128 * 3);
        }

        // Creates a SolidColorBrush from a hex string like "#107C10"
        static Windows::UI::Xaml::Media::SolidColorBrush^ BrushFromHex(Platform::String^ hex)
        {
            std::wstring h(hex->Data());
            // Remove leading #
            if (!h.empty() && h[0] == L'#') h = h.substr(1);

            unsigned int r = 0, g = 0, b = 0;
            if (h.length() == 6)
            {
                try {
                    r = std::stoul(h.substr(0, 2), nullptr, 16);
                    g = std::stoul(h.substr(2, 2), nullptr, 16);
                    b = std::stoul(h.substr(4, 2), nullptr, 16);
                } catch (...) {
                    // Fallback to Xbox Green if parsing fails
                    r = 0x10; g = 0x7C; b = 0x10;
                }
            }

            Windows::UI::Color color;
            color.A = 255;
            color.R = (unsigned char)r;
            color.G = (unsigned char)g;
            color.B = (unsigned char)b;
            return ref new Windows::UI::Xaml::Media::SolidColorBrush(color);
        }

        // Applies the saved accent color to a Button
        static void ApplyAccentToButton(Windows::UI::Xaml::Controls::Button^ btn)
        {
            if (btn == nullptr) return;
            int theme = GetAppTheme();

            if (theme == 9) // RetroStation: flat white outline, no fill
            {
                auto transparent = ref new Windows::UI::Xaml::Media::SolidColorBrush(Windows::UI::Colors::Transparent);
                auto white       = ref new Windows::UI::Xaml::Media::SolidColorBrush(Windows::UI::Colors::White);
                btn->Background      = transparent;
                btn->Foreground      = white;
                btn->BorderBrush     = white;
                btn->BorderThickness = Windows::UI::Xaml::Thickness(1.5);
            }
            else
            {
                auto accent = GetAccentColor();
                btn->Background = BrushFromHex(accent);
                // Accent buttons always have white text for contrast
                Windows::UI::Color white;
                white.A = 255; white.R = 255; white.G = 255; white.B = 255;
                btn->Foreground = ref new Windows::UI::Xaml::Media::SolidColorBrush(white);
            }
        }

        // Applies theme-aware styling to a "Back" button (now uses accent color)
        static void ApplyThemeToBackButton(Windows::UI::Xaml::Controls::Button^ btn)
        {
            if (btn == nullptr) return;
            ApplyAccentToButton(btn);

            int theme = GetAppTheme();
            if (theme == 5 || theme == 7)
            {
                btn->Content = "<";
            }
        }

        // Applies theme-aware foreground to a TextBlock (for hardcoded-white title text)
        static void ApplyThemeToTitle(Windows::UI::Xaml::Controls::TextBlock^ tb)
        {
            if (tb == nullptr) return;
            if (GetAppTheme() == 7)
            {
                tb->Foreground = BrushFromHex("#122414");
                return;
            }
            bool isLight = GetIsLightTheme();
            Windows::UI::Color fg;
            fg.A = 255;
            if (isLight) { fg.R = 0; fg.G = 0; fg.B = 0; }
            else { fg.R = 255; fg.G = 255; fg.B = 255; }
            tb->Foreground = ref new Windows::UI::Xaml::Media::SolidColorBrush(fg);
        }

        // Applies accent color to a ProgressBar
        static void ApplyAccentToProgressBar(Windows::UI::Xaml::Controls::ProgressBar^ pb)
        {
            if (pb == nullptr) return;
            pb->Foreground = BrushFromHex(GetAccentColor());
        }

        // Gets the RequestedTheme value based on settings
        static Windows::UI::Xaml::ElementTheme GetElementTheme()
        {
            int theme = GetAppTheme();
            if (theme == 1 || theme == 7) return Windows::UI::Xaml::ElementTheme::Light;
            if (theme == 2 || theme == 3 || theme == 4 || theme == 5 || theme == 6 || theme == 8 || theme == 9 || theme == 10) return Windows::UI::Xaml::ElementTheme::Dark;
            return Windows::UI::Xaml::ElementTheme::Default;
        }

        // Applies the accent color to a ResourceDictionary to override system brushes
        static void ApplyAccentToResources(Windows::UI::Xaml::ResourceDictionary^ resources)
        {
            if (resources == nullptr) return;

            int theme = GetAppTheme();

            // 1. Cleanup: Remove all previous custom overrides to ensure a clean slate
            // We do this FIRST before inserting any new values.
            const char* keysToRemove[] = {
                "ApplicationPageBackgroundThemeBrush",
                "SystemControlBackgroundAltHighBrush",
                "SystemControlPageBackgroundAltHighBrush",
                "SystemControlPageBackgroundBaseLowBrush",
                "SystemControlBackgroundChromeMediumBrush",
                "SectionBackgroundBrush",
                "SectionBorderBrush",
                "SectionBorderThickness",
                "SectionCornerRadius",
                "LeaderboardItemBackgroundBrush",
                "HeaderSeparatorVisibility",
                "ContentControlThemeFontFamily",
                "ControlContentThemeFontFamily",
                "TextBlockTemplateFontFamily",
                "PivotHeaderItemFontFamily",
                "ComboBoxItemFontFamily",
                "ListViewItemFontFamily",
                "GridViewItemFontFamily",
                "ButtonFontFamily",
                "CheckBoxFontFamily",
                "RadioButtonFontFamily",
                "ToggleSwitchHeaderFontFamily",
                "ContentControlThemeFontWeight",
                "PivotHeaderItemFontWeight",
                "PivotHeaderItemSelectedFontWeight",
                "PivotHeaderItemForeground",
                "PivotHeaderItemForegroundPointerOver",
                "PivotHeaderItemForegroundPressed",
                "PivotHeaderItemForegroundSelected",
                "PivotHeaderItemForegroundSelectedPointerOver",
                "PivotHeaderItemForegroundSelectedPressed",
                "SystemControlPageTextBaseHighBrush",
                "SystemControlForegroundBaseHighBrush",
                "SystemControlPageTextBaseMediumBrush",
                "SystemControlForegroundBaseMediumBrush",
                "SystemControlPageTextBaseLowBrush",
                "SystemControlForegroundBaseLowBrush",
                "SystemControlPageTextTitleBrush",
                "SystemControlForegroundTitleBrush",
                "SystemControlPageTextSubheaderBrush",
                "SystemControlForegroundSubheaderBrush",
                "SystemControlPageTextBodyBrush",
                "SystemControlForegroundBodyBrush",
                "SystemControlPageTextCaptionBrush",
                "SystemControlForegroundCaptionBrush",
                "ListViewItemForeground",
                "ListViewItemForegroundPointerOver",
                "ListViewItemForegroundSelected",
                "GridViewItemForeground",
                "GridViewItemForegroundPointerOver",
                "GridViewItemForegroundSelected",
                "ButtonForeground",
                "ButtonForegroundPointerOver",
                "ButtonForegroundPressed",
                "SystemControlHighlightAccentBrush",
                "SystemControlHighlightAltAccentBrush",
                "SystemControlForegroundAccentBrush",
                "SystemControlBackgroundAccentBrush",
                "SegaSectionWhiteBrush",
                "SegaSectionWhiteThickness",
                "SegaSectionWhiteCornerRadius",
                "SegaSectionInnerBlueBrush",
                "SegaSectionInnerBlueThickness",
                "SegaSectionInnerBlueCornerRadius",
                "ButtonBackground",
                "ButtonBackgroundPointerOver",
                "ButtonBackgroundPressed",
                "ButtonBackgroundDisabled",
                "ButtonForegroundDisabled",
                "ButtonBorderBrush",
                "ButtonBorderBrushPointerOver",
                "ButtonBorderBrushPressed",
                "ButtonBorderBrushDisabled"
            };

            for (const char* key : keysToRemove)
            {
                Platform::String^ pKey = ref new Platform::String(std::wstring(key, key + strlen(key)).c_str());
                if (resources->HasKey(pKey)) resources->Remove(pKey);
            }

            // Default Sega resources to zero for all themes (will be overridden by Hedgehog theme)
            auto transparentBrush = ref new Windows::UI::Xaml::Media::SolidColorBrush(Windows::UI::Colors::Transparent);
            resources->Insert("SegaSectionWhiteBrush", transparentBrush);
            resources->Insert("SegaSectionWhiteThickness", Windows::UI::Xaml::Thickness(0));
            resources->Insert("SegaSectionWhiteCornerRadius", Windows::UI::Xaml::CornerRadius(0));
            resources->Insert("SegaSectionInnerBlueBrush", transparentBrush);
            resources->Insert("SegaSectionInnerBlueThickness", Windows::UI::Xaml::Thickness(0));
            resources->Insert("SegaSectionInnerBlueCornerRadius", Windows::UI::Xaml::CornerRadius(0));

            auto accentColorStr = GetAccentColor();
            auto accentBrush = BrushFromHex(accentColorStr);
            auto accentColor = accentBrush->Color;

            // Default: buttons use accent fill. RetroStation overrides this to transparent below.
            if (theme != 9)
            {
                resources->Insert("ButtonBackground",             accentBrush);
                resources->Insert("ButtonBackgroundPointerOver",  accentBrush);
                resources->Insert("ButtonBackgroundPressed",      accentBrush);
                resources->Insert("ButtonBorderBrush",            accentBrush);
                resources->Insert("ButtonBorderBrushPointerOver", accentBrush);
                resources->Insert("ButtonBorderBrushPressed",     accentBrush);
            }

            // Update Color resources
            resources->Insert("SystemAccentColor", accentColor);

            // Update Brush resources
            resources->Insert("SystemControlHighlightAccentBrush", accentBrush);
            resources->Insert("SystemControlHighlightAltAccentBrush", accentBrush);
            
            // ToggleSwitch resources
            resources->Insert("ToggleSwitchFillOn", accentBrush);
            resources->Insert("ToggleSwitchFillOnPointerOver", accentBrush);
            resources->Insert("ToggleSwitchFillOnPressed", accentBrush);
            resources->Insert("ToggleSwitchKnobFillOn", ref new Windows::UI::Xaml::Media::SolidColorBrush(Windows::UI::Colors::White));
            
            // Pivot resources
            resources->Insert("PivotHeaderItemSelectedPipeBrush", accentBrush);
            resources->Insert("PivotHeaderItemSelectedForegroundBrush", accentBrush);
            
            // GridView / ListView selection and hover resources
            resources->Insert("ListViewItemSelectedBackground", accentBrush);
            resources->Insert("ListViewItemSelectedPointerOverBackground", accentBrush);
            resources->Insert("ListViewItemSelectedPressedBackground", accentBrush);
            resources->Insert("SystemControlHighlightListAccentLowBrush", accentBrush);
            resources->Insert("SystemControlHighlightListAccentMediumBrush", accentBrush);
            resources->Insert("SystemControlHighlightListAccentHighBrush", accentBrush);
            
            // For ProgressBar in DataTemplates (if they use ThemeResource)
            resources->Insert("ProgressBarForegroundThemeBrush", accentBrush);
            resources->Insert("SystemControlProgressBarForegroundBrush", accentBrush);
            resources->Insert("ProgressBarTrackForegroundThemeBrush", accentBrush);

            // 2. Set defaults for custom keys we invented (to avoid XAML crashes)
            resources->Insert("SectionBackgroundBrush", ref new Windows::UI::Xaml::Media::SolidColorBrush(Windows::UI::Colors::Transparent));
            resources->Insert("SectionBorderBrush", ref new Windows::UI::Xaml::Media::SolidColorBrush(Windows::UI::Colors::Transparent));
            resources->Insert("SectionBorderThickness", Windows::UI::Xaml::Thickness(0));
            resources->Insert("SectionCornerRadius", Windows::UI::Xaml::CornerRadius(4));
            resources->Insert("LeaderboardItemBackgroundBrush", resources->Lookup("SystemControlBackgroundChromeMediumLowBrush"));
            resources->Insert("HeaderSeparatorVisibility", Windows::UI::Xaml::Visibility::Visible);

            // 3. Apply theme-specific overrides
            if (theme == 1) // Light
            {
                auto whiteBrush = ref new Windows::UI::Xaml::Media::SolidColorBrush(Windows::UI::Colors::White);
                auto blackBrush = ref new Windows::UI::Xaml::Media::SolidColorBrush(Windows::UI::Colors::Black);
                resources->Insert("ApplicationPageBackgroundThemeBrush", whiteBrush);
                resources->Insert("SystemControlBackgroundAltHighBrush", whiteBrush);
                resources->Insert("SystemControlPageBackgroundAltHighBrush", whiteBrush);
                resources->Insert("SystemControlPageBackgroundBaseLowBrush", whiteBrush);

                // Ensure Pivot (Tab) headers are readable (black text on white background)
                resources->Insert("PivotHeaderItemForegroundSelected", blackBrush);
                resources->Insert("PivotHeaderItemForegroundSelectedPointerOver", blackBrush);
                resources->Insert("PivotHeaderItemForegroundSelectedPressed", blackBrush);
                resources->Insert("PivotHeaderItemForeground", BrushFromHex("#666666")); // Medium gray for inactive

                // Explicitly set text color for Light theme
                resources->Insert("SystemControlPageTextBaseHighBrush", blackBrush);
                resources->Insert("SystemControlForegroundBaseHighBrush", blackBrush);
                resources->Insert("SystemControlPageTextBaseMediumBrush", BrushFromHex("#666666"));
                resources->Insert("SystemControlForegroundBaseMediumBrush", BrushFromHex("#666666"));
                resources->Insert("SystemControlPageTextBaseLowBrush", BrushFromHex("#999999"));
                resources->Insert("SystemControlForegroundBaseLowBrush", BrushFromHex("#999999"));
            }
            else if (theme == 2) // Dark (Gray)
            {
                auto grayBrush = BrushFromHex("#121212"); // Xbox-style Dark Gray
                resources->Insert("ApplicationPageBackgroundThemeBrush", grayBrush);
                resources->Insert("SystemControlBackgroundAltHighBrush", grayBrush);
                resources->Insert("SystemControlPageBackgroundAltHighBrush", grayBrush);
                resources->Insert("SystemControlPageBackgroundBaseLowBrush", grayBrush);

                auto whiteBrush = ref new Windows::UI::Xaml::Media::SolidColorBrush(Windows::UI::Colors::White);
                resources->Insert("SystemControlPageTextBaseHighBrush", whiteBrush);
                resources->Insert("SystemControlForegroundBaseHighBrush", whiteBrush);
                resources->Insert("SystemControlPageTextBaseMediumBrush", BrushFromHex("#B3B3B3"));
                resources->Insert("SystemControlForegroundBaseMediumBrush", BrushFromHex("#B3B3B3"));
                resources->Insert("SystemControlPageTextBaseLowBrush", BrushFromHex("#666666"));
                resources->Insert("SystemControlForegroundBaseLowBrush", BrushFromHex("#666666"));

                // Reset Pivot headers for Dark theme to use accent color
                resources->Insert("PivotHeaderItemForegroundSelected", accentBrush);
                resources->Insert("PivotHeaderItemForeground", ref new Windows::UI::Xaml::Media::SolidColorBrush(Windows::UI::Colors::Gray));
            }
            else if (theme == 3) // AMOLED (Black)
            {
                auto blackBrush = BrushFromHex("#000000");
                resources->Insert("ApplicationPageBackgroundThemeBrush", blackBrush);
                resources->Insert("SystemControlBackgroundAltHighBrush", blackBrush);
                resources->Insert("SystemControlPageBackgroundAltHighBrush", blackBrush);
                resources->Insert("SystemControlPageBackgroundBaseLowBrush", blackBrush);

                // Reset Pivot headers for AMOLED theme to use accent color
                resources->Insert("PivotHeaderItemForegroundSelected", accentBrush);
                resources->Insert("PivotHeaderItemForeground", ref new Windows::UI::Xaml::Media::SolidColorBrush(Windows::UI::Colors::Gray));
            }
            else if (theme == 4) // Steam
            {
                // Steam dark navy background: #1b2838
                auto steamBgBrush    = BrushFromHex("#1b2838");
                // Slightly lighter blue surface for panels: #2a475e
                auto steamPanelBrush = BrushFromHex("#2a475e");
                resources->Insert("ApplicationPageBackgroundThemeBrush",      steamBgBrush);
                resources->Insert("SystemControlBackgroundAltHighBrush",       steamPanelBrush);
                resources->Insert("SystemControlPageBackgroundAltHighBrush",   steamPanelBrush);
                resources->Insert("SystemControlPageBackgroundBaseLowBrush",   steamPanelBrush);
                resources->Insert("SystemControlBackgroundChromeMediumBrush",  steamPanelBrush);

                // Section background for grid boxes: #0C161F
                resources->Insert("SectionBackgroundBrush", BrushFromHex("#0C161F"));
                resources->Insert("LeaderboardItemBackgroundBrush", BrushFromHex("#0C161F"));

                // Hide header separator for Steam theme
                resources->Insert("HeaderSeparatorVisibility", Windows::UI::Xaml::Visibility::Collapsed);

                // Set Pivot headers for Steam theme
                resources->Insert("PivotHeaderItemForegroundSelected", ref new Windows::UI::Xaml::Media::SolidColorBrush(Windows::UI::Colors::White));
                resources->Insert("PivotHeaderItemForeground", BrushFromHex("#666666"));

                // Explicitly set text colors for Steam theme
                auto whiteBrush = ref new Windows::UI::Xaml::Media::SolidColorBrush(Windows::UI::Colors::White);
                auto grayBrush = BrushFromHex("#A2A2A2");
                resources->Insert("SystemControlPageTextBaseHighBrush", whiteBrush);
                resources->Insert("SystemControlForegroundBaseHighBrush", whiteBrush);
                resources->Insert("SystemControlPageTextBaseMediumBrush", grayBrush);
                resources->Insert("SystemControlForegroundBaseMediumBrush", grayBrush);
            }
            else if (theme == 5) // Arcade Hero
            {
                auto blackBrush = BrushFromHex("#000000");
                resources->Insert("ApplicationPageBackgroundThemeBrush", blackBrush);
                resources->Insert("SystemControlBackgroundAltHighBrush", blackBrush);
                resources->Insert("SystemControlPageBackgroundAltHighBrush", blackBrush);
                resources->Insert("SystemControlPageBackgroundBaseLowBrush", blackBrush);

                // Inject Pixel Font globally (Silkscreen is more readable at small sizes)
                auto pixelFont = ref new Windows::UI::Xaml::Media::FontFamily("ms-appx:///Assets/Fonts/Silkscreen.ttf#Silkscreen");
                resources->Insert("ContentControlThemeFontFamily", pixelFont);
                resources->Insert("ControlContentThemeFontFamily", pixelFont);
                resources->Insert("TextBlockTemplateFontFamily", pixelFont);
                resources->Insert("PivotHeaderItemFontFamily", pixelFont);
                resources->Insert("ComboBoxItemFontFamily", pixelFont);
                resources->Insert("ListViewItemFontFamily", pixelFont);
                resources->Insert("GridViewItemFontFamily", pixelFont);
                resources->Insert("ButtonFontFamily", pixelFont);
                resources->Insert("CheckBoxFontFamily", pixelFont);
                resources->Insert("RadioButtonFontFamily", pixelFont);
                resources->Insert("ToggleSwitchHeaderFontFamily", pixelFont);
                
                // Force font weight to normal for these theme resources
                resources->Insert("ContentControlThemeFontWeight", Windows::UI::Text::FontWeights::Normal);
                resources->Insert("PivotHeaderItemFontWeight", Windows::UI::Text::FontWeights::Normal);
                resources->Insert("PivotHeaderItemSelectedFontWeight", Windows::UI::Text::FontWeights::Normal);

                // Set Pivot headers for Arcade Hero theme
                resources->Insert("PivotHeaderItemForegroundSelected", accentBrush);
                resources->Insert("PivotHeaderItemForeground", ref new Windows::UI::Xaml::Media::SolidColorBrush(Windows::UI::Colors::Gray));

                // Explicitly set text colors for Arcade Hero theme
                auto whiteBrush = ref new Windows::UI::Xaml::Media::SolidColorBrush(Windows::UI::Colors::White);
                auto grayBrush = BrushFromHex("#A2A2A2");
                resources->Insert("SystemControlPageTextBaseHighBrush", whiteBrush);
                resources->Insert("SystemControlForegroundBaseHighBrush", whiteBrush);
                resources->Insert("SystemControlPageTextBaseMediumBrush", grayBrush);
                resources->Insert("SystemControlForegroundBaseMediumBrush", grayBrush);
            }
            else if (theme == 6) // Netflix
            {
                auto blackBrush = BrushFromHex("#000000");
                resources->Insert("ApplicationPageBackgroundThemeBrush", blackBrush);
                resources->Insert("SystemControlBackgroundAltHighBrush", blackBrush);
                resources->Insert("SystemControlPageBackgroundAltHighBrush", blackBrush);
                resources->Insert("SystemControlPageBackgroundBaseLowBrush", blackBrush);

                // Hide header separator for Netflix theme
                resources->Insert("HeaderSeparatorVisibility", Windows::UI::Xaml::Visibility::Collapsed);

                // Set Pivot headers for Netflix theme
                resources->Insert("PivotHeaderItemForegroundSelected", accentBrush);
                resources->Insert("PivotHeaderItemForeground", ref new Windows::UI::Xaml::Media::SolidColorBrush(Windows::UI::Colors::Gray));

                // Explicitly set text colors for Netflix theme
                auto whiteBrush = ref new Windows::UI::Xaml::Media::SolidColorBrush(Windows::UI::Colors::White);
                auto grayBrush = BrushFromHex("#A2A2A2");
                resources->Insert("SystemControlPageTextBaseHighBrush", whiteBrush);
                resources->Insert("SystemControlForegroundBaseHighBrush", whiteBrush);
                resources->Insert("SystemControlPageTextBaseMediumBrush", grayBrush);
                resources->Insert("SystemControlForegroundBaseMediumBrush", grayBrush);
            }
            else if (theme == 7) // Retro Boy
            {
                auto bgBrush = BrushFromHex("#E0EECB");
                auto accentBrush = BrushFromHex("#BBD08F");
                auto textBrush = BrushFromHex("#122414");
                auto transparentBrush = ref new Windows::UI::Xaml::Media::SolidColorBrush(Windows::UI::Colors::Transparent);

                resources->Insert("ApplicationPageBackgroundThemeBrush", bgBrush);
                resources->Insert("SystemControlBackgroundAltHighBrush", bgBrush);
                resources->Insert("SystemControlPageBackgroundAltHighBrush", bgBrush);
                resources->Insert("SystemControlPageBackgroundBaseLowBrush", bgBrush);
                resources->Insert("SystemControlBackgroundChromeMediumBrush", bgBrush);
                
                resources->Insert("SectionBackgroundBrush", transparentBrush);
                resources->Insert("SectionBorderBrush", BrushFromHex("#738659"));
                resources->Insert("SectionBorderThickness", Windows::UI::Xaml::Thickness(6));
                resources->Insert("SectionCornerRadius", Windows::UI::Xaml::CornerRadius(15));
                resources->Insert("LeaderboardItemBackgroundBrush", transparentBrush);

                // Text Color Overrides
                resources->Insert("SystemControlPageTextBaseHighBrush", textBrush);
                resources->Insert("SystemControlForegroundBaseHighBrush", textBrush);
                resources->Insert("SystemControlPageTextBaseMediumBrush", textBrush);
                resources->Insert("SystemControlForegroundBaseMediumBrush", textBrush);
                resources->Insert("SystemControlPageTextBaseLowBrush", textBrush);
                resources->Insert("SystemControlForegroundBaseLowBrush", textBrush);
                resources->Insert("SystemControlPageTextTitleBrush", textBrush);
                resources->Insert("SystemControlForegroundTitleBrush", textBrush);
                resources->Insert("SystemControlPageTextSubheaderBrush", textBrush);
                resources->Insert("SystemControlForegroundSubheaderBrush", textBrush);

                // Tooltip Overrides
                auto tooltipBrush = BrushFromHex("#BBCD8F");
                resources->Insert("ToolTipBackground", tooltipBrush);
                resources->Insert("ToolTipBorderBrush", BrushFromHex("#738659"));

                resources->Insert("SystemControlPageTextBodyBrush", textBrush);
                resources->Insert("SystemControlForegroundBodyBrush", textBrush);
                resources->Insert("SystemControlPageTextCaptionBrush", textBrush);
                resources->Insert("SystemControlForegroundCaptionBrush", textBrush);

                resources->Insert("ListViewItemForeground", textBrush);
                resources->Insert("ListViewItemForegroundPointerOver", textBrush);
                resources->Insert("ListViewItemForegroundSelected", textBrush);
                resources->Insert("GridViewItemForeground", textBrush);
                resources->Insert("GridViewItemForegroundPointerOver", textBrush);
                resources->Insert("GridViewItemForegroundSelected", textBrush);
                resources->Insert("ButtonForeground", textBrush);
                resources->Insert("ButtonForegroundPointerOver", textBrush);
                resources->Insert("ButtonForegroundPressed", textBrush);

                resources->Insert("PivotHeaderItemForegroundSelected", textBrush);
                resources->Insert("PivotHeaderItemForeground", textBrush);

                // Inject Pixel Font globally
                auto pixelFont = ref new Windows::UI::Xaml::Media::FontFamily("ms-appx:///Assets/Fonts/Silkscreen.ttf#Silkscreen");
                resources->Insert("ContentControlThemeFontFamily", pixelFont);
                resources->Insert("ControlContentThemeFontFamily", pixelFont);
                resources->Insert("TextBlockTemplateFontFamily", pixelFont);
                resources->Insert("PivotHeaderItemFontFamily", pixelFont);
                resources->Insert("ComboBoxItemFontFamily", pixelFont);
                resources->Insert("ListViewItemFontFamily", pixelFont);
                resources->Insert("GridViewItemFontFamily", pixelFont);
                resources->Insert("ButtonFontFamily", pixelFont);
                resources->Insert("CheckBoxFontFamily", pixelFont);
                resources->Insert("RadioButtonFontFamily", pixelFont);
                resources->Insert("ToggleSwitchHeaderFontFamily", pixelFont);

                resources->Insert("ContentControlThemeFontWeight", Windows::UI::Text::FontWeights::Normal);
                resources->Insert("PivotHeaderItemFontWeight", Windows::UI::Text::FontWeights::Normal);
                resources->Insert("PivotHeaderItemSelectedFontWeight", Windows::UI::Text::FontWeights::Normal);
                
                // Hide header separator for Retro Boy theme
                resources->Insert("HeaderSeparatorVisibility", Windows::UI::Xaml::Visibility::Collapsed);
            }
            else if (theme == 8) // Hedgehog
            {
                auto segaBlue = BrushFromHex("#004d9c");
                auto hedgehogAccent = BrushFromHex("#ffcc00"); // Gold
                auto whiteBrush = ref new Windows::UI::Xaml::Media::SolidColorBrush(Windows::UI::Colors::White);

                resources->Insert("ApplicationPageBackgroundThemeBrush", segaBlue);
                resources->Insert("SystemControlBackgroundAltHighBrush", BrushFromHex("#003366"));
                resources->Insert("SystemControlPageBackgroundAltHighBrush", BrushFromHex("#003366"));
                
                resources->Insert("SystemControlPageTextBaseHighBrush", whiteBrush);
                resources->Insert("SystemControlForegroundBaseHighBrush", whiteBrush);
                
                // Set Pivot headers for Hedgehog theme
                resources->Insert("PivotHeaderItemForegroundSelected", hedgehogAccent);
                resources->Insert("PivotHeaderItemForeground", whiteBrush);
                
                auto hedgehogFont = ref new Windows::UI::Xaml::Media::FontFamily("ms-appx:///Assets/Theme/Hedgehog/Sage.otf#Sage");
                resources->Insert("PivotHeaderItemFontFamily", hedgehogFont);
                
                resources->Insert("SectionBackgroundBrush", BrushFromHex("#00264d"));
                resources->Insert("LeaderboardItemBackgroundBrush", BrushFromHex("#00264d"));
                
                // Sega Triple Border Resources
                resources->Insert("SectionBorderBrush", segaBlue);
                resources->Insert("SectionBorderThickness", Windows::UI::Xaml::Thickness(4));
                resources->Insert("SectionCornerRadius", Windows::UI::Xaml::CornerRadius(15));
                
                resources->Insert("SegaSectionWhiteBrush", whiteBrush);
                resources->Insert("SegaSectionWhiteThickness", Windows::UI::Xaml::Thickness(2));
                resources->Insert("SegaSectionWhiteCornerRadius", Windows::UI::Xaml::CornerRadius(11));
                
                resources->Insert("SegaSectionInnerBlueBrush", segaBlue);
                resources->Insert("SegaSectionInnerBlueThickness", Windows::UI::Xaml::Thickness(4));
                resources->Insert("SegaSectionInnerBlueCornerRadius", Windows::UI::Xaml::CornerRadius(9));
            }
            else if (theme == 9) // RetroStation – transparent boxes so the XMB wave shows through
            {
                auto rsAccent   = BrushFromHex("#90CAF9"); // Sky Blue (PS XMB style)
                auto whiteBrush = ref new Windows::UI::Xaml::Media::SolidColorBrush(Windows::UI::Colors::White);
                auto transparent = ref new Windows::UI::Xaml::Media::SolidColorBrush(Windows::UI::Colors::Transparent);

                // Page background driven by the XMB canvas; keep resource in sync
                resources->Insert("ApplicationPageBackgroundThemeBrush", transparent);
                resources->Insert("SystemControlBackgroundAltHighBrush", transparent);
                resources->Insert("SystemControlPageBackgroundAltHighBrush", transparent);

                resources->Insert("SystemControlPageTextBaseHighBrush", whiteBrush);
                resources->Insert("SystemControlForegroundBaseHighBrush", whiteBrush);

                // Pivot headers
                resources->Insert("PivotHeaderItemForegroundSelected", rsAccent);
                resources->Insert("PivotHeaderItemForeground", whiteBrush);

                // No box backgrounds – fully transparent
                resources->Insert("SectionBackgroundBrush", transparent);
                resources->Insert("LeaderboardItemBackgroundBrush", transparent);

                // No borders at all
                resources->Insert("SectionBorderBrush", transparent);
                resources->Insert("SectionBorderThickness", Windows::UI::Xaml::Thickness(0));
                resources->Insert("SectionCornerRadius", Windows::UI::Xaml::CornerRadius(0));

                resources->Insert("SegaSectionWhiteBrush", transparent);
                resources->Insert("SegaSectionWhiteThickness", Windows::UI::Xaml::Thickness(0));
                resources->Insert("SegaSectionWhiteCornerRadius", Windows::UI::Xaml::CornerRadius(0));

                resources->Insert("SegaSectionInnerBlueBrush", transparent);
                resources->Insert("SegaSectionInnerBlueThickness", Windows::UI::Xaml::Thickness(0));
                resources->Insert("SegaSectionInnerBlueCornerRadius", Windows::UI::Xaml::CornerRadius(0));

                resources->Insert("HeaderSeparatorVisibility", Windows::UI::Xaml::Visibility::Collapsed);

                // Flat white outline buttons – no filled background
                resources->Insert("ButtonBackground",             transparent);
                resources->Insert("ButtonBackgroundPointerOver",  BrushFromHex("#22FFFFFF"));
                resources->Insert("ButtonBackgroundPressed",      BrushFromHex("#44FFFFFF"));
                resources->Insert("ButtonBackgroundDisabled",     transparent);
                resources->Insert("ButtonForeground",             whiteBrush);
                resources->Insert("ButtonForegroundPointerOver",  whiteBrush);
                resources->Insert("ButtonForegroundPressed",      whiteBrush);
                resources->Insert("ButtonForegroundDisabled",     BrushFromHex("#66FFFFFF"));
                resources->Insert("ButtonBorderBrush",            whiteBrush);
                resources->Insert("ButtonBorderBrushPointerOver", whiteBrush);
                resources->Insert("ButtonBorderBrushPressed",     whiteBrush);
                resources->Insert("ButtonBorderBrushDisabled",    BrushFromHex("#44FFFFFF"));
            }
            else if (theme == 10) // Blade – Xbox 360 inspired dark green
            {
                auto xboxGreen     = BrushFromHex("#52B043");
                auto darkBg        = BrushFromHex("#0D0D0D");
                auto sectionBg     = BrushFromHex("#141414");
                auto sectionBorder = BrushFromHex("#52B043");
                auto transparent   = ref new Windows::UI::Xaml::Media::SolidColorBrush(Windows::UI::Colors::Transparent);
                auto whiteBrush2   = ref new Windows::UI::Xaml::Media::SolidColorBrush(Windows::UI::Colors::White);
                auto dimWhite      = BrushFromHex("#99FFFFFF");

                resources->Insert("ApplicationPageBackgroundThemeBrush", darkBg);
                resources->Insert("SystemControlBackgroundAltHighBrush", darkBg);
                resources->Insert("SystemControlPageBackgroundAltHighBrush", darkBg);
                resources->Insert("SystemControlPageTextBaseHighBrush",   whiteBrush2);
                resources->Insert("SystemControlForegroundBaseHighBrush", whiteBrush2);

                resources->Insert("PivotHeaderItemForegroundSelected", xboxGreen);
                resources->Insert("PivotHeaderItemForeground",         dimWhite);

                resources->Insert("SectionBackgroundBrush",        transparent);
                resources->Insert("SectionBorderBrush",            transparent);
                resources->Insert("SectionBorderThickness",        Windows::UI::Xaml::Thickness(0));
                resources->Insert("SectionCornerRadius",           Windows::UI::Xaml::CornerRadius(0));

                resources->Insert("SegaSectionWhiteBrush",              transparent);
                resources->Insert("SegaSectionWhiteThickness",          Windows::UI::Xaml::Thickness(0));
                resources->Insert("SegaSectionWhiteCornerRadius",       Windows::UI::Xaml::CornerRadius(0));
                resources->Insert("SegaSectionInnerBlueBrush",          transparent);
                resources->Insert("SegaSectionInnerBlueThickness",      Windows::UI::Xaml::Thickness(0));
                resources->Insert("SegaSectionInnerBlueCornerRadius",   Windows::UI::Xaml::CornerRadius(0));

                resources->Insert("HeaderSeparatorVisibility", Windows::UI::Xaml::Visibility::Collapsed);

                resources->Insert("ButtonBackground",             xboxGreen);
                resources->Insert("ButtonBackgroundPointerOver",  BrushFromHex("#6AC45A"));
                resources->Insert("ButtonBackgroundPressed",      BrushFromHex("#3D8A31"));
                resources->Insert("ButtonBackgroundDisabled",     BrushFromHex("#333333"));
                resources->Insert("ButtonForeground",             whiteBrush2);
                resources->Insert("ButtonForegroundPointerOver",  whiteBrush2);
                resources->Insert("ButtonForegroundPressed",      whiteBrush2);
                resources->Insert("ButtonForegroundDisabled",     dimWhite);
                resources->Insert("ButtonBorderBrush",            xboxGreen);
                resources->Insert("ButtonBorderBrushPointerOver", BrushFromHex("#6AC45A"));
                resources->Insert("ButtonBorderBrushPressed",     BrushFromHex("#3D8A31"));
                resources->Insert("ButtonBorderBrushDisabled",    BrushFromHex("#333333"));
            }
            else if (theme == 11) // Custom - Copy of Dark Gray for now
            {
                auto grayBrush = BrushFromHex("#121212");
                resources->Insert("ApplicationPageBackgroundThemeBrush", grayBrush);
                resources->Insert("SystemControlBackgroundAltHighBrush", grayBrush);
                resources->Insert("SystemControlPageBackgroundAltHighBrush", grayBrush);
                resources->Insert("SystemControlPageBackgroundBaseLowBrush", grayBrush);

                auto whiteBrush = ref new Windows::UI::Xaml::Media::SolidColorBrush(Windows::UI::Colors::White);
                resources->Insert("SystemControlPageTextBaseHighBrush", whiteBrush);
                resources->Insert("SystemControlForegroundBaseHighBrush", whiteBrush);
                resources->Insert("SystemControlPageTextBaseMediumBrush", BrushFromHex("#B3B3B3"));
                resources->Insert("SystemControlForegroundBaseMediumBrush", BrushFromHex("#B3B3B3"));
                resources->Insert("SystemControlPageTextBaseLowBrush", BrushFromHex("#666666"));
                resources->Insert("SystemControlForegroundBaseLowBrush", BrushFromHex("#666666"));

                resources->Insert("PivotHeaderItemForegroundSelected", accentBrush);
                resources->Insert("PivotHeaderItemForeground", ref new Windows::UI::Xaml::Media::SolidColorBrush(Windows::UI::Colors::Gray));
                resources->Insert("HeaderSeparatorVisibility", Windows::UI::Xaml::Visibility::Collapsed);
                
                auto localSettings = Windows::Storage::ApplicationData::Current->LocalSettings;
                if (localSettings->Values->HasKey("CustomFontPath"))
                {
                    auto fontPath = dynamic_cast<Platform::String^>(localSettings->Values->Lookup("CustomFontPath"));
                    if (fontPath != nullptr)
                    {
                        std::wstring pathStr(fontPath->Data());
                        size_t lastDot = pathStr.find_last_of(L".");
                        std::wstring familyName = (lastDot != std::wstring::npos) ? pathStr.substr(0, lastDot) : pathStr;
                        std::wstring fullPath = L"ms-appdata:///local/" + pathStr + L"#" + familyName;
                        auto customFont = ref new Windows::UI::Xaml::Media::FontFamily(ref new Platform::String(fullPath.c_str()));
                        
                        resources->Insert("ContentControlThemeFontFamily", customFont);
                        resources->Insert("ControlContentThemeFontFamily", customFont);
                        resources->Insert("TextBlockTemplateFontFamily", customFont);
                        resources->Insert("PivotHeaderItemFontFamily", customFont);
                        resources->Insert("ComboBoxItemFontFamily", customFont);
                        resources->Insert("ListViewItemFontFamily", customFont);
                        resources->Insert("GridViewItemFontFamily", customFont);
                        resources->Insert("ButtonFontFamily", customFont);
                        resources->Insert("CheckBoxFontFamily", customFont);
                        resources->Insert("RadioButtonFontFamily", customFont);
                        resources->Insert("ToggleSwitchHeaderFontFamily", customFont);
                    }
                }
            }
        }

        static void ApplyThemeFontToTitle(Windows::UI::Xaml::Controls::TextBlock^ tb)
        {
            if (tb == nullptr) return;
            int theme = GetAppTheme();
            if (theme == 8) // Hedgehog only
            {
                tb->FontFamily = ref new Windows::UI::Xaml::Media::FontFamily("ms-appx:///Assets/Theme/Hedgehog/Sage.otf#Sage");
            }
            else if (theme == 7)
            {
                tb->FontFamily = ref new Windows::UI::Xaml::Media::FontFamily("ms-appx:///Assets/Fonts/Silkscreen.ttf#Silkscreen");
            }
            else if (theme == 11) // Custom
            {
                auto localSettings = Windows::Storage::ApplicationData::Current->LocalSettings;
                if (localSettings->Values->HasKey("CustomFontPath"))
                {
                    auto fontPath = dynamic_cast<Platform::String^>(localSettings->Values->Lookup("CustomFontPath"));
                    if (fontPath != nullptr)
                    {
                        std::wstring pathStr(fontPath->Data());
                        size_t lastDot = pathStr.find_last_of(L".");
                        std::wstring familyName = (lastDot != std::wstring::npos) ? pathStr.substr(0, lastDot) : pathStr;
                        std::wstring fullPath = L"ms-appdata:///local/" + pathStr + L"#" + familyName;
                        tb->FontFamily = ref new Windows::UI::Xaml::Media::FontFamily(ref new Platform::String(fullPath.c_str()));
                        return;
                    }
                }
                tb->ClearValue(Windows::UI::Xaml::Controls::TextBlock::FontFamilyProperty);
            }
            else
            {
                tb->ClearValue(Windows::UI::Xaml::Controls::TextBlock::FontFamilyProperty);
            }
        }

        // ── XMB Wave helpers (RetroStation theme) ──────────────────────────────

        static Windows::UI::Xaml::DispatcherTimer^& WaveTimer()
        {
            static Windows::UI::Xaml::DispatcherTimer^ t = nullptr;
            return t;
        }
        static Windows::UI::Xaml::Controls::Canvas^& WaveCanvas()
        {
            static Windows::UI::Xaml::Controls::Canvas^ c = nullptr;
            return c;
        }
        static double& WaveTime()
        {
            static double t = 0.0;
            return t;
        }

        static void UpdateXMBWave(Windows::UI::Xaml::Controls::Canvas^ canvas)
        {
            if (canvas == nullptr) return;
            WaveTime() += 0.015;
            double t = WaveTime();

            auto bounds = Windows::UI::Xaml::Window::Current->Bounds;
            double W = (bounds.Width  > 1) ? bounds.Width  : 1280;
            double H = (bounds.Height > 1) ? bounds.Height : 800;

            // Wave parameters: baseY fraction, amplitude, frequency, phase, speed
            const int WC = 6;
            double baseY[]  = { 0.22, 0.36, 0.50, 0.62, 0.74, 0.84 };
            double amp[]    = { 40,   55,   70,   58,   42,   28   };
            double freq[]   = { 0.006, 0.0050, 0.0070, 0.0055, 0.006, 0.0080 };
            double phase[]  = { 0.0,   1.0,    2.1,    0.7,    3.4,   1.5    };
            double speed[]  = { 0.7,   0.9,    1.0,    0.8,    1.1,   0.6    };

            for (int i = 0; i < WC && i < (int)canvas->Children->Size; i++)
            {
                auto poly = dynamic_cast<Windows::UI::Xaml::Shapes::Polyline^>(
                                canvas->Children->GetAt(i));
                if (poly == nullptr) continue;
                poly->Points->Clear();
                double by = H * baseY[i];
                for (double x = 0; x <= W; x += 4.0)
                {
                    double y = by + amp[i] * std::sin(x * freq[i] + phase[i] + t * speed[i]);
                    poly->Points->Append(Windows::Foundation::Point((float)x, (float)y));
                }
            }

            // Nodes follow hero wave (index 3)
            const int NC = 5;
            double nodeX[] = { 0.15, 0.30, 0.45, 0.60, 0.75 };
            for (int n = 0; n < NC; n++)
            {
                int idx = WC + n;
                if (idx >= (int)canvas->Children->Size) break;
                auto el = dynamic_cast<Windows::UI::Xaml::Shapes::Ellipse^>(
                              canvas->Children->GetAt(idx));
                if (el == nullptr) continue;
                double x = W * nodeX[n];
                double y = H * baseY[3] + amp[3] * std::sin(x * freq[3] + phase[3] + t * speed[3]);
                Windows::UI::Xaml::Controls::Canvas::SetLeft(el, x - el->Width  / 2);
                Windows::UI::Xaml::Controls::Canvas::SetTop (el, y - el->Height / 2);
            }
        }

        static void StopRetroStationWave(Windows::UI::Xaml::Controls::Grid^ rootGrid)
        {
            if (WaveTimer() != nullptr) { WaveTimer()->Stop(); WaveTimer() = nullptr; }
            WaveCanvas() = nullptr;
            if (rootGrid == nullptr) return;
            for (unsigned int i = 0; i < rootGrid->Children->Size; i++)
            {
                auto c = dynamic_cast<Windows::UI::Xaml::Controls::Canvas^>(rootGrid->Children->GetAt(i));
                if (c != nullptr && c->Name == "XMBWaveCanvas") { rootGrid->Children->RemoveAt(i); break; }
            }
        }

        static void ApplyRetroStationWave(Windows::UI::Xaml::Controls::Grid^ rootGrid)
        {
            if (rootGrid == nullptr) return;
            StopRetroStationWave(rootGrid); // clean up any previous instance

            // ── Parse accent color into RGB ──────────────────────────────────
            Platform::String^ accentHex = GetAccentColor();
            std::wstring hexStr(accentHex->Data());
            if (!hexStr.empty() && hexStr[0] == L'#') hexStr = hexStr.substr(1);
            int aR = 100, aG = 160, aB = 240; // safe fallback
            try {
                aR = std::stoi(hexStr.substr(0, 2), nullptr, 16);
                aG = std::stoi(hexStr.substr(2, 2), nullptr, 16);
                aB = std::stoi(hexStr.substr(4, 2), nullptr, 16);
            } catch (...) {}

            // Helper: clamp and build a SolidColorBrush from RGB
            auto MakeRGB = [](int r, int g, int b) -> Windows::UI::Xaml::Media::SolidColorBrush^
            {
                Windows::UI::Color c;
                c.A = 255;
                c.R = (unsigned char)(r < 0 ? 0 : r > 255 ? 255 : r);
                c.G = (unsigned char)(g < 0 ? 0 : g > 255 ? 255 : g);
                c.B = (unsigned char)(b < 0 ? 0 : b > 255 ? 255 : b);
                return ref new Windows::UI::Xaml::Media::SolidColorBrush(c);
            };

            // Background: near-black deeply tinted with the accent hue
            auto canvas = ref new Windows::UI::Xaml::Controls::Canvas();
            canvas->Name = "XMBWaveCanvas";
            canvas->IsHitTestVisible = false;
            canvas->Background = MakeRGB((int)(aR * 0.06 + 4), (int)(aG * 0.06 + 4), (int)(aB * 0.06 + 12));
            Windows::UI::Xaml::Controls::Grid::SetRowSpan   (canvas, 99);
            Windows::UI::Xaml::Controls::Grid::SetColumnSpan(canvas, 99);

            // 6 wave polylines – brightness ramp from 8 % to 95 % of accent color
            double factors[] = { 0.08, 0.16, 0.30, 0.55, 0.78, 0.95 };
            double thick[]   = {  0.8,  1.0,  1.3,  2.2,  1.5,  0.8 };
            double opac[]    = { 0.40, 0.50, 0.65, 0.95, 0.75, 0.50 };
            for (int i = 0; i < 6; i++)
            {
                auto poly = ref new Windows::UI::Xaml::Shapes::Polyline();
                poly->Stroke          = MakeRGB((int)(aR * factors[i]),
                                                (int)(aG * factors[i]),
                                                (int)(aB * factors[i]));
                poly->StrokeThickness = thick[i];
                poly->Opacity         = opac[i];
                poly->StrokeLineJoin  = Windows::UI::Xaml::Media::PenLineJoin::Round;
                canvas->Children->Append(poly);
            }

            // 5 node ellipses – full accent color with a brighter white-mixed stroke
            for (int n = 0; n < 5; n++)
            {
                auto el = ref new Windows::UI::Xaml::Shapes::Ellipse();
                el->Width           = 7;
                el->Height          = 7;
                el->Fill            = MakeRGB(aR, aG, aB);
                el->Stroke          = MakeRGB((aR + 255) / 2, (aG + 255) / 2, (aB + 255) / 2);
                el->StrokeThickness = 1.5;
                el->Opacity         = 0.95;
                canvas->Children->Append(el);
            }

            rootGrid->Children->InsertAt(0, canvas);
            WaveCanvas() = canvas;
            WaveTime()   = 0.0;

            auto timer = ref new Windows::UI::Xaml::DispatcherTimer();
            Windows::Foundation::TimeSpan interval;
            interval.Duration = 16667; // ~60 fps in 100-ns ticks
            timer->Interval = interval;
            timer->Tick += ref new Windows::Foundation::EventHandler<Platform::Object^>(
                [canvas](Platform::Object^, Platform::Object^) { UpdateXMBWave(canvas); });
            timer->Start();
            WaveTimer() = timer;
        }


        // ───────────────────────────────────────────────────────────────────────

        static void ApplyThemeFontRecursive(Windows::UI::Xaml::DependencyObject^ obj, Windows::UI::Xaml::Media::FontFamily^ font)
        {
            if (obj == nullptr) return;

            auto control = dynamic_cast<Windows::UI::Xaml::Controls::Control^>(obj);
            if (control != nullptr)
            {
                if (font != nullptr) {
                    control->FontFamily = font;
                    control->FontWeight = Windows::UI::Text::FontWeights::Normal;
                } else {
                    control->ClearValue(Windows::UI::Xaml::Controls::Control::FontFamilyProperty);
                    control->ClearValue(Windows::UI::Xaml::Controls::Control::FontWeightProperty);
                }
            }

            auto textBlock = dynamic_cast<Windows::UI::Xaml::Controls::TextBlock^>(obj);
            if (textBlock != nullptr)
            {
                if (font != nullptr) {
                    textBlock->FontFamily = font;
                    textBlock->FontWeight = Windows::UI::Text::FontWeights::Normal;
                } else {
                    textBlock->ClearValue(Windows::UI::Xaml::Controls::TextBlock::FontFamilyProperty);
                    textBlock->ClearValue(Windows::UI::Xaml::Controls::TextBlock::FontWeightProperty);
                }
            }

            int count = Windows::UI::Xaml::Media::VisualTreeHelper::GetChildrenCount(obj);
            for (int i = 0; i < count; i++)
            {
                ApplyThemeFontRecursive(Windows::UI::Xaml::Media::VisualTreeHelper::GetChild(obj, i), font);
            }
        }

        // Applies the custom background image and theme font
        static void ApplyBackground(Windows::UI::Xaml::FrameworkElement^ rootElement)
        {
            if (rootElement == nullptr) return;

            // Apply theme font recursively for pixel themes
            int theme = GetAppTheme();
            if (theme == 5 || theme == 7)
            {
                auto pixelFont = ref new Windows::UI::Xaml::Media::FontFamily("ms-appx:///Assets/Fonts/Silkscreen.ttf#Silkscreen");
                ApplyThemeFontRecursive(rootElement, pixelFont);
            }
            else if (theme == 11)
            {
                auto localSettings = Windows::Storage::ApplicationData::Current->LocalSettings;
                if (localSettings->Values->HasKey("CustomFontPath"))
                {
                    auto fontPath = dynamic_cast<Platform::String^>(localSettings->Values->Lookup("CustomFontPath"));
                    if (fontPath != nullptr)
                    {
                        std::wstring pathStr(fontPath->Data());
                        size_t lastDot = pathStr.find_last_of(L".");
                        std::wstring familyName = (lastDot != std::wstring::npos) ? pathStr.substr(0, lastDot) : pathStr;
                        std::wstring fullPath = L"ms-appdata:///local/" + pathStr + L"#" + familyName;
                        auto customFont = ref new Windows::UI::Xaml::Media::FontFamily(ref new Platform::String(fullPath.c_str()));
                        ApplyThemeFontRecursive(rootElement, customFont);
                    }
                    else
                    {
                        ApplyThemeFontRecursive(rootElement, nullptr);
                    }
                }
                else
                {
                    ApplyThemeFontRecursive(rootElement, nullptr);
                }
            }
            else
            {
                ApplyThemeFontRecursive(rootElement, nullptr);
            }

            // Background logic usually applies to Panels or Controls
            auto rootPanel = dynamic_cast<Windows::UI::Xaml::Controls::Panel^>(rootElement);
            auto rootControl = dynamic_cast<Windows::UI::Xaml::Controls::Control^>(rootElement);
            
            auto localSettings = Windows::Storage::ApplicationData::Current->LocalSettings;
            
            Windows::UI::Xaml::Media::Brush^ targetBrush = nullptr;

            // 1. User's custom background image takes highest priority (except for RetroStation which is locked)
            if (theme != 9 && localSettings->Values->HasKey("CustomBackgroundPath"))
            {
                auto path = safe_cast<Platform::String^>(localSettings->Values->Lookup("CustomBackgroundPath"));
                if (path != nullptr && !path->IsEmpty())
                {
                    try {
                        auto uri = ref new Windows::Foundation::Uri("ms-appdata:///local/" + path);
                        auto bitmap = ref new Windows::UI::Xaml::Media::Imaging::BitmapImage(uri);
                        auto brush = ref new Windows::UI::Xaml::Media::ImageBrush();
                        brush->ImageSource = bitmap;
                        brush->Stretch = Windows::UI::Xaml::Media::Stretch::UniformToFill;
                        brush->Opacity = 1.0; 
                        targetBrush = brush;
                    } catch (...) {}
                }
            }

            // 2. If no custom image, use theme-specific images or colors
            if (targetBrush == nullptr)
            {
                if (theme == 5) // Arcade Hero
                {
                    try {
                        auto uri = ref new Windows::Foundation::Uri("ms-appx:///Assets/Theme/ArcadeHero/bg.png");
                        auto bitmap = ref new Windows::UI::Xaml::Media::Imaging::BitmapImage(uri);
                        auto brush = ref new Windows::UI::Xaml::Media::ImageBrush();
                        brush->ImageSource = bitmap;
                        brush->Stretch = Windows::UI::Xaml::Media::Stretch::UniformToFill;
                        brush->Opacity = 1.0;
                        targetBrush = brush;
                    } catch (...) {}
                }
                else if (theme == 4) // Steam
                {
                    try {
                        auto uri = ref new Windows::Foundation::Uri("ms-appx:///Assets/Theme/Steam/bg.png");
                        auto bitmap = ref new Windows::UI::Xaml::Media::Imaging::BitmapImage(uri);
                        auto brush = ref new Windows::UI::Xaml::Media::ImageBrush();
                        brush->ImageSource = bitmap;
                        brush->Stretch = Windows::UI::Xaml::Media::Stretch::UniformToFill;
                        brush->Opacity = 1.0;
                        targetBrush = brush;
                    } catch (...) {}
                }
                else if (theme == 1) // Light
                {
                    targetBrush = ref new Windows::UI::Xaml::Media::SolidColorBrush(Windows::UI::Colors::White);
                }
                else if (theme == 2) // Dark Gray
                {
                    targetBrush = BrushFromHex("#121212");
                }
                else if (theme == 3) // AMOLED
                {
                    targetBrush = ref new Windows::UI::Xaml::Media::SolidColorBrush(Windows::UI::Colors::Black);
                }
                else if (theme == 6) // Netflix
                {
                    try {
                        auto uri = ref new Windows::Foundation::Uri("ms-appx:///Assets/Theme/Netflix/bg.png");
                        auto bitmap = ref new Windows::UI::Xaml::Media::Imaging::BitmapImage(uri);
                        auto brush = ref new Windows::UI::Xaml::Media::ImageBrush();
                        brush->ImageSource = bitmap;
                        brush->Stretch = Windows::UI::Xaml::Media::Stretch::UniformToFill;
                        brush->Opacity = 1.0;
                        targetBrush = brush;
                    } catch (...) {}
                }
                else if (theme == 7) // Retro Boy
                {
                    targetBrush = BrushFromHex("#E0EECB");
                }
                else if (theme == 8) // Hedgehog
                {
                    try {
                        auto uri = ref new Windows::Foundation::Uri("ms-appx:///Assets/Theme/Hedgehog/bg.png");
                        auto bitmap = ref new Windows::UI::Xaml::Media::Imaging::BitmapImage(uri);
                        auto brush = ref new Windows::UI::Xaml::Media::ImageBrush();
                        brush->ImageSource = bitmap;
                        brush->Stretch = Windows::UI::Xaml::Media::Stretch::UniformToFill;
                        brush->Opacity = 1.0;
                        targetBrush = brush;
                    } catch (...) {}
                }
                else if (theme == 9) // RetroStation
                {
                    targetBrush = ref new Windows::UI::Xaml::Media::SolidColorBrush(Windows::UI::Colors::Transparent);
                }
                else if (theme == 10) // Blade
                {
                    try {
                        auto uri = ref new Windows::Foundation::Uri("ms-appx:///Assets/Theme/Blade/bg.png");
                        auto bitmap = ref new Windows::UI::Xaml::Media::Imaging::BitmapImage(uri);
                        auto brush = ref new Windows::UI::Xaml::Media::ImageBrush();
                        brush->ImageSource = bitmap;
                        brush->Stretch = Windows::UI::Xaml::Media::Stretch::UniformToFill;
                        brush->Opacity = 1.0;
                        targetBrush = brush;
                    } catch (...) {}
                }
            }

            if (targetBrush != nullptr)
            {
                if (rootPanel != nullptr) rootPanel->Background = targetBrush;
                if (rootControl != nullptr) rootControl->Background = targetBrush;
            }
            else
            {
                // Clear locally set backgrounds to allow XAML/ThemeResource backgrounds to show
                if (rootPanel != nullptr) rootPanel->ClearValue(Windows::UI::Xaml::Controls::Panel::BackgroundProperty);
                if (rootControl != nullptr) rootControl->ClearValue(Windows::UI::Xaml::Controls::Control::BackgroundProperty);
            }
        }

        static void ApplyThemeBorder(Windows::UI::Xaml::Controls::Grid^ rootGrid)
        {
            if (rootGrid == nullptr) return;
            
            // Look for existing border
            Windows::UI::Xaml::Controls::Border^ border = nullptr;
            for (unsigned int i = 0; i < rootGrid->Children->Size; i++)
            {
                auto child = rootGrid->Children->GetAt(i);
                auto b = dynamic_cast<Windows::UI::Xaml::Controls::Border^>(child);
                if (b != nullptr && b->Name == "AppBorder")
                {
                    border = b;
                    break;
                }
            }

            int theme = GetAppTheme();
            if (theme == 7) // Retro Boy
            {
                // Set the outermost background to Black to fill the rounded corner gaps
                rootGrid->Background = ref new Windows::UI::Xaml::Media::SolidColorBrush(Windows::UI::Colors::Black);

                if (border == nullptr)
                {
                    border = ref new Windows::UI::Xaml::Controls::Border();
                    border->Name = "AppBorder";
                    border->IsHitTestVisible = false; // Don't block clicks
                    // Ensure it spans the entire grid
                    Windows::UI::Xaml::Controls::Grid::SetRowSpan(border, 99);
                    Windows::UI::Xaml::Controls::Grid::SetColumnSpan(border, 99);
                    rootGrid->Children->Append(border);
                }
                border->BorderThickness = Windows::UI::Xaml::Thickness(10);
                border->CornerRadius = Windows::UI::Xaml::CornerRadius(20);
                border->BorderBrush = BrushFromHex("#738659");
                border->Visibility = Windows::UI::Xaml::Visibility::Visible;
                
                Windows::UI::Xaml::Controls::Border^ bgBorder = nullptr;
                for (unsigned int i = 0; i < rootGrid->Children->Size; i++)
                {
                    auto b = dynamic_cast<Windows::UI::Xaml::Controls::Border^>(rootGrid->Children->GetAt(i));
                    if (b != nullptr && b->Name == "AppBackgroundBorder") { bgBorder = b; break; }
                }

                if (bgBorder == nullptr)
                {
                    bgBorder = ref new Windows::UI::Xaml::Controls::Border();
                    bgBorder->Name = "AppBackgroundBorder";
                    bgBorder->IsHitTestVisible = false;
                    Windows::UI::Xaml::Controls::Grid::SetRowSpan(bgBorder, 99);
                    Windows::UI::Xaml::Controls::Grid::SetColumnSpan(bgBorder, 99);
                    rootGrid->Children->InsertAt(0, bgBorder);
                }

                // Prioritize custom background
                auto localSettings = Windows::Storage::ApplicationData::Current->LocalSettings;
                Windows::UI::Xaml::Media::Brush^ customBrush = nullptr;
                if (localSettings->Values->HasKey("CustomBackgroundPath"))
                {
                    auto path = safe_cast<Platform::String^>(localSettings->Values->Lookup("CustomBackgroundPath"));
                    if (path != nullptr && !path->IsEmpty())
                    {
                        try {
                            auto uri = ref new Windows::Foundation::Uri("ms-appdata:///local/" + path);
                            auto bitmap = ref new Windows::UI::Xaml::Media::Imaging::BitmapImage(uri);
                            auto brush = ref new Windows::UI::Xaml::Media::ImageBrush();
                            brush->ImageSource = bitmap;
                            brush->Stretch = Windows::UI::Xaml::Media::Stretch::UniformToFill;
                            customBrush = brush;
                        } catch (...) {}
                    }
                }

                bgBorder->Background = (customBrush != nullptr) ? customBrush : BrushFromHex("#E0EECB");
                bgBorder->CornerRadius = Windows::UI::Xaml::CornerRadius(20);
                bgBorder->Visibility = Windows::UI::Xaml::Visibility::Visible;
                
                // Frame border should have no background
                border->Background = nullptr;
            }
            else if (theme == 8) // Hedgehog
            {
                // Set the outermost background to Black to fill the rounded corner gaps
                rootGrid->Background = ref new Windows::UI::Xaml::Media::SolidColorBrush(Windows::UI::Colors::Black);

                if (border == nullptr)
                {
                    border = ref new Windows::UI::Xaml::Controls::Border();
                    border->Name = "AppBorder";
                    border->IsHitTestVisible = false;
                    Windows::UI::Xaml::Controls::Grid::SetRowSpan(border, 99);
                    Windows::UI::Xaml::Controls::Grid::SetColumnSpan(border, 99);
                    rootGrid->Children->Append(border);
                }

                border->Visibility = Windows::UI::Xaml::Visibility::Visible;
                border->Background = nullptr;
                border->Child = nullptr; // Clear previous nested borders

                // Outer Blue Stripe (6px)
                border->BorderThickness = Windows::UI::Xaml::Thickness(6);
                border->CornerRadius = Windows::UI::Xaml::CornerRadius(30);
                border->BorderBrush = BrushFromHex("#004d9c");

                // Middle White Line (3px)
                auto whiteBorder = ref new Windows::UI::Xaml::Controls::Border();
                whiteBorder->BorderThickness = Windows::UI::Xaml::Thickness(3);
                whiteBorder->CornerRadius = Windows::UI::Xaml::CornerRadius(24);
                whiteBorder->BorderBrush = ref new Windows::UI::Xaml::Media::SolidColorBrush(Windows::UI::Colors::White);
                
                // Inner Blue Stripe (6px)
                auto innerBlueBorder = ref new Windows::UI::Xaml::Controls::Border();
                innerBlueBorder->BorderThickness = Windows::UI::Xaml::Thickness(6);
                innerBlueBorder->CornerRadius = Windows::UI::Xaml::CornerRadius(21);
                innerBlueBorder->BorderBrush = BrushFromHex("#004d9c");

                whiteBorder->Child = innerBlueBorder;
                border->Child = whiteBorder;

                // Add the background border to show the striped background inside the rounded edges
                Windows::UI::Xaml::Controls::Border^ bgBorder = nullptr;
                for (unsigned int i = 0; i < rootGrid->Children->Size; i++)
                {
                    auto b = dynamic_cast<Windows::UI::Xaml::Controls::Border^>(rootGrid->Children->GetAt(i));
                    if (b != nullptr && b->Name == "AppBackgroundBorder") { bgBorder = b; break; }
                }

                if (bgBorder == nullptr)
                {
                    bgBorder = ref new Windows::UI::Xaml::Controls::Border();
                    bgBorder->Name = "AppBackgroundBorder";
                    bgBorder->IsHitTestVisible = false;
                    Windows::UI::Xaml::Controls::Grid::SetRowSpan(bgBorder, 99);
                    Windows::UI::Xaml::Controls::Grid::SetColumnSpan(bgBorder, 99);
                    rootGrid->Children->InsertAt(0, bgBorder);
                }

                // Prioritize custom background
                auto localSettings = Windows::Storage::ApplicationData::Current->LocalSettings;
                Windows::UI::Xaml::Media::Brush^ customBrush = nullptr;
                if (localSettings->Values->HasKey("CustomBackgroundPath"))
                {
                    auto path = safe_cast<Platform::String^>(localSettings->Values->Lookup("CustomBackgroundPath"));
                    if (path != nullptr && !path->IsEmpty())
                    {
                        try {
                            auto uri = ref new Windows::Foundation::Uri("ms-appdata:///local/" + path);
                            auto bitmap = ref new Windows::UI::Xaml::Media::Imaging::BitmapImage(uri);
                            auto brush = ref new Windows::UI::Xaml::Media::ImageBrush();
                            brush->ImageSource = bitmap;
                            brush->Stretch = Windows::UI::Xaml::Media::Stretch::UniformToFill;
                            customBrush = brush;
                        } catch (...) {}
                    }
                }

                if (customBrush != nullptr)
                {
                    bgBorder->Background = customBrush;
                }
                else
                {
                    // Load Hedgehog background
                    try {
                        auto uri = ref new Windows::Foundation::Uri("ms-appx:///Assets/Theme/Hedgehog/bg.png");
                        auto bitmap = ref new Windows::UI::Xaml::Media::Imaging::BitmapImage(uri);
                        auto brush = ref new Windows::UI::Xaml::Media::ImageBrush();
                        brush->ImageSource = bitmap;
                        brush->Stretch = Windows::UI::Xaml::Media::Stretch::UniformToFill;
                        bgBorder->Background = brush;
                    } catch (...) {}
                }
                
                bgBorder->CornerRadius = Windows::UI::Xaml::CornerRadius(30);
                bgBorder->Visibility = Windows::UI::Xaml::Visibility::Visible;
            }
            else if (theme == 9) // RetroStation (Background is locked to XMB wave)
            {
                rootGrid->Background = ref new Windows::UI::Xaml::Media::SolidColorBrush(Windows::UI::Colors::Black);

                if (border != nullptr) border->Visibility = Windows::UI::Xaml::Visibility::Collapsed;

                ApplyRetroStationWave(rootGrid);
            }
            else if (theme == 10) // Blade – dark bg image + slim Xbox-green outer ring
            {
                StopRetroStationWave(rootGrid);
                rootGrid->Background = BrushFromHex("#0D0D0D");

                // Outer green ring (AppBorder)
                if (border == nullptr)
                {
                    border = ref new Windows::UI::Xaml::Controls::Border();
                    border->Name = "AppBorder";
                    border->IsHitTestVisible = false;
                    Windows::UI::Xaml::Controls::Grid::SetRowSpan   (border, 99);
                    Windows::UI::Xaml::Controls::Grid::SetColumnSpan(border, 99);
                    rootGrid->Children->Append(border);
                }
                border->Background      = nullptr;
                border->Child           = nullptr;
                border->BorderThickness = Windows::UI::Xaml::Thickness(2);
                border->CornerRadius    = Windows::UI::Xaml::CornerRadius(30);
                border->BorderBrush     = BrushFromHex("#52B043");
                border->Visibility      = Windows::UI::Xaml::Visibility::Visible;

                // Background image layer (AppBackgroundBorder)
                Windows::UI::Xaml::Controls::Border^ bgBorder = nullptr;
                for (unsigned int i = 0; i < rootGrid->Children->Size; i++)
                {
                    auto b = dynamic_cast<Windows::UI::Xaml::Controls::Border^>(rootGrid->Children->GetAt(i));
                    if (b != nullptr && b->Name == "AppBackgroundBorder") { bgBorder = b; break; }
                }
                if (bgBorder == nullptr)
                {
                    bgBorder = ref new Windows::UI::Xaml::Controls::Border();
                    bgBorder->Name = "AppBackgroundBorder";
                    bgBorder->IsHitTestVisible = false;
                    Windows::UI::Xaml::Controls::Grid::SetRowSpan(bgBorder, 99);
                    Windows::UI::Xaml::Controls::Grid::SetColumnSpan(bgBorder, 99);
                    rootGrid->Children->InsertAt(0, bgBorder);
                }

                // Prioritize custom background
                auto localSettings = Windows::Storage::ApplicationData::Current->LocalSettings;
                Windows::UI::Xaml::Media::Brush^ customBrush = nullptr;
                if (localSettings->Values->HasKey("CustomBackgroundPath"))
                {
                    auto path = safe_cast<Platform::String^>(localSettings->Values->Lookup("CustomBackgroundPath"));
                    if (path != nullptr && !path->IsEmpty())
                    {
                        try {
                            auto uri = ref new Windows::Foundation::Uri("ms-appdata:///local/" + path);
                            auto bitmap = ref new Windows::UI::Xaml::Media::Imaging::BitmapImage(uri);
                            auto brush = ref new Windows::UI::Xaml::Media::ImageBrush();
                            brush->ImageSource = bitmap;
                            brush->Stretch = Windows::UI::Xaml::Media::Stretch::UniformToFill;
                            customBrush = brush;
                        } catch (...) {}
                    }
                }

                if (customBrush != nullptr)
                {
                    bgBorder->Background = customBrush;
                }
                else
                {
                    try {
                        auto uri = ref new Windows::Foundation::Uri("ms-appx:///Assets/Theme/Blade/bg.png");
                        auto bitmap = ref new Windows::UI::Xaml::Media::Imaging::BitmapImage(uri);
                        auto brush = ref new Windows::UI::Xaml::Media::ImageBrush();
                        brush->ImageSource = bitmap;
                        brush->Stretch = Windows::UI::Xaml::Media::Stretch::UniformToFill;
                        bgBorder->Background = brush;
                    } catch (...) {
                        bgBorder->Background = BrushFromHex("#0D0D0D");
                    }
                }
                bgBorder->CornerRadius = Windows::UI::Xaml::CornerRadius(30);
                bgBorder->Visibility = Windows::UI::Xaml::Visibility::Visible;
            }
            else
            {
                // Reset background and stop any live animations
                StopRetroStationWave(rootGrid);
                rootGrid->ClearValue(Windows::UI::Xaml::Controls::Grid::BackgroundProperty);
                if (border != nullptr) border->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
                
                // Hide background border if it exists
                for (unsigned int i = 0; i < rootGrid->Children->Size; i++)
                {
                    auto b = dynamic_cast<Windows::UI::Xaml::Controls::Border^>(rootGrid->Children->GetAt(i));
                    if (b != nullptr && b->Name == "AppBackgroundBorder") 
                    {
                        b->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
                        break;
                    }
                }
            }
        }

        static void SetImageWithRetroBoyEffect(Windows::UI::Xaml::Controls::Image^ img, Platform::String^ url)
        {
            if (img == nullptr) return;
            if (url == nullptr || url->IsEmpty()) 
            {
                img->Source = nullptr;
                return;
            }
            
            int theme = GetAppTheme();
            if (theme != 7)
            {
                img->Source = ref new Windows::UI::Xaml::Media::Imaging::BitmapImage(ref new Windows::Foundation::Uri(url));
                return;
            }

            // Async processing for Retro Boy
            using namespace Windows::Web::Http;
            using namespace Windows::Graphics::Imaging;
            using namespace Windows::Storage::Streams;
            using namespace Windows::UI::Xaml::Media::Imaging;
            using namespace concurrency;
            using namespace Microsoft::WRL;

            auto uri = ref new Windows::Foundation::Uri(url);
            auto client = ref new HttpClient();

            create_task(client->GetAsync(uri)).then([](HttpResponseMessage^ response) {
                if (!response->IsSuccessStatusCode) throw ref new Platform::Exception(E_FAIL);
                return response->Content->ReadAsInputStreamAsync();
            }).then([](IInputStream^ stream) {
                auto randomStream = ref new InMemoryRandomAccessStream();
                return create_task(RandomAccessStream::CopyAsync(stream, randomStream)).then([randomStream](unsigned long long) {
                    randomStream->Seek(0);
                    return BitmapDecoder::CreateAsync(randomStream);
                });
            }).then([img](BitmapDecoder^ decoder) {
                uint32 width = decoder->PixelWidth;
                uint32 height = decoder->PixelHeight;
                
                return create_task(decoder->GetPixelDataAsync(
                    BitmapPixelFormat::Bgra8,
                    BitmapAlphaMode::Straight,
                    ref new BitmapTransform(),
                    ExifOrientationMode::IgnoreExifOrientation,
                    ColorManagementMode::DoNotColorManage
                )).then([img, width, height](PixelDataProvider^ provider) {
                    auto pixels = provider->DetachPixelData();
                    auto data = pixels->Data;

                    // 1. Downsample (Pixelate) - Factor of 2
                    int pixelSize = 2;
                    for (uint32 y = 0; y < height; y += pixelSize) {
                        for (uint32 x = 0; x < width; x += pixelSize) {
                            int idx = (y * width + x) * 4;
                            byte b = data[idx];
                            byte g = data[idx+1];
                            byte r = data[idx+2];
                            byte a = data[idx+3];

                            for (int yy = 0; yy < pixelSize && (y + yy) < height; yy++) {
                                for (int xx = 0; xx < pixelSize && (x + xx) < width; xx++) {
                                    int tidx = ((y + yy) * width + (x + xx)) * 4;
                                    data[tidx] = b; data[tidx+1] = g; data[tidx+2] = r; data[tidx+3] = a;
                                }
                            }
                        }
                    }

                    // 2. Grayscale & Palette Map
                    for (uint32 i = 0; i < width * height * 4; i += 4) {
                        byte b = data[i];
                        byte g = data[i+1];
                        byte r = data[i+2];
                        byte a = data[i+3];

                        if (a < 128) continue; // Keep transparent

                        float lum = (r * 0.299f + g * 0.587f + b * 0.114f);

                        // #0f380f (15, 56, 15)
                        // #306230 (48, 98, 48)
                        // #8bac0f (139, 172, 15)
                        // #9bbc0f (155, 188, 15)

                        if (lum < 64) { r=15; g=56; b=15; }
                        else if (lum < 128) { r=48; g=98; b=48; }
                        else if (lum < 192) { r=139; g=172; b=15; }
                        else { r=155; g=188; b=15; }

                        data[i] = b; data[i+1] = g; data[i+2] = r; data[i+3] = 255;
                    }

                    auto dispatcher = img->Dispatcher;
                    dispatcher->RunAsync(Windows::UI::Core::CoreDispatcherPriority::Normal, ref new Windows::UI::Core::DispatchedHandler([img, width, height, pixels]() {
                        auto wb = ref new WriteableBitmap(width, height);
                        auto buffer = wb->PixelBuffer;
                        
                        ComPtr<IBufferByteAccess> bufferByteAccess;
                        reinterpret_cast<IInspectable*>(buffer)->QueryInterface(IID_PPV_ARGS(&bufferByteAccess));
                        byte* outPixels = nullptr;
                        bufferByteAccess->Buffer(&outPixels);
                        
                        memcpy(outPixels, pixels->Data, pixels->Length);
                        wb->Invalidate();
                        img->Source = wb;
                    }));
                });
            }).then([](task<void> prevTask) {
                try { prevTask.get(); } catch (...) {} // Ignore errors (e.g. invalid URLs)
            });
        }

        static void SetImageBrushWithRetroBoyEffect(Windows::UI::Xaml::Media::ImageBrush^ brush, Platform::String^ url)
        {
            if (brush == nullptr) return;
            if (url == nullptr || url->IsEmpty()) 
            {
                brush->ImageSource = nullptr;
                return;
            }
            
            int theme = GetAppTheme();
            if (theme != 7)
            {
                brush->ImageSource = ref new Windows::UI::Xaml::Media::Imaging::BitmapImage(ref new Windows::Foundation::Uri(url));
                return;
            }

            // Async processing for Retro Boy
            using namespace Windows::Web::Http;
            using namespace Windows::Graphics::Imaging;
            using namespace Windows::Storage::Streams;
            using namespace Windows::UI::Xaml::Media::Imaging;
            using namespace concurrency;
            using namespace Microsoft::WRL;

            auto uri = ref new Windows::Foundation::Uri(url);
            auto client = ref new HttpClient();

            create_task(client->GetAsync(uri)).then([](HttpResponseMessage^ response) {
                if (!response->IsSuccessStatusCode) throw ref new Platform::Exception(E_FAIL);
                return response->Content->ReadAsInputStreamAsync();
            }).then([](IInputStream^ stream) {
                auto randomStream = ref new InMemoryRandomAccessStream();
                return create_task(RandomAccessStream::CopyAsync(stream, randomStream)).then([randomStream](unsigned long long) {
                    randomStream->Seek(0);
                    return BitmapDecoder::CreateAsync(randomStream);
                });
            }).then([brush](BitmapDecoder^ decoder) {
                uint32 width = decoder->PixelWidth;
                uint32 height = decoder->PixelHeight;
                
                return create_task(decoder->GetPixelDataAsync(
                    BitmapPixelFormat::Bgra8,
                    BitmapAlphaMode::Straight,
                    ref new BitmapTransform(),
                    ExifOrientationMode::IgnoreExifOrientation,
                    ColorManagementMode::DoNotColorManage
                )).then([brush, width, height](PixelDataProvider^ provider) {
                    auto pixels = provider->DetachPixelData();
                    auto data = pixels->Data;

                    int pixelSize = 2;
                    for (uint32 y = 0; y < height; y += pixelSize) {
                        for (uint32 x = 0; x < width; x += pixelSize) {
                            int idx = (y * width + x) * 4;
                            byte b = data[idx];
                            byte g = data[idx+1];
                            byte r = data[idx+2];
                            byte a = data[idx+3];

                            for (int yy = 0; yy < pixelSize && (y + yy) < height; yy++) {
                                for (int xx = 0; xx < pixelSize && (x + xx) < width; xx++) {
                                    int tidx = ((y + yy) * width + (x + xx)) * 4;
                                    data[tidx] = b; data[tidx+1] = g; data[tidx+2] = r; data[tidx+3] = a;
                                }
                            }
                        }
                    }

                    for (uint32 i = 0; i < width * height * 4; i += 4) {
                        byte b = data[i];
                        byte g = data[i+1];
                        byte r = data[i+2];
                        byte a = data[i+3];

                        if (a < 128) continue;

                        float lum = (r * 0.299f + g * 0.587f + b * 0.114f);

                        if (lum < 64) { r=15; g=56; b=15; }
                        else if (lum < 128) { r=48; g=98; b=48; }
                        else if (lum < 192) { r=139; g=172; b=15; }
                        else { r=155; g=188; b=15; }

                        data[i] = b; data[i+1] = g; data[i+2] = r; data[i+3] = 255;
                    }

                    auto dispatcher = brush->Dispatcher;
                    dispatcher->RunAsync(Windows::UI::Core::CoreDispatcherPriority::Normal, ref new Windows::UI::Core::DispatchedHandler([brush, width, height, pixels]() {
                        auto wb = ref new WriteableBitmap(width, height);
                        auto buffer = wb->PixelBuffer;
                        
                        ComPtr<IBufferByteAccess> bufferByteAccess;
                        reinterpret_cast<IInspectable*>(buffer)->QueryInterface(IID_PPV_ARGS(&bufferByteAccess));
                        byte* outPixels = nullptr;
                        bufferByteAccess->Buffer(&outPixels);
                        
                        memcpy(outPixels, pixels->Data, pixels->Length);
                        wb->Invalidate();
                        brush->ImageSource = wb;
                    }));
                });
            }).then([](task<void> prevTask) {
                try { prevTask.get(); } catch (...) {}
            });
        }
        static void RestartApp()
        {
            Windows::ApplicationModel::Core::CoreApplication::RequestRestartAsync("");
        }

        static Platform::String^ GetPointsSuffix()
        {
            return (GetAppTheme() == 8) ? " Rings" : " Points";
        }
    };
}

