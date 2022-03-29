using System;
using System.Collections.Generic;
using WLANWecker.ViewModel;
using Xamarin.Forms;

namespace WLANWecker.View
{
    public partial class ClockView : ContentPage
    {
        public ClockView()
        {
            InitializeComponent();

            BindingContext = new ClockViewModel();
        }
    }
}
