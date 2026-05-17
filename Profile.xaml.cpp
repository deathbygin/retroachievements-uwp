// Profile.xaml.cpp
// Implementation of the Profile class.
//

#include "pch.h"
#include "Profile.xaml.h"
#include "MainPage.xaml.h"
#include "GameInfoPage.xaml.h"
#include "AllGamesPage.xaml.h"
#include "PointsShop.xaml.h"
#include <ppltasks.h>
#include <string>
#include <sstream>
#include <vector>
#include <functional>
#include "ThemeSettings.h"

using namespace RetroAchievements_UWP;

using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Media::Imaging;
using namespace Windows::Web::Http;
using namespace Windows::UI::Core;
using namespace concurrency;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::Storage;
using namespace Windows::Storage::Pickers;

// The Profile page

Profile::Profile()
{
    InitializeComponent();
}

void Profile::OnNavigatedTo(Windows::UI::Xaml::Navigation::NavigationEventArgs^ e)
{
    // Get the MainPage reference passed as parameter
    MainPage^ mainPage = dynamic_cast<MainPage^>(e->Parameter);
    if (mainPage != nullptr)
    {
        this->ApiKey = mainPage->ApiKey;
        this->Username = mainPage->Username;
        FetchUserProfile();
        FetchRecentlyPlayedGames();
        FetchRecentAchievements();
        FetchWantToPlayGames();
    }

    auto localSettings = ApplicationData::Current->LocalSettings;
    if (localSettings->Values->HasKey("DisplayHardcoreStats"))
    {
        HardcoreModeToggle->IsOn = safe_cast<bool>(localSettings->Values->Lookup("DisplayHardcoreStats"));
    }

    // Load theme setting
    ThemeSelector->SelectedIndex = ThemeSettings::GetAppTheme();

    // Load active avatar frame
    auto settings = ApplicationData::Current->LocalSettings->Values;
    if (settings->HasKey("ActiveAvatarFrame"))
    {
        auto activeFrameUrl = dynamic_cast<Platform::String^>(settings->Lookup("ActiveAvatarFrame"));
        if (activeFrameUrl != nullptr)
        {
            AvatarFrameImage->Source = ref new BitmapImage(ref new Uri(activeFrameUrl));
        }
    }
    else
    {
        AvatarFrameImage->Source = nullptr;
    }
    
    ApplyThemeSettings();

    // Load available accent colors for the picker
    AccentColorGridView->ItemsSource = ThemeSettings::GetAvailableAccentColors();

    // Set focus to the ProfilePivot to enable controller navigation immediately
    ProfilePivot->Focus(Windows::UI::Xaml::FocusState::Programmatic);
}

void Profile::FetchUserProfile()
{
    if (this->ApiKey == nullptr || this->Username == nullptr)
    {
        UserTextBlock->Text = "Error: Missing credentials";
        return;
    }

    // Build the API URL for get-user-profile
    // https://api-docs.retroachievements.org/v1/get-user-profile.html
    std::wstring baseUrl = L"https://retroachievements.org/API/API_GetUserProfile.php?";
    std::wstring fullUrl = baseUrl;
    fullUrl += L"z=" + std::wstring(this->ApiKey->Data());
    fullUrl += L"&y=" + std::wstring(this->ApiKey->Data());
    fullUrl += L"&u=" + std::wstring(this->Username->Data());

    auto uri = ref new Windows::Foundation::Uri(ref new Platform::String(fullUrl.c_str()));
    HttpClient^ client = ref new HttpClient();

    create_task(client->GetAsync(uri)).then([this](task<HttpResponseMessage^> prevTask)
    {
        try
        {
            HttpResponseMessage^ response = prevTask.get();
            if (response->IsSuccessStatusCode)
            {
                create_task(response->Content->ReadAsStringAsync()).then([this](Platform::String^ body)
                {
                    // Parse the JSON response to extract fields
                    std::wstring responseBody(body->Data());

                    // Helper lambda to extract string value from JSON
                    auto extractJsonString = [](const std::wstring& json, const std::wstring& key) -> std::wstring
                    {
                        size_t keyPos = json.find(L"\"" + key + L"\"");
                        if (keyPos != std::wstring::npos)
                        {
                            size_t colonPos = json.find(L":", keyPos);
                            size_t quotePos = json.find(L"\"", colonPos);
                            if (quotePos != std::wstring::npos)
                            {
                                std::wstring result;
                                size_t i = quotePos + 1;
                                while (i < json.length() && json[i] != L'\"')
                                {
                                    if (json[i] == L'\\' && i + 1 < json.length())
                                    {
                                        wchar_t next = json[i + 1];
                                        if (next == L'\"') result += L'\"';
                                        else if (next == L'\\') result += L'\\';
                                        else if (next == L'/') result += L'/';
                                        else if (next == L'b') result += L'\b';
                                        else if (next == L'f') result += L'\f';
                                        else if (next == L'n') result += L'\n';
                                        else if (next == L'r') result += L'\r';
                                        else if (next == L't') result += L'\t';
                                        else if (next == L'u' && i + 5 < json.length())
                                        {
                                            std::wstring hexStr = json.substr(i + 2, 4);
                                            try {
                                                wchar_t wc = (wchar_t)std::stoi(hexStr, nullptr, 16);
                                                result += wc;
                                            } catch (...) {}
                                            i += 4;
                                        } else {
                                            result += next;
                                        }
                                        i += 2;
                                    }
                                    else
                                    {
                                        result += json[i];
                                        i++;
                                    }
                                }

                                // Sometimes the API literally contains "\/" even after unescaping
                                size_t pos = 0;
                                while ((pos = result.find(L"\\/", pos)) != std::wstring::npos) {
                                    result.replace(pos, 2, L"/");
                                }

                                return result;
                            }
                        }
                        return L"";
                    };

                    // Helper lambda to extract numeric value from JSON
                    auto extractJsonNumber = [](const std::wstring& json, const std::wstring& key) -> std::wstring
                    {
                        size_t keyPos = json.find(L"\"" + key + L"\"");
                        if (keyPos != std::wstring::npos)
                        {
                            size_t colonPos = json.find(L":", keyPos);
                            size_t valueStart = json.find_first_not_of(L" \t", colonPos + 1);
                            size_t valueEnd = valueStart;
                            while (valueEnd < json.length() && (iswdigit(json[valueEnd]) || json[valueEnd] == L'.'))
                            {
                                valueEnd++;
                            }
                            if (valueStart != std::wstring::npos && valueEnd > valueStart)
                            {
                                return json.substr(valueStart, valueEnd - valueStart);
                            }
                        }
                        return L"";
                    };

                    std::wstring username = extractJsonString(responseBody, L"User");
                    std::wstring memberSince = extractJsonString(responseBody, L"MemberSince");
                    std::wstring motto = extractJsonString(responseBody, L"Motto");
                    std::wstring totalPoints = extractJsonNumber(responseBody, L"TotalPoints");
                    std::wstring userPic = extractJsonString(responseBody, L"UserPic");

                    auto dispatcher = this->Dispatcher;
                    dispatcher->RunAsync(CoreDispatcherPriority::Normal, ref new DispatchedHandler([this, username, memberSince, motto, totalPoints, userPic]() {
                        UserTextBlock->Text = ref new Platform::String(username.c_str());
                        MemberSinceTextBlock->Text = ref new Platform::String(memberSince.c_str());
                        MottoTextBlock->Text = ref new Platform::String(motto.c_str());
                        TotalPointsTextBlock->Text = ref new Platform::String(totalPoints.c_str());

                        if (!userPic.empty())
                        {
                            std::wstring imageUrl = L"https://retroachievements.org" + userPic;
                            auto url = ref new Platform::String(imageUrl.c_str());
                            this->UserPicUrl = url;
                            ProfileImageBrush->ImageSource = ref new Windows::UI::Xaml::Media::Imaging::BitmapImage(ref new Windows::Foundation::Uri(url));
                        }

                        // Handle Arcade Hero Theme visibility based on points (250 threshold)
                        try {
                            int points = _wtoi(totalPoints.c_str());
                            auto arcadeHeroItem = dynamic_cast<Windows::UI::Xaml::Controls::ComboBoxItem^>(ThemeSelector->Items->GetAt(5));
                            if (arcadeHeroItem != nullptr)
                            {
                                arcadeHeroItem->Visibility = (points >= 250) ? Windows::UI::Xaml::Visibility::Visible : Windows::UI::Xaml::Visibility::Collapsed;
                            }

                            // Handle Steam Theme visibility based on points (500 threshold)
                            auto steamItem = dynamic_cast<Windows::UI::Xaml::Controls::ComboBoxItem^>(ThemeSelector->Items->GetAt(4));
                            if (steamItem != nullptr)
                            {
                                steamItem->Visibility = (points >= 500) ? Windows::UI::Xaml::Visibility::Visible : Windows::UI::Xaml::Visibility::Collapsed;
                            }

                            // Handle Netflix Theme visibility based on points (800 threshold)
                            auto netflixItem = dynamic_cast<Windows::UI::Xaml::Controls::ComboBoxItem^>(ThemeSelector->Items->GetAt(6));
                            if (netflixItem != nullptr)
                            {
                                netflixItem->Visibility = (points >= 800) ? Windows::UI::Xaml::Visibility::Visible : Windows::UI::Xaml::Visibility::Collapsed;
                            }

                            // Handle Retro Boy Theme visibility based on points (1100 threshold)
                            auto retroBoyItem = dynamic_cast<Windows::UI::Xaml::Controls::ComboBoxItem^>(ThemeSelector->Items->GetAt(7));
                            if (retroBoyItem != nullptr)
                            {
                                retroBoyItem->Visibility = (points >= 1100) ? Windows::UI::Xaml::Visibility::Visible : Windows::UI::Xaml::Visibility::Collapsed;
                            }

                            // Handle Hedgehog Theme visibility based on points (1500 threshold)
                            auto hedgehogItem = dynamic_cast<Windows::UI::Xaml::Controls::ComboBoxItem^>(ThemeSelector->Items->GetAt(8));
                            if (hedgehogItem != nullptr)
                            {
                                hedgehogItem->Visibility = (points >= 1500) ? Windows::UI::Xaml::Visibility::Visible : Windows::UI::Xaml::Visibility::Collapsed;
                            }

                            // Handle RetroStation Theme visibility based on points (1900 threshold)
                            auto retroStationItem = dynamic_cast<Windows::UI::Xaml::Controls::ComboBoxItem^>(ThemeSelector->Items->GetAt(9));
                            if (retroStationItem != nullptr)
                            {
                                retroStationItem->Visibility = (points >= 1900) ? Windows::UI::Xaml::Visibility::Visible : Windows::UI::Xaml::Visibility::Collapsed;
                            }

                            // Handle Blade Theme visibility based on points (2400 threshold)
                            auto bladeItem = dynamic_cast<Windows::UI::Xaml::Controls::ComboBoxItem^>(ThemeSelector->Items->GetAt(10));
                            if (bladeItem != nullptr)
                            {
                                bladeItem->Visibility = (points >= 2400) ? Windows::UI::Xaml::Visibility::Visible : Windows::UI::Xaml::Visibility::Collapsed;
                            }

                            // Handle Custom Theme visibility based on points (3000 threshold)
                            auto customItem = dynamic_cast<Windows::UI::Xaml::Controls::ComboBoxItem^>(ThemeSelector->Items->GetAt(11));
                            if (customItem != nullptr)
                            {
                                customItem->Visibility = (points >= 3000) ? Windows::UI::Xaml::Visibility::Visible : Windows::UI::Xaml::Visibility::Collapsed;
                            }
                        } catch (...) {}
                    }));
                });
            }
            else
            {
                auto dispatcher = this->Dispatcher;
                dispatcher->RunAsync(CoreDispatcherPriority::Normal, ref new DispatchedHandler([this]() {
                    UserTextBlock->Text = "Error: Failed to fetch profile";
                }));
            }
        }
        catch (...)
        {
            auto dispatcher = this->Dispatcher;
            dispatcher->RunAsync(CoreDispatcherPriority::Normal, ref new DispatchedHandler([this]() {
                UserTextBlock->Text = "Error: Exception occurred";
            }));
        }
    });
}



void Profile::ClearCredentials_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    CredentialManager::GetInstance()->ClearCredentials();

    // Navigate back to MainPage
    this->Frame->Navigate(Windows::UI::Xaml::Interop::TypeName(MainPage::typeid), nullptr);
}

void Profile::FetchRecentAchievements()
{
    if (this->ApiKey == nullptr || this->Username == nullptr)
    {
        return;
    }

    // https://retroachievements.org/API/API_GetUserRecentAchievements.php
    std::wstring baseUrl = L"https://retroachievements.org/API/API_GetUserRecentAchievements.php?";
    std::wstring fullUrl = baseUrl;
    fullUrl += L"z=" + std::wstring(this->ApiKey->Data());
    fullUrl += L"&y=" + std::wstring(this->ApiKey->Data());
    fullUrl += L"&u=" + std::wstring(this->Username->Data());
    fullUrl += L"&m=525600"; // up to 1 year of recent achievements

    auto uri = ref new Windows::Foundation::Uri(ref new Platform::String(fullUrl.c_str()));
    HttpClient^ client = ref new HttpClient();

    create_task(client->GetAsync(uri)).then([this](task<HttpResponseMessage^> prevTask)
    {
        try
        {
            HttpResponseMessage^ response = prevTask.get();
            if (response->IsSuccessStatusCode)
            {
                create_task(response->Content->ReadAsStringAsync()).then([this](Platform::String^ body)
                {
                    std::wstring responseBody(body->Data());
                    
                    // Helper lambda to extract string value from JSON
                    auto extractJsonString = [](const std::wstring& json, const std::wstring& key) -> std::wstring {
                        size_t keyPos = json.find(L"\"" + key + L"\"");
                        if (keyPos != std::wstring::npos) {
                            size_t colonPos = json.find(L":", keyPos);
                            size_t quotePos = json.find(L"\"", colonPos);
                            if (quotePos != std::wstring::npos) {
                                std::wstring result;
                                size_t i = quotePos + 1;
                                while (i < json.length() && json[i] != L'\"') {
                                    if (json[i] == L'\\' && i + 1 < json.length()) {
                                        wchar_t next = json[i + 1];
                                        if (next == L'\"') result += L'\"';
                                        else if (next == L'\\') result += L'\\';
                                        else if (next == L'/') result += L'/';
                                        else if (next == L'b') result += L'\b';
                                        else if (next == L'f') result += L'\f';
                                        else if (next == L'n') result += L'\n';
                                        else if (next == L'r') result += L'\r';
                                        else if (next == L't') result += L'\t';
                                        else if (next == L'u' && i + 5 < json.length()) {
                                            std::wstring hexStr = json.substr(i + 2, 4);
                                            try {
                                                wchar_t wc = (wchar_t)std::stoi(hexStr, nullptr, 16);
                                                result += wc;
                                            } catch (...) {}
                                            i += 4;
                                        } else {
                                            result += next;
                                        }
                                        i += 2;
                                    } else {
                                        result += json[i];
                                        i++;
                                    }
                                }

                                // Sometimes the API literally contains "\/" even after unescaping
                                size_t pos = 0;
                                while ((pos = result.find(L"\\/", pos)) != std::wstring::npos) {
                                    result.replace(pos, 2, L"/");
                                }

                                return result;
                            }
                        }
                        return L"";
                    };

                    // Helper lambda to extract numeric value from JSON
                    auto extractJsonNumber = [](const std::wstring& json, const std::wstring& key) -> std::wstring {
                        size_t keyPos = json.find(L"\"" + key + L"\"");
                        if (keyPos != std::wstring::npos) {
                            size_t colonPos = json.find(L":", keyPos);
                            size_t valueStart = json.find_first_not_of(L" \t", colonPos + 1);
                            size_t valueEnd = valueStart;
                            while (valueEnd < json.length() && (iswdigit(json[valueEnd]) || json[valueEnd] == L'.')) {
                                valueEnd++;
                            }
                            if (valueStart != std::wstring::npos && valueEnd > valueStart) {
                                return json.substr(valueStart, valueEnd - valueStart);
                            }
                        }
                        return L"";
                    };

                    // Get the hardcore setting once
                    bool useHardcoreSetting = false;
                    auto localSettings = Windows::Storage::ApplicationData::Current->LocalSettings;
                    if (localSettings->Values->HasKey("DisplayHardcoreStats"))
                    {
                        useHardcoreSetting = safe_cast<bool>(localSettings->Values->Lookup("DisplayHardcoreStats"));
                    }

                    std::vector<RecentAchievementItem^> itemsList;
                    size_t currentPos = 0;

                    // Parse achievement objects in the JSON array
                    while (currentPos < responseBody.length() && itemsList.size() < 8)
                    {
                        size_t objectStart = responseBody.find(L"{", currentPos);
                        if (objectStart == std::wstring::npos)
                            break;

                        size_t objectEnd = responseBody.find(L"}", objectStart);
                        if (objectEnd == std::wstring::npos)
                            break;

                        std::wstring achObj = responseBody.substr(objectStart, objectEnd - objectStart + 1);
                        
                        // Check if this achievement matches the hardcore setting
                        std::wstring hardcoreVal = extractJsonNumber(achObj, L"HardcoreMode");
                        if (hardcoreVal.empty()) hardcoreVal = extractJsonString(achObj, L"HardcoreMode");
                        bool isHardcore = (hardcoreVal == L"1" || hardcoreVal == L"true");

                        if (useHardcoreSetting && !isHardcore)
                        {
                            currentPos = objectEnd + 1;
                            continue;
                        }

                        std::wstring badgeName = extractJsonString(achObj, L"BadgeName");
                        std::wstring title = extractJsonString(achObj, L"Title");
                        std::wstring description = extractJsonString(achObj, L"Description");
                        std::wstring gameTitle = extractJsonString(achObj, L"GameTitle");
                        std::wstring gameId = extractJsonNumber(achObj, L"GameID");
                        if (gameId.empty()) gameId = extractJsonString(achObj, L"GameID");
                        std::wstring points = extractJsonNumber(achObj, L"Points");
                        if (points.empty()) points = extractJsonString(achObj, L"Points");
                        std::wstring date = extractJsonString(achObj, L"Date");

                        if (!badgeName.empty())
                        {
                            RecentAchievementItem^ item = ref new RecentAchievementItem();
                            item->ImageUrl = ref new Platform::String((L"https://retroachievements.org/Badge/" + badgeName + L".png").c_str());
                            item->Title = ref new Platform::String(title.c_str());
                            item->Description = ref new Platform::String(description.c_str());
                            item->GameTitle = ref new Platform::String(gameTitle.c_str());
                            item->GameID = ref new Platform::String(gameId.c_str());
                            item->Points = ref new Platform::String((points + ThemeSettings::GetPointsSuffix()->Data()).c_str());
                            item->DateEarnedText = ref new Platform::String((L"Date Earned: " + date).c_str());
                            itemsList.push_back(item);
                        }

                        currentPos = objectEnd + 1;
                    }

                    auto dispatcher = this->Dispatcher;
                    dispatcher->RunAsync(CoreDispatcherPriority::Normal, ref new DispatchedHandler([this, itemsList]() {
                        auto items = RecentAchievementsGridView->Items;
                        items->Clear();
                        for (auto item : itemsList)
                        {
                            items->Append(item);
                        }
                    }));
                });
            }
        }
        catch (...)
        {
            // Fail gracefully
        }
    });
}

void Profile::RecentAchievementsGridView_ItemClick(Platform::Object^ sender, Windows::UI::Xaml::Controls::ItemClickEventArgs^ e)
{
    RecentAchievementItem^ clickedAch = dynamic_cast<RecentAchievementItem^>(e->ClickedItem);
    if (clickedAch != nullptr)
    {
        // Create a GameItem to pass to GameInfoPage
        GameItem^ game = ref new GameItem();
        game->GameID = clickedAch->GameID;
        game->Title = clickedAch->GameTitle;
        // The GameInfoPage will fetch progress itself, but we can set some defaults if we want
        game->AchievementProgress = ""; 
        game->NumAchieved = 0;
        game->NumPossibleAchievements = 0;

        GameInfoNavParam^ param = ref new GameInfoNavParam();
        param->Game = game;
        param->Header = GetProfileHeaderData();

        this->Frame->Navigate(Windows::UI::Xaml::Interop::TypeName(GameInfoPage::typeid), param);
    }
}

void Profile::FetchRecentlyPlayedGames()
{
    if (this->ApiKey == nullptr || this->Username == nullptr)
    {
        return;
    }

    // Build the API URL for get-user-recently-played-games
    // https://api-docs.retroachievements.org/v1/get-user-recently-played-games.html
    std::wstring baseUrl = L"https://retroachievements.org/API/API_GetUserRecentlyPlayedGames.php?";
    std::wstring fullUrl = baseUrl;
    fullUrl += L"z=" + std::wstring(this->ApiKey->Data());
    fullUrl += L"&y=" + std::wstring(this->ApiKey->Data());
    fullUrl += L"&u=" + std::wstring(this->Username->Data());
    fullUrl += L"&c=16"; // Get the last 16 recently played games

    auto uri = ref new Windows::Foundation::Uri(ref new Platform::String(fullUrl.c_str()));
    HttpClient^ client = ref new HttpClient();

    create_task(client->GetAsync(uri)).then([this](task<HttpResponseMessage^> prevTask)
    {
        try
        {
            HttpResponseMessage^ response = prevTask.get();
            if (response->IsSuccessStatusCode)
            {
                create_task(response->Content->ReadAsStringAsync()).then([this](Platform::String^ body)
                {
                    std::wstring responseBody(body->Data());

                    // Helper lambda to extract string value from JSON
                    auto extractJsonString = [](const std::wstring& json, const std::wstring& key) -> std::wstring
                    {
                        size_t keyPos = json.find(L"\"" + key + L"\"");
                        if (keyPos != std::wstring::npos)
                        {
                            size_t colonPos = json.find(L":", keyPos);
                            size_t quotePos = json.find(L"\"", colonPos);
                            if (quotePos != std::wstring::npos)
                            {
                                std::wstring result;
                                size_t i = quotePos + 1;
                                while (i < json.length() && json[i] != L'\"')
                                {
                                    if (json[i] == L'\\' && i + 1 < json.length())
                                    {
                                        wchar_t next = json[i + 1];
                                        if (next == L'\"') result += L'\"';
                                        else if (next == L'\\') result += L'\\';
                                        else if (next == L'/') result += L'/';
                                        else if (next == L'b') result += L'\b';
                                        else if (next == L'f') result += L'\f';
                                        else if (next == L'n') result += L'\n';
                                        else if (next == L'r') result += L'\r';
                                        else if (next == L't') result += L'\t';
                                        else if (next == L'u' && i + 5 < json.length())
                                        {
                                            std::wstring hexStr = json.substr(i + 2, 4);
                                            try {
                                                wchar_t wc = (wchar_t)std::stoi(hexStr, nullptr, 16);
                                                result += wc;
                                            } catch (...) {}
                                            i += 4;
                                        } else {
                                            result += next;
                                        }
                                        i += 2;
                                    }
                                    else
                                    {
                                        result += json[i];
                                        i++;
                                    }
                                }

                                // Sometimes the API literally contains "\/" even after unescaping
                                size_t pos = 0;
                                while ((pos = result.find(L"\\/", pos)) != std::wstring::npos) {
                                    result.replace(pos, 2, L"/");
                                }

                                return result;
                            }
                        }
                        return L"";
                    };

                    // Helper lambda to extract numeric value from JSON
                    auto extractJsonNumber = [](const std::wstring& json, const std::wstring& key) -> std::wstring
                    {
                        size_t keyPos = json.find(L"\"" + key + L"\"");
                        if (keyPos != std::wstring::npos)
                        {
                            size_t colonPos = json.find(L":", keyPos);
                            size_t valueStart = json.find_first_not_of(L" \t", colonPos + 1);
                            size_t valueEnd = valueStart;
                            while (valueEnd < json.length() && (iswdigit(json[valueEnd]) || json[valueEnd] == L'.'))
                            {
                                valueEnd++;
                            }
                            if (valueStart != std::wstring::npos && valueEnd > valueStart)
                            {
                                return json.substr(valueStart, valueEnd - valueStart);
                            }
                        }
                        return L"";
                    };

                    // Parse the JSON array of games
                    // The API returns an array of game objects
                    std::vector<GameItem^> gameItems;
                    size_t currentPos = 0;

                    // Find all game objects in the array
                    while (currentPos < responseBody.length())
                    {
                        size_t objectStart = responseBody.find(L"{", currentPos);
                        if (objectStart == std::wstring::npos)
                            break;

                        size_t objectEnd = responseBody.find(L"}", objectStart);
                        if (objectEnd == std::wstring::npos)
                            break;

                        std::wstring gameObject = responseBody.substr(objectStart, objectEnd - objectStart + 1);
                        std::wstring gameID = extractJsonNumber(gameObject, L"GameID");
                        std::wstring imageIcon = extractJsonString(gameObject, L"ImageIcon");
                        std::wstring title = extractJsonString(gameObject, L"Title");
                        std::wstring consoleName = extractJsonString(gameObject, L"ConsoleName");
                        std::wstring numAchieved = extractJsonNumber(gameObject, L"NumAchieved");
                        std::wstring numPossible = extractJsonNumber(gameObject, L"NumPossibleAchievements");

                        if (!imageIcon.empty())
                        {
                            GameItem^ item = ref new GameItem();
                            item->GameID = ref new Platform::String(gameID.c_str());
                            item->ImageUrl = ref new Platform::String((L"https://retroachievements.org" + imageIcon).c_str());
                            item->Title = ref new Platform::String(title.c_str());
                            item->ConsoleName = ref new Platform::String(consoleName.c_str());
                            std::wstring progress = numAchieved + L"/" + numPossible + L" earned";
                            item->AchievementProgress = ref new Platform::String(progress.c_str());
                            
                            double achievedVal = 0.0;
                            if (!numAchieved.empty()) {
                                achievedVal = _wtof(numAchieved.c_str());
                            }
                            
                            double possibleVal = 1.0;
                            if (!numPossible.empty()) {
                                possibleVal = _wtof(numPossible.c_str());
                                if (possibleVal <= 0) possibleVal = 1.0;
                            }
                            
                            item->NumAchieved = achievedVal;
                            item->NumPossibleAchievements = possibleVal;
                            
                            gameItems.push_back(item);
                        }

                        currentPos = objectEnd + 1;
                    }

                    auto dispatcher = this->Dispatcher;
                    dispatcher->RunAsync(CoreDispatcherPriority::Normal, ref new DispatchedHandler([this, gameItems]() {
                        auto items = RecentGamesGridView->Items;
                        items->Clear();

                        for (auto item : gameItems)
                        {
                            items->Append(item);
                        }
                    }));
                });
            }
            else
            {
                auto dispatcher = this->Dispatcher;
                dispatcher->RunAsync(CoreDispatcherPriority::Normal, ref new DispatchedHandler([this]() {
                    auto items = RecentGamesGridView->Items;
                    items->Clear();
                }));
            }
        }
        catch (...)
        {
            auto dispatcher = this->Dispatcher;
            dispatcher->RunAsync(CoreDispatcherPriority::Normal, ref new DispatchedHandler([this]() {
                auto items = RecentGamesGridView->Items;
                items->Clear();
            }));
        }
    });
}

UserProfileHeaderData^ Profile::GetProfileHeaderData()
{
    UserProfileHeaderData^ header = ref new UserProfileHeaderData();
    header->ProfileImage = ProfileImageBrush->ImageSource;
    header->ProfileImageUrl = this->UserPicUrl;
    header->Username = UserTextBlock->Text;
    header->MemberSince = MemberSinceTextBlock->Text;
    header->Motto = MottoTextBlock->Text;
    header->TotalPoints = TotalPointsTextBlock->Text;
    return header;
}

void Profile::ViewAllGames_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    this->Frame->Navigate(Windows::UI::Xaml::Interop::TypeName(AllGamesPage::typeid), GetProfileHeaderData());
}

void Profile::NavigatePointsShop_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    this->Frame->Navigate(Windows::UI::Xaml::Interop::TypeName(PointsShop::typeid), GetProfileHeaderData());
}

void Profile::RecentGamesGridView_ItemClick(Platform::Object^ sender, Windows::UI::Xaml::Controls::ItemClickEventArgs^ e)
{
    GameItem^ clickedItem = dynamic_cast<GameItem^>(e->ClickedItem);
    if (clickedItem != nullptr)
    {
        GameInfoNavParam^ param = ref new GameInfoNavParam();
        param->Game = clickedItem;
        param->Header = GetProfileHeaderData();

        this->Frame->Navigate(Windows::UI::Xaml::Interop::TypeName(GameInfoPage::typeid), param);
    }
}

void Profile::FetchWantToPlayGames()
{
    if (this->ApiKey == nullptr || this->Username == nullptr)
    {
        return;
    }

    std::wstring baseUrl = L"https://retroachievements.org/API/API_GetUserWantToPlayList.php?";
    std::wstring fullUrl = baseUrl;
    fullUrl += L"z=" + std::wstring(this->ApiKey->Data());
    fullUrl += L"&y=" + std::wstring(this->ApiKey->Data());
    fullUrl += L"&u=" + std::wstring(this->Username->Data());

    auto uri = ref new Windows::Foundation::Uri(ref new Platform::String(fullUrl.c_str()));
    HttpClient^ client = ref new HttpClient();

    create_task(client->GetAsync(uri)).then([this](task<HttpResponseMessage^> prevTask)
    {
        try
        {
            HttpResponseMessage^ response = prevTask.get();
            if (response->IsSuccessStatusCode)
            {
                create_task(response->Content->ReadAsStringAsync()).then([this](Platform::String^ body)
                {
                    try
                    {
                        std::wstring responseBody(body->Data());
                        std::vector<GameItem^> games;

                        size_t currentPos = 0;
                        while (currentPos < responseBody.length())
                        {
                            size_t objectStart = responseBody.find(L"{", currentPos);
                            if (objectStart == std::wstring::npos) break;

                            size_t objectEnd = responseBody.find(L"}", objectStart);
                            if (objectEnd == std::wstring::npos) break;

                            std::wstring gameObj = responseBody.substr(objectStart, objectEnd - objectStart + 1);
                            
                            auto extractJsonString = [](const std::wstring& json, const std::wstring& key) -> std::wstring
                            {
                                size_t keyPos = json.find(L"\"" + key + L"\"");
                                if (keyPos != std::wstring::npos)
                                {
                                    size_t colonPos = json.find(L":", keyPos);
                                    size_t quotePos = json.find(L"\"", colonPos);
                                    if (quotePos != std::wstring::npos)
                                    {
                                        std::wstring result;
                                        size_t i = quotePos + 1;
                                        while (i < json.length() && json[i] != L'\"')
                                        {
                                            if (json[i] == L'\\' && i + 1 < json.length())
                                            {
                                                wchar_t next = json[i + 1];
                                                if (next == L'\"') result += L'\"';
                                                else if (next == L'\\') result += L'\\';
                                                else if (next == L'/') result += L'/';
                                                else if (next == L'b') result += L'\b';
                                                else if (next == L'f') result += L'\f';
                                                else if (next == L'n') result += L'\n';
                                                else if (next == L'r') result += L'\r';
                                                else if (next == L't') result += L'\t';
                                                else if (next == L'u' && i + 5 < json.length())
                                                {
                                                    std::wstring hexStr = json.substr(i + 2, 4);
                                                    try {
                                                        wchar_t wc = (wchar_t)std::stoi(hexStr, nullptr, 16);
                                                        result += wc;
                                                    } catch (...) {}
                                                    i += 4;
                                                } else {
                                                    result += next;
                                                }
                                                i += 2;
                                            }
                                            else
                                            {
                                                result += json[i];
                                                i++;
                                            }
                                        }
                                        
                                        // Sometimes the API literally contains "\/" even after unescaping
                                        size_t pos = 0;
                                        while ((pos = result.find(L"\\/", pos)) != std::wstring::npos) {
                                            result.replace(pos, 2, L"/");
                                        }
                                        
                                        return result;
                                    }
                                }
                                return L"";
                            };
                            
                            auto extractJsonNumber = [](const std::wstring& json, const std::wstring& key) -> std::wstring
                            {
                                size_t keyPos = json.find(L"\"" + key + L"\"");
                                if (keyPos != std::wstring::npos)
                                {
                                    size_t colonPos = json.find(L":", keyPos);
                                    size_t valueStart = json.find_first_of(L"0123456789-.", colonPos);
                                    if (valueStart != std::wstring::npos)
                                    {
                                        size_t valueEnd = json.find_first_not_of(L"0123456789-.", valueStart);
                                        if (valueEnd != std::wstring::npos)
                                        {
                                            return json.substr(valueStart, valueEnd - valueStart);
                                        }
                                        return json.substr(valueStart);
                                    }
                                }
                                return L"";
                            };

                            std::wstring idStr = extractJsonNumber(gameObj, L"ID");
                            if (idStr.empty()) idStr = extractJsonString(gameObj, L"ID");
                            std::wstring title = extractJsonString(gameObj, L"Title");
                            std::wstring consoleName = extractJsonString(gameObj, L"ConsoleName");
                            std::wstring imageIcon = extractJsonString(gameObj, L"ImageIcon");

                            if (!idStr.empty() && !title.empty())
                            {
                                GameItem^ item = ref new GameItem();
                                item->GameID = ref new Platform::String(idStr.c_str());
                                item->Title = ref new Platform::String(title.c_str());
                                item->ConsoleName = ref new Platform::String(consoleName.c_str());
                                
                                std::wstring imageUrl = L"https://retroachievements.org" + imageIcon;
                                item->ImageUrl = ref new Platform::String(imageUrl.c_str());

                                games.push_back(item);
                            }

                            currentPos = objectEnd + 1;
                        }

                        auto dispatcher = this->Dispatcher;
                        dispatcher->RunAsync(CoreDispatcherPriority::Normal, ref new DispatchedHandler([this, games]() {
                            auto items = WantToPlayGridView->Items;
                            items->Clear();
                            for (auto game : games)
                            {
                                items->Append(game);
                            }
                        }));
                    }
                    catch (...) {}
                });
            }
        }
        catch (...)
        {
            // Fail gracefully
        }
    });
}

void Profile::WantToPlayGridView_ItemClick(Platform::Object^ sender, Windows::UI::Xaml::Controls::ItemClickEventArgs^ e)
{
    GameItem^ clickedItem = dynamic_cast<GameItem^>(e->ClickedItem);
    if (clickedItem != nullptr)
    {
        GameInfoNavParam^ param = ref new GameInfoNavParam();
        param->Game = clickedItem;
        param->Header = GetProfileHeaderData();

        this->Frame->Navigate(Windows::UI::Xaml::Interop::TypeName(GameInfoPage::typeid), param);
    }
}

void Profile::HardcoreModeToggle_Toggled(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    auto localSettings = ApplicationData::Current->LocalSettings;
    localSettings->Values->Insert("DisplayHardcoreStats", HardcoreModeToggle->IsOn);
}

void Profile::ThemeSelector_SelectionChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::SelectionChangedEventArgs^ e)
{
    // Prevent restart loop on boot: Only restart if there was a previous selection being removed
    if (e->RemovedItems->Size == 0) return;

    int selectedIndex = ThemeSelector->SelectedIndex;
    if (selectedIndex < 0) return;

    auto localSettings = Windows::Storage::ApplicationData::Current->LocalSettings;
    localSettings->Values->Insert("AppTheme", selectedIndex);
    ThemeSettings::RestartApp();
}

void Profile::AccentColorGridView_ItemClick(Platform::Object^ sender, Windows::UI::Xaml::Controls::ItemClickEventArgs^ e)
{
    Platform::String^ hex = dynamic_cast<Platform::String^>(e->ClickedItem);
    if (hex != nullptr)
    {
        ThemeSettings::SetAccentColor(hex);
        ThemeSettings::RestartApp();
    }
}

void Profile::AccentColorGridView_ContainerContentChanging(Windows::UI::Xaml::Controls::ListViewBase^ sender, Windows::UI::Xaml::Controls::ContainerContentChangingEventArgs^ args)
{
    if (!args->InRecycleQueue)
    {
        Platform::String^ hex = dynamic_cast<Platform::String^>(args->Item);
        if (hex != nullptr)
        {
            auto rectangle = dynamic_cast<Windows::UI::Xaml::Shapes::Rectangle^>(args->ItemContainer->ContentTemplateRoot);
            if (rectangle != nullptr)
            {
                rectangle->Fill = ThemeSettings::BrushFromHex(hex);
                
                // Highlight the currently selected color
                if (hex == ThemeSettings::GetAccentColor())
                {
                    rectangle->StrokeThickness = 4;
                }
                else
                {
                    rectangle->StrokeThickness = 1;
                }
            }
        }
    }
}

void Profile::ChooseBackgroundImage_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    auto picker = ref new Windows::Storage::Pickers::FileOpenPicker();
    picker->ViewMode = Windows::Storage::Pickers::PickerViewMode::Thumbnail;
    picker->SuggestedStartLocation = Windows::Storage::Pickers::PickerLocationId::PicturesLibrary;
    picker->FileTypeFilter->Append(".jpg");
    picker->FileTypeFilter->Append(".jpeg");
    picker->FileTypeFilter->Append(".png");
    picker->FileTypeFilter->Append(".bmp");

    create_task(picker->PickSingleFileAsync()).then([this](Windows::Storage::StorageFile^ file)
    {
        if (file != nullptr)
        {
            auto localFolder = Windows::Storage::ApplicationData::Current->LocalFolder;
            create_task(file->CopyAsync(localFolder, "custom_background.jpg", Windows::Storage::NameCollisionOption::ReplaceExisting)).then([this](Windows::Storage::StorageFile^ copiedFile)
            {
                auto localSettings = Windows::Storage::ApplicationData::Current->LocalSettings;
                localSettings->Values->Insert("CustomBackgroundPath", ref new Platform::String(L"custom_background.jpg"));
                
                this->Dispatcher->RunAsync(Windows::UI::Core::CoreDispatcherPriority::Normal, ref new Windows::UI::Core::DispatchedHandler([this]() {
                    ApplyThemeSettings();
                }));
            });
        }
    });
}

void Profile::ClearBackgroundImage_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    auto localSettings = Windows::Storage::ApplicationData::Current->LocalSettings;
    if (localSettings->Values->HasKey("CustomBackgroundPath"))
    {
        localSettings->Values->Remove("CustomBackgroundPath");
        ApplyThemeSettings();
    }
}

void Profile::ChooseFontButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    auto picker = ref new Windows::Storage::Pickers::FileOpenPicker();
    picker->ViewMode = Windows::Storage::Pickers::PickerViewMode::List;
    picker->SuggestedStartLocation = Windows::Storage::Pickers::PickerLocationId::DocumentsLibrary;
    picker->FileTypeFilter->Append(".ttf");
    picker->FileTypeFilter->Append(".otf");

    create_task(picker->PickSingleFileAsync()).then([this](Windows::Storage::StorageFile^ file)
    {
        if (file != nullptr)
        {
            auto localFolder = Windows::Storage::ApplicationData::Current->LocalFolder;
            create_task(file->CopyAsync(localFolder, file->Name, Windows::Storage::NameCollisionOption::ReplaceExisting)).then([this, file](Windows::Storage::StorageFile^ copiedFile)
            {
                auto localSettings = Windows::Storage::ApplicationData::Current->LocalSettings;
                localSettings->Values->Insert("CustomFontPath", file->Name);
                
                this->Dispatcher->RunAsync(Windows::UI::Core::CoreDispatcherPriority::Normal, ref new Windows::UI::Core::DispatchedHandler([this]() {
                    ThemeSettings::RestartApp();
                }));
            });
        }
    });
}

void Profile::ClearFontButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    auto localSettings = Windows::Storage::ApplicationData::Current->LocalSettings;
    if (localSettings->Values->HasKey("CustomFontPath"))
    {
        localSettings->Values->Remove("CustomFontPath");
        ThemeSettings::RestartApp();
    }
}

void Profile::CustomTabStyleSelector_SelectionChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::SelectionChangedEventArgs^ e)
{
    if (CustomTabStyleSelector == nullptr || CustomTabStyleSelector->SelectedIndex == -1) return;
    auto localSettings = Windows::Storage::ApplicationData::Current->LocalSettings;
    int newStyle = CustomTabStyleSelector->SelectedIndex;
    
    if (localSettings->Values->HasKey("CustomTabStyle"))
    {
        int oldStyle = safe_cast<int>(localSettings->Values->Lookup("CustomTabStyle"));
        if (oldStyle == newStyle) return;
    }
    
    localSettings->Values->Insert("CustomTabStyle", newStyle);
    ThemeSettings::RestartApp();
}

void Profile::Page_KeyDown(Platform::Object^ sender, Windows::UI::Xaml::Input::KeyRoutedEventArgs^ e)
{
    if (e->Key == Windows::System::VirtualKey::GamepadLeftShoulder)
    {
        int nextIndex = (ProfilePivot->SelectedIndex - 1 + ProfilePivot->Items->Size) % ProfilePivot->Items->Size;
        ProfilePivot->SelectedIndex = nextIndex;
        e->Handled = true;
    }
    else if (e->Key == Windows::System::VirtualKey::GamepadRightShoulder)
    {
        int nextIndex = (ProfilePivot->SelectedIndex + 1) % ProfilePivot->Items->Size;
        ProfilePivot->SelectedIndex = nextIndex;
        e->Handled = true;
    }
    else if (e->Key == Windows::System::VirtualKey::GamepadB || e->Key == Windows::System::VirtualKey::Escape)
    {
        // Go back to login if we're on the profile
        if (this->Frame->CanGoBack)
        {
            this->Frame->GoBack();
        }
        else
        {
            this->Frame->Navigate(Windows::UI::Xaml::Interop::TypeName(MainPage::typeid));
        }
        e->Handled = true;
    }
}

void Profile::ApplyThemeSettings()
{
    ThemeSettings::ApplyAccentToResources(Application::Current->Resources);
    ThemeSettings::ApplyAccentToResources(this->Resources);
    
    auto targetTheme = ThemeSettings::GetElementTheme();
    
    // Force a re-evaluation of ThemeResources by toggling the theme on the root frame
    auto rootFrame = dynamic_cast<Windows::UI::Xaml::Controls::Frame^>(Windows::UI::Xaml::Window::Current->Content);
    if (rootFrame != nullptr)
    {
        rootFrame->RequestedTheme = (targetTheme == Windows::UI::Xaml::ElementTheme::Light) ? Windows::UI::Xaml::ElementTheme::Dark : Windows::UI::Xaml::ElementTheme::Light;
        rootFrame->RequestedTheme = targetTheme;
    }

    ThemeSettings::ApplyThemeToTitle(AppTitleTextBlock);
    ThemeSettings::ApplyAccentToButton(ViewAllGamesButton);
    ThemeSettings::ApplyAccentToButton(ClearCredentialsButton);
    ThemeSettings::ApplyAccentToButton(ChooseBackgroundButton);
    ThemeSettings::ApplyAccentToButton(ClearBackgroundButton);
    ThemeSettings::ApplyAccentToButton(NavigatePointsShopButton);

    // Update background button visibility
    auto localSettings = Windows::Storage::ApplicationData::Current->LocalSettings;
    bool hasBackground = localSettings->Values->HasKey("CustomBackgroundPath");
    int theme = ThemeSettings::GetAppTheme();
    bool isRetroStation = (theme == 9);

    if (isRetroStation)
    {
        ChooseBackgroundButton->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
        ClearBackgroundButton->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    }
    else
    {
        ChooseBackgroundButton->Visibility = hasBackground ? Windows::UI::Xaml::Visibility::Collapsed : Windows::UI::Xaml::Visibility::Visible;
        ClearBackgroundButton->Visibility = hasBackground ? Windows::UI::Xaml::Visibility::Visible : Windows::UI::Xaml::Visibility::Collapsed;
    }
    
    bool isCustomTheme = (theme == 11);
    bool hasCustomFont = localSettings->Values->HasKey("CustomFontPath");
    if (isCustomTheme)
    {
        ChooseFontButton->Visibility = hasCustomFont ? Windows::UI::Xaml::Visibility::Collapsed : Windows::UI::Xaml::Visibility::Visible;
        ClearFontButton->Visibility = hasCustomFont ? Windows::UI::Xaml::Visibility::Visible : Windows::UI::Xaml::Visibility::Collapsed;
        
        if (CustomTabStyleSelector != nullptr)
        {
            CustomTabStyleSelector->Visibility = Windows::UI::Xaml::Visibility::Visible;
            int customTabStyle = 0;
            if (localSettings->Values->HasKey("CustomTabStyle"))
            {
                customTabStyle = safe_cast<int>(localSettings->Values->Lookup("CustomTabStyle"));
            }
            CustomTabStyleSelector->SelectedIndex = customTabStyle;
        }
    }
    else
    {
        ChooseFontButton->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
        ClearFontButton->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
        if (CustomTabStyleSelector != nullptr)
        {
            CustomTabStyleSelector->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
        }
    }
    
    // Update accent color picker visibility
    bool isLockedAccent = (theme == 4 || theme == 6 || theme == 7 || theme == 8 || theme == 10);
    if (AccentColorGridView != nullptr)
    {
        AccentColorGridView->Visibility = isLockedAccent ? Windows::UI::Xaml::Visibility::Collapsed : Windows::UI::Xaml::Visibility::Visible;
    }
    if (AccentColorUnavailableText != nullptr)
    {
        AccentColorUnavailableText->Visibility = isLockedAccent ? Windows::UI::Xaml::Visibility::Visible : Windows::UI::Xaml::Visibility::Collapsed;
    }

    // Update logo visibility (Netflix and Steam)
    bool isNetflix = (theme == 6);
    bool isSteam = (theme == 4);
    bool isRetroBoy = (theme == 7);
    bool isHedgehog = (theme == 8);
    bool isBlade = (theme == 10);

    if (NetflixLogoImage != nullptr) NetflixLogoImage->Visibility = isNetflix ? Windows::UI::Xaml::Visibility::Visible : Windows::UI::Xaml::Visibility::Collapsed;
    if (SteamLogoImage != nullptr) SteamLogoImage->Visibility = isSteam ? Windows::UI::Xaml::Visibility::Visible : Windows::UI::Xaml::Visibility::Collapsed;
    if (HedgehogLogoImage != nullptr) {
        if (isHedgehog) {
            HedgehogLogoImage->Source = ref new Windows::UI::Xaml::Media::Imaging::BitmapImage(ref new Windows::Foundation::Uri("ms-appx:///Assets/Theme/Hedgehog/ico.png"));
            HedgehogLogoImage->Visibility = Windows::UI::Xaml::Visibility::Visible;
        } else if (isRetroStation) {
            HedgehogLogoImage->Source = ref new Windows::UI::Xaml::Media::Imaging::BitmapImage(ref new Windows::Foundation::Uri("ms-appx:///Assets/Theme/RetroStation/ico.png"));
            HedgehogLogoImage->Visibility = Windows::UI::Xaml::Visibility::Visible;
        } else {
            HedgehogLogoImage->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
        }
    }
    if (BladeLogoImage != nullptr) BladeLogoImage->Visibility = isBlade ? Windows::UI::Xaml::Visibility::Visible : Windows::UI::Xaml::Visibility::Collapsed;

    if (TotalPointsLabel != nullptr)
    {
        TotalPointsLabel->Text = isHedgehog ? "TOTAL RINGS" : (isBlade ? "GAMERSCORE" : "TOTAL POINTS");
    }

    if (NavigatePointsShopButton != nullptr)
    {
        NavigatePointsShopButton->Content = isHedgehog ? "Open Rings Shop" : (isBlade ? "Open Gamerscore Shop" : "Open Points Shop");
    }
    
    ThemeSettings::ApplyThemeFontToTitle(RecentAchievementsTitle);
    ThemeSettings::ApplyThemeFontToTitle(RecentGamesTitle);
    ThemeSettings::ApplyThemeFontToTitle(WantToPlayTitle);
    ThemeSettings::ApplyThemeFontToTitle(SettingsTitle);
    
    if (AppTitleTextStack != nullptr)
    {
        AppTitleTextStack->Visibility = (isNetflix || isSteam || isHedgehog || isRetroStation || isBlade) ? Windows::UI::Xaml::Visibility::Collapsed : Windows::UI::Xaml::Visibility::Visible;
    }

    // Refresh color picker highlights
    if (AccentColorGridView != nullptr && AccentColorGridView->ItemsSource != nullptr)
    {
        auto items = AccentColorGridView->ItemsSource;
        AccentColorGridView->ItemsSource = nullptr;
        AccentColorGridView->ItemsSource = items;
    }

    ThemeSettings::ApplyBackground(this);
    ThemeSettings::ApplyThemeBorder(RootGrid);

    // Show tab background bar only for Steam theme (index 4) and use the accent color
    if (TabBackgroundBar != nullptr)
    {
        if (ThemeSettings::GetAppTheme() == 4)
        {
            TabBackgroundBar->Visibility = Windows::UI::Xaml::Visibility::Visible;
            TabBackgroundBar->Fill = ThemeSettings::BrushFromHex(ThemeSettings::GetAccentColor());
        }
        else
        {
            TabBackgroundBar->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
        }
    }

    // Update tab icons for RetroStation, hide them for Blade, or restore text for other themes
    int currentSelectedIndex = ProfilePivot != nullptr ? ProfilePivot->SelectedIndex : 0;
    UpdateRetroStationTabHeaders(currentSelectedIndex);
    
    int customTabStyle = 0;
    if (ThemeSettings::GetAppTheme() == 11 && localSettings->Values->HasKey("CustomTabStyle")) {
        customTabStyle = safe_cast<int>(localSettings->Values->Lookup("CustomTabStyle"));
    }
    bool showBladeOverlay = isBlade || (ThemeSettings::GetAppTheme() == 11 && customTabStyle == 2);
    
    if (BladeUIOverlay != nullptr)
    {
        BladeUIOverlay->Visibility = showBladeOverlay ? Windows::UI::Xaml::Visibility::Visible : Windows::UI::Xaml::Visibility::Collapsed;
        if (showBladeOverlay) UpdateBladePositions(currentSelectedIndex);
    }
}

void Profile::UpdateRetroStationTabHeaders(int selectedIndex)
{
    if (AchievementsTab == nullptr || WantToPlayTab == nullptr || SettingsTab == nullptr) return;

    Windows::UI::Xaml::Controls::PivotItem^ tabs[] = { AchievementsTab, WantToPlayTab, SettingsTab };

    int theme = ThemeSettings::GetAppTheme();
    int customTabStyle = 0;
    if (theme == 11) {
        auto localSettings = Windows::Storage::ApplicationData::Current->LocalSettings;
        if (localSettings->Values->HasKey("CustomTabStyle")) {
            customTabStyle = safe_cast<int>(localSettings->Values->Lookup("CustomTabStyle"));
        }
    }

    if (theme == 9 || (theme == 11 && customTabStyle == 1)) // RetroStation or Custom (Icon)
    {
        const wchar_t* normal[] = {
            L"ms-appx:///Assets/Theme/RetroStation/ico1.png",
            L"ms-appx:///Assets/Theme/RetroStation/ico2.png",
            L"ms-appx:///Assets/Theme/RetroStation/ico3.png"
        };
        const wchar_t* sel[] = {
            L"ms-appx:///Assets/Theme/RetroStation/ico1_sel.png",
            L"ms-appx:///Assets/Theme/RetroStation/ico2_sel.png",
            L"ms-appx:///Assets/Theme/RetroStation/ico3_sel.png"
        };
        for (int i = 0; i < 3; i++)
        {
            auto img = ref new Windows::UI::Xaml::Controls::Image();
            img->Width  = 36;
            img->Height = 36;
            img->Stretch = Windows::UI::Xaml::Media::Stretch::Uniform;
            img->Source  = ref new Windows::UI::Xaml::Media::Imaging::BitmapImage(
                ref new Windows::Foundation::Uri(ref new Platform::String(i == selectedIndex ? sel[i] : normal[i])));
            tabs[i]->Header = img;
        }
    }
    else if (theme == 10 || (theme == 11 && customTabStyle == 2)) // Blade or Custom (Blade)
    {
        // Hide headers completely
        for (int i = 0; i < 3; i++)
        {
            auto grid = ref new Windows::UI::Xaml::Controls::Grid();
            grid->Width = 0;
            grid->Height = 0;
            grid->Margin = Windows::UI::Xaml::Thickness(-12, -12, -12, -12); // Negate any default padding
            tabs[i]->Header = grid;
        }
    }
    else
    {
        // Restore text headers
        tabs[0]->Header = ref new Platform::String(L"Achievements");
        tabs[1]->Header = ref new Platform::String(L"Want to Play");
        tabs[2]->Header = ref new Platform::String(L"Settings");
    }
}

void Profile::UpdateBladePositions(int selectedIndex)
{
    if (BladeLeftStack == nullptr || BladeRightStack == nullptr) return;

    Windows::UI::Xaml::Controls::Grid^ blades[] = { Blade0, Blade1, Blade2 };
    Windows::UI::Xaml::Shapes::Path^ graphics[] = { Blade0Graphic, Blade1Graphic, Blade2Graphic };
    Windows::UI::Xaml::Controls::TextBlock^ texts[] = { Blade0Text, Blade1Text, Blade2Text };

    int leftCount = 0;
    int rightCount = 0;
    BladeLeftStack->Children->Clear();
    BladeRightStack->Children->Clear();

    auto solidBlack = ref new Windows::UI::Xaml::Media::SolidColorBrush(Windows::UI::Colors::Black);
    solidBlack->Opacity = 1.0;
    
    // Get the dynamic accent color brush
    auto accentBrush = ThemeSettings::BrushFromHex(ThemeSettings::GetAccentColor());

    // Left side: Outermost (0) to Innermost (selectedIndex)
    for (int i = 0; i <= selectedIndex; i++)
    {
        if (blades[i] == nullptr) continue;
        leftCount++;
        
        // Force a detach/reattach and visibility toggle to jumpstart the renderer
        blades[i]->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
        BladeLeftStack->Children->Append(blades[i]);
        blades[i]->Visibility = Windows::UI::Xaml::Visibility::Visible;

        blades[i]->Opacity = 1.0; // Force full opacity
        if (graphics[i] != nullptr)
        {
            graphics[i]->Fill = solidBlack;
            graphics[i]->Stroke = accentBrush;
            graphics[i]->Opacity = 1.0; // Force full opacity
            auto transform = ref new Windows::UI::Xaml::Media::ScaleTransform();
            transform->ScaleX = 1;
            graphics[i]->RenderTransformOrigin = Windows::Foundation::Point(0.5, 0.5);
            graphics[i]->RenderTransform = transform;
        }
        if (texts[i] != nullptr)
        {
            texts[i]->Foreground = accentBrush;
            texts[i]->Opacity = 1.0;
            auto textTransform = ref new Windows::UI::Xaml::Media::RotateTransform();
            textTransform->Angle = -90;
            texts[i]->RenderTransform = textTransform;
        }
        blades[i]->Margin = Windows::UI::Xaml::Thickness(0, 0, -15, 0);
    }

    // Right side: Outermost (2) to Innermost (selectedIndex + 1)
    for (int i = 2; i > selectedIndex; i--)
    {
        if (blades[i] == nullptr) continue;
        rightCount++;
        
        blades[i]->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
        BladeRightStack->Children->InsertAt(0, blades[i]);
        blades[i]->Visibility = Windows::UI::Xaml::Visibility::Visible;

        blades[i]->Opacity = 1.0; // Force full opacity
        if (graphics[i] != nullptr)
        {
            graphics[i]->Fill = solidBlack;
            graphics[i]->Stroke = accentBrush;
            graphics[i]->Opacity = 1.0; // Force full opacity
            auto transform = ref new Windows::UI::Xaml::Media::ScaleTransform();
            transform->ScaleX = -1;
            graphics[i]->RenderTransformOrigin = Windows::Foundation::Point(0.5, 0.5);
            graphics[i]->RenderTransform = transform;
        }
        if (texts[i] != nullptr)
        {
            texts[i]->Foreground = accentBrush;
            texts[i]->Opacity = 1.0;
            auto textTransform = ref new Windows::UI::Xaml::Media::RotateTransform();
            textTransform->Angle = 90;
            texts[i]->RenderTransform = textTransform;
        }
        blades[i]->Margin = Windows::UI::Xaml::Thickness(0, 0, -15, 0);
    }

    // Update the unified content background and Pivot margins to fit between the stacks
    if (BladeContentBackground != nullptr)
    {
        BladeContentBackground->Visibility = Windows::UI::Xaml::Visibility::Visible;
        BladeContentBackground->BorderBrush = accentBrush;
        // The stacks are leftCount * 30 + 15 pixels wide. 
        // We add a little extra padding (20px) to clear the blades.
        double leftMargin = (leftCount * 30) + 20;
        double rightMargin = (rightCount * 30) + 20;
        BladeContentBackground->Margin = Windows::UI::Xaml::Thickness(leftMargin, 60, rightMargin, 30);
        
        if (ProfilePivot != nullptr)
        {
            // Give the Pivot content the same margins so it sits inside the background box
            ProfilePivot->Margin = Windows::UI::Xaml::Thickness(leftMargin + 10, 65, rightMargin + 10, 35);
        }
    }
}

void Profile::Blade_PointerPressed(Platform::Object^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e)
{
    auto grid = dynamic_cast<Windows::UI::Xaml::Controls::Grid^>(sender);
    if (grid != nullptr && ProfilePivot != nullptr)
    {
        Platform::String^ tagStr = grid->Tag->ToString();
        ProfilePivot->SelectedIndex = _wtoi(tagStr->Data());
    }
}

void Profile::ProfilePivot_SelectionChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::SelectionChangedEventArgs^ e)
{
    if (ProfilePivot == nullptr) return;
    
    int theme = ThemeSettings::GetAppTheme();
    int customTabStyle = 0;
    if (theme == 11) {
        auto localSettings = Windows::Storage::ApplicationData::Current->LocalSettings;
        if (localSettings->Values->HasKey("CustomTabStyle")) {
            customTabStyle = safe_cast<int>(localSettings->Values->Lookup("CustomTabStyle"));
        }
    }

    if (theme == 9 || (theme == 11 && customTabStyle == 1))
    {
        UpdateRetroStationTabHeaders(ProfilePivot->SelectedIndex);
    }
    else if (theme == 10 || (theme == 11 && customTabStyle == 2))
    {
        UpdateBladePositions(ProfilePivot->SelectedIndex);
    }
}

void Profile::OnContainerContentChanging(ListViewBase^ sender, ContainerContentChangingEventArgs^ args)
{
    if (!args->InRecycleQueue)
    {
        auto container = args->ItemContainer;
        if (container != nullptr)
        {
            // Attach focus handlers to the container (the GridViewItem)
            container->GotFocus += ref new RoutedEventHandler(this, &Profile::Item_GotFocus);
            container->LostFocus += ref new RoutedEventHandler(this, &Profile::Item_LostFocus);

            // Apply theme-appropriate image processing (handles Retro Boy effect and standard loading)
            Platform::String^ url = nullptr;
            auto game = dynamic_cast<GameItem^>(args->Item);
            auto achievement = dynamic_cast<RecentAchievementItem^>(args->Item);
            
            if (game != nullptr) url = game->ImageUrl;
            else if (achievement != nullptr) url = achievement->ImageUrl;

            if (url != nullptr)
            {
                // Find the Image controls in the visual tree of the container
                std::function<void(DependencyObject^)> findAndApply = [&](DependencyObject^ parent)
                {
                    int count = Windows::UI::Xaml::Media::VisualTreeHelper::GetChildrenCount(parent);
                    for (int i = 0; i < count; i++)
                    {
                        auto child = Windows::UI::Xaml::Media::VisualTreeHelper::GetChild(parent, i);
                        
                        auto img = dynamic_cast<Windows::UI::Xaml::Controls::Image^>(child);
                        auto border = dynamic_cast<Windows::UI::Xaml::Controls::Border^>(child);

                        if (img != nullptr)
                        {
                            if (img->Name == "IconImage")
                            {
                                ThemeSettings::SetImageWithRetroBoyEffect(img, url);
                            }
                            else if (img->Name == "IconFrame")
                            {
                                if (ThemeSettings::GetAppTheme() == 8) // Hedgehog
                                {
                                    img->Source = ref new Windows::UI::Xaml::Media::Imaging::BitmapImage(ref new Windows::Foundation::Uri("ms-appx:///Assets/Theme/Hedgehog/ring.png"));
                                    img->Visibility = Windows::UI::Xaml::Visibility::Visible;
                                }
                                else
                                {
                                    img->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
                                }
                            }
                        }
                        else if (border != nullptr)
                        {
                            if (border->Name == "IconBorder")
                            {
                                if (ThemeSettings::GetAppTheme() == 8) // Hedgehog
                                {
                                    border->Width = 75;
                                    border->Height = 75;
                                    border->CornerRadius = Windows::UI::Xaml::CornerRadius(37.5);
                                }
                                else
                                {
                                    border->Width = 100;
                                    border->Height = 100;
                                    border->CornerRadius = Windows::UI::Xaml::CornerRadius(0);
                                }
                            }
                            findAndApply(border);
                        }
                        else
                        {
                            findAndApply(child);
                        }
                    }
                };
                findAndApply(container);
            }
        }
    }
}

void Profile::Item_GotFocus(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    auto container = dynamic_cast<DependencyObject^>(sender);
    if (container != nullptr)
    {
        // Helper to find an element with a tooltip recursively
        std::function<void(DependencyObject^)> openTooltip = [&](DependencyObject^ obj)
        {
            auto tooltip = ToolTipService::GetToolTip(obj);
            if (tooltip != nullptr)
            {
                auto tt = dynamic_cast<ToolTip^>(tooltip);
                if (tt != nullptr)
                {
                    tt->IsOpen = true;
                }
                return;
            }

            int count = Windows::UI::Xaml::Media::VisualTreeHelper::GetChildrenCount(obj);
            for (int i = 0; i < count; i++)
            {
                openTooltip(Windows::UI::Xaml::Media::VisualTreeHelper::GetChild(obj, i));
            }
        };

        openTooltip(container);
    }
}

void Profile::Item_LostFocus(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    auto container = dynamic_cast<DependencyObject^>(sender);
    if (container != nullptr)
    {
        // Helper to close tooltip recursively
        std::function<void(DependencyObject^)> closeTooltip = [&](DependencyObject^ obj)
        {
            auto tooltip = ToolTipService::GetToolTip(obj);
            if (tooltip != nullptr)
            {
                auto tt = dynamic_cast<ToolTip^>(tooltip);
                if (tt != nullptr)
                {
                    tt->IsOpen = false;
                }
                return;
            }

            int count = Windows::UI::Xaml::Media::VisualTreeHelper::GetChildrenCount(obj);
            for (int i = 0; i < count; i++)
            {
                closeTooltip(Windows::UI::Xaml::Media::VisualTreeHelper::GetChild(obj, i));
            }
        };

        closeTooltip(container);
    }
}
