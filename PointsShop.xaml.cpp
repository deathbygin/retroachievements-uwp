#include "pch.h"
#include <algorithm>
#include "PointsShop.xaml.h"
#include "Profile.xaml.h"

using namespace RetroAchievements_UWP;

using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;
using namespace Windows::UI::Xaml::Media::Imaging;

PointsShop::PointsShop()
{
	InitializeComponent();
	_items = ref new Platform::Collections::Vector<ShopItem^>();
	_themes = ref new Platform::Collections::Vector<ShopItem^>();
	
	// Arcade Hero Theme (250 Points)
	AddShopItem("Arcade Hero", 250, "250 Points", "ms-appx:///Assets/Theme/ArcadeHero/ico.png", "Theme");

	// Steamed Theme (500 Points)
	AddShopItem("Steamed", 500, "500 Points", "ms-appx:///Assets/Theme/Steam/ico.png", "Theme");

	// Retroflix Theme (800 Points)
	AddShopItem("Retroflix", 800, "800 Points", "ms-appx:///Assets/Theme/Netflix/ico.png", "Theme");

	// Retro Boy Theme (1100 Points)
	AddShopItem("Retro Boy", 1100, "1100 Points", "ms-appx:///Assets/Theme/RetroBoy/ico.png", "Theme");

	// Hedgehog Theme (1500 Points)
	AddShopItem("Hedgehog", 1500, "1500 Points", "ms-appx:///Assets/Theme/Hedgehog/ico.png", "Theme");

	// RetroStation Theme (1900 Points)
	AddShopItem("RetroStation", 1900, "1900 Points", "ms-appx:///Assets/Theme/RetroStation/ico.png", "Theme");

	// Blade Theme (2400 Points)
	AddShopItem("Blade", 2400, "2400 Points", "ms-appx:///Assets/Theme/Blade/ico.png", "Theme");

	// Custom Theme (3000 Points - Final Unlock)
	AddShopItem("Custom", 3000, "3000 Points", "ms-appx:///Assets/Theme/Custom/ico.png", "Theme");

	// Free Frames
	AddShopItem("Basic Red", 0, "Free", "ms-appx:///Assets/Avatars/b_red.png");
	AddShopItem("Basic Blue", 0, "Free", "ms-appx:///Assets/Avatars/b_blue.png");
	AddShopItem("Basic Green", 0, "Free", "ms-appx:///Assets/Avatars/b_green.png");

	// Starter Progression
	AddShopItem("Basic Silver", 10, "10 Points", "ms-appx:///Assets/Avatars/b_silver.png");
	AddShopItem("Basic Gold", 50, "50 Points", "ms-appx:///Assets/Avatars/b_gold.png");
	AddShopItem("Novice Frame", 100, "100 Points", "ms-appx:///Assets/Avatars/novice.png");
	AddShopItem("Amateur Frame", 150, "150 Points", "ms-appx:///Assets/Avatars/amateur.png");
	AddShopItem("Pro Frame", 250, "250 Points", "ms-appx:///Assets/Avatars/pro.png");

	// Advanced Progression (350 - 7000 Points)
	AddShopItem("Ring of Fire", 350, "350 Points", "ms-appx:///Assets/Avatars/ring_of_fire.png");
	AddShopItem("Ice Ring", 500, "500 Points", "ms-appx:///Assets/Avatars/ice_ring.png");
	AddShopItem("C.R.E.A.M", 750, "750 Points", "ms-appx:///Assets/Avatars/cream.png");
	AddShopItem("Thorned Rose", 1000, "1,000 Points", "ms-appx:///Assets/Avatars/thorned_rose.png");
	AddShopItem("Steampunk'd", 1300, "1,300 Points", "ms-appx:///Assets/Avatars/steampunk.png");
	AddShopItem("Cherry Blossom", 1700, "1,700 Points", "ms-appx:///Assets/Avatars/cherryblossom.png");
	AddShopItem("Color Picker", 2100, "2,100 Points", "ms-appx:///Assets/Avatars/colorpicker.png");
	AddShopItem("Collared", 2600, "2,600 Points", "ms-appx:///Assets/Avatars/collared.png");
	AddShopItem("Keychain Pet", 3200, "3,200 Points", "ms-appx:///Assets/Avatars/kc_pet.png");
	AddShopItem("Kanagawa", 3800, "3,800 Points", "ms-appx:///Assets/Avatars/kanagawa.png");
	AddShopItem("Familiar Brick", 4500, "4,500 Points", "ms-appx:///Assets/Avatars/f_brick.png");
	AddShopItem("Puffy Cloud", 5200, "5,200 Points", "ms-appx:///Assets/Avatars/puffycloud.png");
	AddShopItem("Darkflame", 5800, "5,800 Points", "ms-appx:///Assets/Avatars/darkflame.png");
	AddShopItem("Sketchy", 6300, "6,300 Points", "ms-appx:///Assets/Avatars/sketchy.png");
	AddShopItem("Hermaeus", 6600, "6,600 Points", "ms-appx:///Assets/Avatars/hermaeus.png");
	AddShopItem("Crayon", 6800, "6,800 Points", "ms-appx:///Assets/Avatars/crayon.png");
	AddShopItem("Demonic", 6900, "6,900 Points", "ms-appx:///Assets/Avatars/demonic.png");
	AddShopItem("Ghostly", 6950, "6,950 Points", "ms-appx:///Assets/Avatars/ghostly.png");
	AddShopItem("Falling Leaves", 6975, "6,975 Points", "ms-appx:///Assets/Avatars/fallingleaves.png");
	AddShopItem("Royalty Frame", 7000, "7,000 Points", "ms-appx:///Assets/Avatars/royalty.png");
}

void PointsShop::AddShopItem(Platform::String^ title, int costVal, Platform::String^ costText, Platform::String^ imageUrl, Platform::String^ type)
{
	auto item = ref new ShopItem();
	item->Title = title;
	item->CostValue = costVal;
	if (ThemeSettings::GetAppTheme() == 8)
	{
		std::wstring cost(costText->Data());
		size_t pos = cost.find(L"Points");
		if (pos != std::wstring::npos)
		{
			cost.replace(pos, 6, L"Rings");
		}
		item->Cost = ref new Platform::String(cost.c_str());
	}
	else
	{
		item->Cost = costText;
	}
	item->ImageUrl = imageUrl;
	item->ItemType = type;
	
	if (type == "Theme") _themes->Append(item);
	else _items->Append(item);
}

void PointsShop::OnContainerContentChanging(ListViewBase^ sender, ContainerContentChangingEventArgs^ args)
{
	// Native x:Bind handles image loading for shop items now
}

void PointsShop::OnNavigatedTo(NavigationEventArgs^ e)
{
	ApplyTheme();

	auto settings = Windows::Storage::ApplicationData::Current->LocalSettings->Values;

	auto headerData = dynamic_cast<UserProfileHeaderData^>(e->Parameter);
	if (headerData != nullptr)
	{
		UserTextBlock->Text = headerData->Username;
		MottoTextBlock->Text = headerData->Motto;
		MemberSinceTextBlock->Text = headerData->MemberSince;
		TotalPointsTextBlock->Text = headerData->TotalPoints;
		
		ProfileImageBrush->ImageSource = ref new Windows::UI::Xaml::Media::Imaging::BitmapImage(ref new Windows::Foundation::Uri(headerData->ProfileImageUrl));

		// Load active frame in header
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

		// Parse points and check unlocks
		std::wstring ws(headerData->TotalPoints->Data());
		ws.erase(std::remove(ws.begin(), ws.end(), L','), ws.end());
		int userPoints = 0;
		try { userPoints = std::stoi(ws); } catch (...) {}

		Platform::String^ activeFrame = nullptr;
		if (settings->HasKey("ActiveAvatarFrame"))
		{
			activeFrame = dynamic_cast<Platform::String^>(settings->Lookup("ActiveAvatarFrame"));
		}

		// Calculate for Frames
		for (uint32 i = 0; i < _items->Size; i++)
		{
			auto item = _items->GetAt(i);
			item->ProfileImage = headerData->ProfileImage;

			if (userPoints >= item->CostValue)
			{
				item->IsUnlocked = true;
				item->IsButtonEnabled = true;

				if (activeFrame != nullptr && activeFrame == item->ImageUrl)
				{
					item->ButtonText = "Unset";
				}
				else
				{
					item->ButtonText = "Set";
				}
			}
			else
			{
				item->IsUnlocked = false;
				item->ButtonText = "Locked";
				item->IsButtonEnabled = false;
			}
		}

		// Calculate for Themes
		for (uint32 i = 0; i < _themes->Size; i++)
		{
			auto item = _themes->GetAt(i);
			item->ProfileImage = headerData->ProfileImage;

			if (userPoints >= item->CostValue)
			{
				item->IsUnlocked = true;
				item->ButtonText = "Unlocked";
				item->IsButtonEnabled = true; // Still enabled so it can be selected, but we won't hook click in XAML for themes if needed, or just make it unclickable
			}
			else
			{
				item->IsUnlocked = false;
				item->ButtonText = "Locked";
				item->IsButtonEnabled = false;
			}
		}
	}
}

void PointsShop::BackButton_Click(Object^ sender, RoutedEventArgs^ e)
{
	if (this->Frame->CanGoBack)
	{
		this->Frame->GoBack();
	}
}

void PointsShop::ActionButton_Click(Object^ sender, RoutedEventArgs^ e)
{
	auto button = dynamic_cast<Button^>(sender);
	auto clickedItem = dynamic_cast<ShopItem^>(button->DataContext);

	if (clickedItem != nullptr && clickedItem->IsUnlocked)
	{
		auto settings = Windows::Storage::ApplicationData::Current->LocalSettings->Values;
		
		if (clickedItem->ItemType == "Theme")
		{
			int themeIndex = -1;
			if (clickedItem->Title == "Arcade Hero") themeIndex = 3;
			else if (clickedItem->Title == "Steamed") themeIndex = 4;
			else if (clickedItem->Title == "Retroflix") themeIndex = 6;
			else if (clickedItem->Title == "Retro Boy") themeIndex = 7;
			else if (clickedItem->Title == "Hedgehog") themeIndex = 8;
			else if (clickedItem->Title == "Blade") themeIndex = 10;
			else if (clickedItem->Title == "Custom") themeIndex = 11;

			if (themeIndex != -1)
			{
				settings->Insert("AppTheme", themeIndex);
				ThemeSettings::RestartApp();
			}
			return;
		}

		if (clickedItem->ButtonText == "Unset")
		{
			// Unset the current frame
			settings->Remove("ActiveAvatarFrame");
			clickedItem->ButtonText = "Set";
			AvatarFrameImage->Source = nullptr;
		}
		else
		{
			// Set the new frame
			settings->Insert("ActiveAvatarFrame", clickedItem->ImageUrl);
			AvatarFrameImage->Source = ref new BitmapImage(ref new Uri(clickedItem->ImageUrl));
			
			// Update all items to ensure only the clicked one is "Unset"
			for (uint32 i = 0; i < _items->Size; i++)
			{
				auto item = _items->GetAt(i);
				if (item->IsUnlocked)
				{
					if (item == clickedItem)
					{
						item->ButtonText = "Unset";
					}
					else
					{
						item->ButtonText = "Set";
					}
				}
			}
		}
		
		// Force refresh the GridView items so they pick up new Set/Unset property states immediately
		if (ShopGridView != nullptr)
		{
			auto itemsSource = ShopGridView->ItemsSource;
			ShopGridView->ItemsSource = nullptr;
			ShopGridView->ItemsSource = itemsSource;
		}
	}
}

void PointsShop::ApplyTheme()
{
	ThemeSettings::ApplyAccentToResources(this->Resources);
	ThemeSettings::ApplyThemeToBackButton(BackButton);
	// Update logo visibility (Netflix and Steam)
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

	if (TotalPointsLabel != nullptr)
	{
		TotalPointsLabel->Text = isHedgehog ? "TOTAL RINGS" : (isBlade ? "GAMERSCORE" : "TOTAL POINTS");
	}

	ThemeSettings::ApplyThemeFontToTitle(AvatarFramesTitleTextBlock);
	ThemeSettings::ApplyThemeFontToTitle(ThemesTitleTextBlock);

	if (AppTitleTextStack != nullptr)
	{
		AppTitleTextStack->Visibility = (isNetflix || isSteam || isHedgehog || isRetroStation || isBlade) ? Windows::UI::Xaml::Visibility::Collapsed : Windows::UI::Xaml::Visibility::Visible;
	}

	ThemeSettings::ApplyThemeBorder(RootGrid);
	ThemeSettings::ApplyBackground(this);
}
