using System;
using System.Collections.Generic;
using WLANWecker.View;
using Xamarin.Forms;

namespace WLANWecker
{
    public partial class AppShell : Xamarin.Forms.Shell
    {
        public AppShell()
        {
            InitializeComponent();

            Routing.RegisterRoute(nameof(ClockView), typeof(ClockView));
            Routing.RegisterRoute(nameof(ClockItemView), typeof(ClockItemView));
            Routing.RegisterRoute(nameof(AlarmView), typeof(AlarmView));
            Routing.RegisterRoute(nameof(ClockSettingsView), typeof(ClockSettingsView));
        }
    }
}
