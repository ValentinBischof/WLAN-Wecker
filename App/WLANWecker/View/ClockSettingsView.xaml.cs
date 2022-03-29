using System;
using System.Collections.Generic;
using WLANWecker.ViewModel;
using Xamarin.Forms;

namespace WLANWecker.View
{
    public partial class ClockSettingsView : ContentPage
    {
        public ClockSettingsView()
        {
            InitializeComponent();

           BindingContext = new ClockSettingsViewModel();
        }
    }
}
