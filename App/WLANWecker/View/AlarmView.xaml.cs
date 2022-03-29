using System;
using System.Collections.Generic;
using WLANWecker.ViewModel;
using Xamarin.Forms;

namespace WLANWecker.View
{
    public partial class AlarmView : ContentPage
    {
        public AlarmView()
        {
            InitializeComponent();
            BindingContext = new AlarmViewModel();
        }
    }
}
