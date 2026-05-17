#pragma once

#include "PointsShop.g.h"
#include "ThemeSettings.h"

namespace RetroAchievements_UWP
{
	[Windows::UI::Xaml::Data::Bindable]
	public ref class ShopItem sealed
	{
	public:
		ShopItem() { }

		property Windows::UI::Xaml::Media::ImageSource^ ProfileImage;
		property Platform::String^ Title;
		property int CostValue;
		property Platform::String^ Cost;
		property Platform::String^ ImageUrl;
		property Platform::String^ ItemType; // "Frame" or "Theme"
		property bool IsUnlocked;

		property Platform::String^ ButtonText;
		property bool IsButtonEnabled;
	};

	[Windows::Foundation::Metadata::WebHostHidden]
	public ref class PointsShop sealed
	{
	public:
		PointsShop();

		property Windows::Foundation::Collections::IObservableVector<ShopItem^>^ Items
		{
			Windows::Foundation::Collections::IObservableVector<ShopItem^>^ get() { return _items; }
		}

		property Windows::Foundation::Collections::IObservableVector<ShopItem^>^ Themes
		{
			Windows::Foundation::Collections::IObservableVector<ShopItem^>^ get() { return _themes; }
		}

	protected:
		virtual void OnNavigatedTo(Windows::UI::Xaml::Navigation::NavigationEventArgs^ e) override;

	private:
		Windows::Foundation::Collections::IObservableVector<ShopItem^>^ _items;
		Windows::Foundation::Collections::IObservableVector<ShopItem^>^ _themes;
		void AddShopItem(Platform::String^ title, int costVal, Platform::String^ costText, Platform::String^ imageUrl, Platform::String^ type = "Frame");
		void OnContainerContentChanging(Windows::UI::Xaml::Controls::ListViewBase^ sender, Windows::UI::Xaml::Controls::ContainerContentChangingEventArgs^ args);
		void BackButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void ActionButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void ApplyTheme();
	};
}
