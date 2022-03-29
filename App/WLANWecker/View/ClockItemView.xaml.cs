using System;
using System.Collections.Generic;
using WLANWecker.ViewModel;
using Xamarin.Forms;

namespace WLANWecker.View
{
    public partial class ClockItemView : ContentPage
    {
        public ClockItemView()
        {
            InitializeComponent();
            BindingContext = new ClockItemViewModel();
        }
    }
}
