#include "pch.h"
#include "AllGamesPage.xaml.h"
#include "Profile.xaml.h"
#include "GameInfoPage.xaml.h"
#include "CredentialManager.h"
#include "ThemeSettings.h"
#include <ppltasks.h>
#include <string>
#include <vector>
#include <functional>

using namespace RetroAchievements_UWP;

using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::Web::Http;
using namespace concurrency;
using namespace Windows::UI::Core;

AllGamesPage::AllGamesPage()
{
    InitializeComponent();
}

void AllGamesPage::OnNavigatedTo(Windows::UI::Xaml::Navigation::NavigationEventArgs^ e)
{
    UserProfileHeaderData^ headerData = dynamic_cast<UserProfileHeaderData^>(e->Parameter);
    if (headerData != nullptr)
    {
        ProfileImageBrush->ImageSource = headerData->ProfileImage;
        UserTextBlock->Text = headerData->Username;
        MemberSinceTextBlock->Text = headerData->MemberSince;
        MottoTextBlock->Text = headerData->Motto;
        TotalPointsTextBlock->Text = headerData->TotalPoints;
        
        // Load active avatar frame
        auto settings = Windows::Storage::ApplicationData::Current->LocalSettings->Values;
        if (settings->HasKey("ActiveAvatarFrame"))
        {
            auto activeFrameUrl = dynamic_cast<Platform::String^>(settings->Lookup("ActiveAvatarFrame"));
            if (activeFrameUrl != nullptr)
            {
                AvatarFrameImage->Source = ref new Windows::UI::Xaml::Media::Imaging::BitmapImage(ref new Windows::Foundation::Uri(activeFrameUrl));
            }
        }
        else
        {
            AvatarFrameImage->Source = nullptr;
        }

        ThemeSettings::ApplyAccentToResources(this->Resources);
        this->RequestedTheme = ThemeSettings::GetElementTheme();
        ThemeSettings::ApplyThemeToTitle(AppTitleTextBlock);
        ThemeSettings::ApplyThemeToBackButton(BackButton);
        ThemeSettings::ApplyBackground(this);
        ThemeSettings::ApplyThemeBorder(RootGrid);

        // Apply theme-specific logos and labels
        int currentTheme = ThemeSettings::GetAppTheme();
        bool isNetflix = (currentTheme == 6);
        bool isSteam = (currentTheme == 4);
        bool isHedgehog = (currentTheme == 8);
        bool isRetroStation = (currentTheme == 9);
        bool isBlade = (currentTheme == 10);

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

        if (TotalPointsLabel != nullptr) TotalPointsLabel->Text = isHedgehog ? "TOTAL RINGS" : (isBlade ? "GAMERSCORE" : "TOTAL POINTS");

        ThemeSettings::ApplyThemeFontToTitle(AllGamesTitleTextBlock);

        if (AppTitleTextStack != nullptr)
        {
            AppTitleTextStack->Visibility = (isNetflix || isSteam || isHedgehog || isRetroStation || isBlade) ? Windows::UI::Xaml::Visibility::Collapsed : Windows::UI::Xaml::Visibility::Visible;
        }

        FetchAllGames();
        BackButton->Focus(Windows::UI::Xaml::FocusState::Programmatic);
    }
}

void AllGamesPage::FetchAllGames()
{
    Platform::String^ username;
    Platform::String^ apiKey;

    if (!CredentialManager::GetInstance()->LoadCredentials(&username, &apiKey))
    {
        return;
    }

    // Call API_GetUserCompletionProgress.php
    std::wstring baseUrl = L"https://retroachievements.org/API/API_GetUserCompletionProgress.php?";
    std::wstring fullUrl = baseUrl;
    fullUrl += L"z=" + std::wstring(username->Data());
    fullUrl += L"&y=" + std::wstring(apiKey->Data());
    fullUrl += L"&u=" + std::wstring(username->Data());
    fullUrl += L"&c=100"; // Fetch up to 100 games for now, or omit to fetch all? The API accepts count. Let's omit for all or use default. Actually, c=100 is good to limit response size if user has played many games. I'll omit to let default behavior happen or set a reasonable limit like 100. Let's omit.
    // Wait, the API docs say c (count) and o (offset). If not provided, it might return all or default to a limit.

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

                    std::vector<GameItem^> itemsList;
                    
                    // The API returns an object with a "Results" array
                    size_t resultsPos = responseBody.find(L"\"Results\"");
                    if (resultsPos == std::wstring::npos) return;
                    
                    size_t currentPos = responseBody.find(L"[", resultsPos);
                    if (currentPos == std::wstring::npos) return;

                    while (currentPos < responseBody.length())
                    {
                        size_t objectStart = responseBody.find(L"{", currentPos);
                        if (objectStart == std::wstring::npos)
                            break;

                        size_t objectEnd = responseBody.find(L"}", objectStart);
                        if (objectEnd == std::wstring::npos)
                            break;

                        std::wstring gameObj = responseBody.substr(objectStart, objectEnd - objectStart + 1);
                        std::wstring gameId = extractJsonNumber(gameObj, L"GameID");
                        if (gameId.empty()) gameId = extractJsonString(gameObj, L"GameID");
                        std::wstring title = extractJsonString(gameObj, L"Title");
                        std::wstring consoleName = extractJsonString(gameObj, L"ConsoleName");
                        std::wstring imageIcon = extractJsonString(gameObj, L"ImageIcon");
                        std::wstring numAchieved = extractJsonNumber(gameObj, L"NumAwarded");
                        if (numAchieved.empty()) numAchieved = extractJsonString(gameObj, L"NumAwarded");
                        std::wstring numPossible = extractJsonNumber(gameObj, L"MaxPossible");
                        if (numPossible.empty()) numPossible = extractJsonString(gameObj, L"MaxPossible");

                        if (!gameId.empty() && !title.empty())
                        {
                            GameItem^ item = ref new GameItem();
                            item->GameID = ref new Platform::String(gameId.c_str());
                            item->Title = ref new Platform::String(title.c_str());
                            item->ConsoleName = ref new Platform::String(consoleName.c_str());
                            item->ImageUrl = ref new Platform::String((L"https://retroachievements.org" + imageIcon).c_str());
                            
                            double achieved = 0;
                            double possible = 0;
                            try { achieved = std::stod(numAchieved); } catch (...) {}
                            try { possible = std::stod(numPossible); } catch (...) {}
                            
                            item->NumAchieved = achieved;
                            item->NumPossibleAchievements = possible;
                            item->AchievementProgress = ref new Platform::String((numAchieved + L" / " + numPossible).c_str());
                            
                            itemsList.push_back(item);
                        }

                        currentPos = objectEnd + 1;
                    }

                    auto dispatcher = this->Dispatcher;
                    dispatcher->RunAsync(CoreDispatcherPriority::Normal, ref new DispatchedHandler([this, itemsList]() {
                        auto items = AllGamesGridView->Items;
                        items->Clear();
                        for (auto item : itemsList)
                        {
                            items->Append(item);
                        }

                        // Update title with count
                        AllGamesTitleTextBlock->Text = "All Games (" + itemsList.size().ToString() + ")";
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

void AllGamesPage::AllGamesGridView_ItemClick(Platform::Object^ sender, Windows::UI::Xaml::Controls::ItemClickEventArgs^ e)
{
    GameItem^ clickedItem = dynamic_cast<GameItem^>(e->ClickedItem);
    if (clickedItem != nullptr)
    {
        UserProfileHeaderData^ headerData = ref new UserProfileHeaderData();
        headerData->ProfileImage = ProfileImageBrush->ImageSource;
        headerData->Username = UserTextBlock->Text;
        headerData->MemberSince = MemberSinceTextBlock->Text;
        headerData->Motto = MottoTextBlock->Text;
        headerData->TotalPoints = TotalPointsTextBlock->Text;

        GameInfoNavParam^ param = ref new GameInfoNavParam();
        param->Game = clickedItem;
        param->Header = headerData;

        this->Frame->Navigate(Windows::UI::Xaml::Interop::TypeName(GameInfoPage::typeid), param);
    }
}

void AllGamesPage::Page_KeyDown(Platform::Object^ sender, Windows::UI::Xaml::Input::KeyRoutedEventArgs^ e)
{
    if (e->Key == Windows::System::VirtualKey::GamepadB || e->Key == Windows::System::VirtualKey::Escape)
    {
        if (this->Frame->CanGoBack)
        {
            this->Frame->GoBack();
            e->Handled = true;
        }
    }
}

void AllGamesPage::BackButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    if (this->Frame->CanGoBack)
    {
        this->Frame->GoBack();
    }
}
void AllGamesPage::OnContainerContentChanging(ListViewBase^ sender, ContainerContentChangingEventArgs^ args)
{
    if (!args->InRecycleQueue)
    {
        auto container = args->ItemContainer;
        if (container != nullptr)
        {
            container->GotFocus += ref new RoutedEventHandler(this, &AllGamesPage::Item_GotFocus);
            container->LostFocus += ref new RoutedEventHandler(this, &AllGamesPage::Item_LostFocus);

            // Apply theme-appropriate image processing
            auto game = dynamic_cast<GameItem^>(args->Item);
            if (game != nullptr && game->ImageUrl != nullptr)
            {
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
                                ThemeSettings::SetImageWithRetroBoyEffect(img, game->ImageUrl);
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

void AllGamesPage::Item_GotFocus(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    auto container = dynamic_cast<DependencyObject^>(sender);
    if (container != nullptr)
    {
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

void AllGamesPage::Item_LostFocus(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    auto container = dynamic_cast<DependencyObject^>(sender);
    if (container != nullptr)
    {
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
